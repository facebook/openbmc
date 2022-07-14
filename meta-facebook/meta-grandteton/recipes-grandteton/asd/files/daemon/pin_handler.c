#include <pthread.h>
#include <safe_str_lib.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-pal.h>
#include <openbmc/pal_def.h> 
#include "target_handler.h"
#include "logging.h"

// Set alias
#define BMC_JTAG_SEL_N BMC_RSMRST_B
#define PREQ_WAIT_US 10000

// PLTRST, PWRGD, PRDY, XDP_PRESENT
enum MONITOR_EVENTS {
    JTAG_PLTRST_EVENT = 0,
    JTAG_PWRGD_EVENT,
    JTAG_PRDY_EVENT,
    JTAG_XDP_PRESENT_EVENT,
    JTAG_EVENT_NUM,
};

struct gpio_evt {
    bool triggered;
    gpio_value_t value;
};

static void gpio_handle(gpiopoll_pin_t *gpio, gpio_value_t last, gpio_value_t curr);
static struct gpiopoll_config g_gpios[JTAG_EVENT_NUM] = {
    {RST_PLTRST_BMC_N, "0", GPIO_EDGE_BOTH, gpio_handle, NULL},
    {FM_CPU0_PWRGD, "1", GPIO_EDGE_BOTH, gpio_handle, NULL},
    {IRQ_BMC_PRDY_N, "2", GPIO_EDGE_FALLING, gpio_handle, NULL},
    {DBP_PRESENT_N, "3", GPIO_EDGE_BOTH, gpio_handle, NULL}
};
static gpiopoll_desc_t *polldesc = NULL;
static struct gpio_evt g_gpios_evt[JTAG_EVENT_NUM] = {
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID}
};
static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value, STATUS* result) {
    gpio_value_t val = gpio_get_value_by_shadow(gpio.name);

    if (val == GPIO_VALUE_INVALID) {
        *result = ST_ERR;
        return;
    }

    if (gpio.active_low) {
        *value = (val == GPIO_VALUE_LOW) ? 1 : 0;
    } else {
        *value = (val == GPIO_VALUE_HIGH) ? 1 : 0;
    }

    *result = ST_OK;
}

void write_pin_value(uint8_t fru, Target_Control_GPIO gpio, int value, STATUS* result) {
    gpio_value_t val;

    if (gpio.fd < 0 || gpio.fd == BMC_SYSPWROK) {
        *result = ST_OK;
        return;
    }

    if (gpio.active_low) {
        val = (value) ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;
    } else {
        val = (value) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    }

    *result = (!gpio_set_value_by_shadow(gpio.name, val)) ? ST_OK : ST_ERR;
    if (gpio.fd == BMC_TCK_MUX_SEL) {
        msleep(1);
    }
}

void get_pin_events(Target_Control_GPIO gpio, short* events) {
    *events = POLL_GPIO; /*Not used*/
}

static void gpio_handle(gpiopoll_pin_t *gpio, gpio_value_t last, gpio_value_t curr)
{
    size_t idx;
    const struct gpiopoll_config *cfg = gpio_poll_get_config(gpio);
    if (!cfg) {
        return;
    }
    idx = atoi(cfg->description);
    if (idx < 4) {
        pthread_mutex_lock(&triggered_mutex);
        g_gpios_evt[idx].value = curr;
        g_gpios_evt[idx].triggered = true;
        pthread_mutex_unlock(&triggered_mutex);
    }
}

static void *gpio_poll_thread(void *unused) {
    gpio_poll(polldesc, -1);
    return NULL;
}

STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle *state) {
    STATUS result = ST_ERR;
    gpio_desc_t *gpio;
    int ret = -1;

    if ((gpio = gpio_open_by_shadow(state->gpios[BMC_PREQ_N].name))) {
        ret = gpio_set_init_value(gpio, GPIO_VALUE_HIGH);
        gpio_close(gpio);
    }
    if (!ret) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_PREQ_N is set to High successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_PREQ_N");
        return ST_ERR;
    }

    write_pin_value(state->fru, state->gpios[BMC_JTAG_SEL_N], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_JTAG_SEL_N is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_JTAG_SEL_N");
        return ST_ERR;
    }

    static bool gpios_polling = false;
    static pthread_t poll_thread;
    if (gpios_polling == false) {
        polldesc = gpio_poll_open(g_gpios, 4);
        if (!polldesc) {
          return ST_ERR;
        }
        pthread_create(&poll_thread, NULL, gpio_poll_thread, NULL);
        gpios_polling = true;
    } else {
        pthread_mutex_lock(&triggered_mutex);
        g_gpios_evt[JTAG_PLTRST_EVENT].triggered = false;
        g_gpios_evt[JTAG_PWRGD_EVENT].triggered = false;
        g_gpios_evt[JTAG_PRDY_EVENT].triggered = false;
        g_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered = false;
        pthread_mutex_unlock(&triggered_mutex);
    }

    return ST_OK;
}

STATUS pin_hndlr_deinit_asd_gpio(Target_Control_Handle *state) {
    STATUS result = ST_ERR;
    bool is_deinitialized = true;
    gpio_desc_t *gpio;
    int ret = -1;

    if ((gpio = gpio_open_by_shadow(state->gpios[BMC_PREQ_N].name))) {
        ret = gpio_set_direction(gpio, GPIO_DIRECTION_IN);
        gpio_close(gpio);
    }
    if (ret) {
        ASD_log(ASD_LogLevel_Error, stream, option, "deinit BMC_PREQ_N failed");
        is_deinitialized = false;
    }

    write_pin_value(state->fru, state->gpios[BMC_JTAG_SEL_N], 1, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_JTAG_SEL_N is set to 1 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_JTAG_SEL_N");
        is_deinitialized = false;
    }

    return (is_deinitialized == true)?ST_OK:ST_ERR;
}

STATUS pin_hndlr_read_gpio_event(Target_Control_Handle* state,
                                 struct pollfd poll_fd,
                                 ASD_EVENT* event) {
    //printf("state->gpios[poll_fd.fd].name :%s(%d)\n", state->gpios[poll_fd.fd].name, poll_fd.fd);
    state->gpios[poll_fd.fd].handler(state, event);
    return ST_OK;
}

STATUS on_power_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    gpio_value_t value;

    if (g_gpios_evt[JTAG_PWRGD_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

    pthread_mutex_lock(&triggered_mutex);
    value = g_gpios_evt[JTAG_PWRGD_EVENT].value;
    g_gpios_evt[JTAG_PWRGD_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    if (value == 1) {
        *event = ASD_EVENT_PWRRESTORE;
    } else {
        *event = ASD_EVENT_PWRFAIL;
    }
#ifdef ENABLE_DEBUG_LOGGING
    if (value == 1) {
        ASD_log(ASD_LogLevel_Debug, stream, option, "Power restored");
    } else {
        ASD_log(ASD_LogLevel_Debug, stream, option, "Power fail");
    }
#endif    

    return result;
}

STATUS on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    gpio_value_t value;

    if (g_gpios_evt[JTAG_PLTRST_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

    pthread_mutex_lock(&triggered_mutex);
    value = !g_gpios_evt[JTAG_PLTRST_EVENT].value;
    g_gpios_evt[JTAG_PLTRST_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    if (value == 0) {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "Platform reset de-asserted");
#endif
        *event = ASD_EVENT_PLRSTDEASSRT;
        if (state->event_cfg.reset_break) {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "ResetBreak detected PLT_RESET "
                    "assert, asserting PREQ");
#endif
            write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 1, &result);
            if (result != ST_OK) {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to assert PREQ");
            }
        }
    } else {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Platform reset asserted");
#endif
        *event = ASD_EVENT_PLRSTASSERT;
    }

    return result;
}

STATUS on_prdy_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;

    if (g_gpios_evt[JTAG_PRDY_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "CPU_PRDY Asserted Event Detected.");
#endif
    *event = ASD_EVENT_PRDY_EVENT;

    pthread_mutex_lock(&triggered_mutex);
    g_gpios_evt[JTAG_PRDY_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    if (state->event_cfg.break_all) {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "BreakAll detected PRDY, asserting PREQ");
#endif
        write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 1, &result);
        if (result != ST_OK) {
            ASD_log(ASD_LogLevel_Error, stream, option, "Failed to assert PREQ");
        }
        else if (!state->event_cfg.reset_break) {
            usleep(PREQ_WAIT_US);
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "CPU_PRDY, de-asserting PREQ");
#endif
            write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 0, &result);
            if (result != ST_OK) {
                ASD_log(ASD_LogLevel_Error, stream, option, "Failed to deassert PREQ");
            }
        }
    }

    return result;
}

STATUS on_xdp_present_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    (void)state; /* unused */

    if (g_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return ST_OK;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "XDP Present state change detected");
#endif
    *event = ASD_EVENT_XDP_PRESENT;

    pthread_mutex_lock(&triggered_mutex);
    g_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    return ST_OK;
}

STATUS platform_init(Target_Control_Handle* state)
{
    strcpy_s(state->gpios[BMC_TCK_MUX_SEL].name,
             sizeof(state->gpios[BMC_TCK_MUX_SEL].name), FM_JTAG_TCK_MUX_SEL);
    state->gpios[BMC_TCK_MUX_SEL].number = 1;
    state->gpios[BMC_TCK_MUX_SEL].fd = BMC_TCK_MUX_SEL;

    strcpy_s(state->gpios[BMC_PREQ_N].name,
             sizeof(state->gpios[BMC_PREQ_N].name), FM_BMC_PREQ_N);
    state->gpios[BMC_PREQ_N].number = 1;
    state->gpios[BMC_PREQ_N].fd = BMC_PREQ_N;
    state->gpios[BMC_PREQ_N].active_low = true;

    strcpy_s(state->gpios[BMC_PRDY_N].name,
             sizeof(state->gpios[BMC_PRDY_N].name), IRQ_BMC_PRDY_N);
    state->gpios[BMC_PRDY_N].number = 1;
    state->gpios[BMC_PRDY_N].fd = BMC_PRDY_N;
    state->gpios[BMC_PRDY_N].active_low = true;

    strcpy_s(state->gpios[BMC_JTAG_SEL_N].name,
             sizeof(state->gpios[BMC_JTAG_SEL_N].name), FM_JTAG_BMC_MUX_SEL);
    state->gpios[BMC_JTAG_SEL_N].number = 1;
    state->gpios[BMC_JTAG_SEL_N].fd = BMC_JTAG_SEL_N;
    state->gpios[BMC_JTAG_SEL_N].active_low = true;

    strcpy_s(state->gpios[BMC_CPU_PWRGD].name,
             sizeof(state->gpios[BMC_CPU_PWRGD].name), FM_CPU0_PWRGD);
    state->gpios[BMC_CPU_PWRGD].number = 1;
    state->gpios[BMC_CPU_PWRGD].fd = BMC_CPU_PWRGD;

    strcpy_s(state->gpios[BMC_PLTRST_B].name,
             sizeof(state->gpios[BMC_PLTRST_B].name), RST_PLTRST_BMC_N);
    state->gpios[BMC_PLTRST_B].number = 1;
    state->gpios[BMC_PLTRST_B].fd = BMC_PLTRST_B;
    state->gpios[BMC_PLTRST_B].active_low = true;

    strcpy_s(state->gpios[BMC_SYSPWROK].name,
             sizeof(state->gpios[BMC_SYSPWROK].name), PWRGD_SYS_PWROK);
    state->gpios[BMC_SYSPWROK].number = 1;
    state->gpios[BMC_SYSPWROK].fd = BMC_SYSPWROK;

    strcpy_s(state->gpios[BMC_PWR_DEBUG_N].name,
             sizeof(state->gpios[BMC_PWR_DEBUG_N].name), FM_BMC_CPU_PWR_DEBUG_N);
    state->gpios[BMC_PWR_DEBUG_N].number = 1;
    state->gpios[BMC_PWR_DEBUG_N].fd = BMC_PWR_DEBUG_N;
    state->gpios[BMC_PWR_DEBUG_N].active_low = true;

    strcpy_s(state->gpios[BMC_XDP_PRST_IN].name,
             sizeof(state->gpios[BMC_XDP_PRST_IN].name), DBP_PRESENT_N);
    state->gpios[BMC_XDP_PRST_IN].number = 1;
    state->gpios[BMC_XDP_PRST_IN].fd = BMC_XDP_PRST_IN;
    state->gpios[BMC_XDP_PRST_IN].active_low = true;

    return ST_OK;
}
