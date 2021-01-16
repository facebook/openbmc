#include "pin_handler.h"

//for socket and pthread
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <openbmc/libgpio.h>

//#include "mem_helper.h"
#include "openbmc/pal.h"

//Set alias
#define BMC_JTAG_SEL_N BMC_RSMRST_B

// PLTRST,  PRDY, XDP_PRESENT
enum  MONITOR_EVENTS {
  JTAG_PLTRST_EVENT = 0,
  JTAG_PRDY_EVENT,
  JTAG_XDP_PRESENT_EVENT,
  JTAG_EVENT_NUM,
};

static void gpio_handle(gpiopoll_pin_t *gp, gpio_value_t last, gpio_value_t curr);
static void *gpio_poll_thread(void *unused);
static struct gpiopoll_config g_gpios[3] = {
  {"RST_PLTRST_BMC_N", "0", GPIO_EDGE_BOTH, gpio_handle, NULL},
  {"IRQ_BMC_PRDY_N", "1", GPIO_EDGE_FALLING, gpio_handle, NULL},
  {"DBP_PRESENT_N", "2", GPIO_EDGE_BOTH, gpio_handle, NULL}
};
static gpiopoll_desc_t *polldesc = NULL;
static bool g_gpios_triggered[JTAG_EVENT_NUM] = {false, false, false};
static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value, STATUS* result) {
  gpio_value_t val = gpio_get_value_by_shadow(gpio.name);

  if (val == GPIO_VALUE_INVALID) {
    *result = ST_ERR;
    return;
  }

  switch (gpio.fd) {
    case BMC_PREQ_N:
    case BMC_PWR_DEBUG_N:
      *value = (val == GPIO_VALUE_LOW) ? 1 : 0;
      break;
    default:
      *value = (val == GPIO_VALUE_HIGH) ? 1 : 0;
      break;
  }

  *result = ST_OK;
}

void write_pin_value(uint8_t fru, Target_Control_GPIO gpio, int value, STATUS* result) {
  gpio_value_t val;

  switch (gpio.fd) {
    case BMC_PREQ_N:
    case BMC_PWR_DEBUG_N:
    case RESET_BTN:
      val = (value) ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;
      break;
    default:
      val = (value) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
      break;
  }

  *result = (!gpio_set_value_by_shadow(gpio.name, val)) ? ST_OK : ST_ERR;
}

void get_pin_events(Target_Control_GPIO gpio, short* events) {
  *events = POLL_GPIO; /*Not used*/
}

static STATUS handle_preq_event(Target_Control_Handle* state, uint8_t fru) {
  STATUS result = ST_OK;
  ASD_log(ASD_LogLevel_Debug, stream, option,
          "BreakAll detected PRDY, asserting PREQ");

  write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 1, &result);
  if (result != ST_OK) {
    ASD_log(ASD_LogLevel_Error, stream, option, "Failed to assert PREQ");
  }
  else if (!state->event_cfg.reset_break) {
    usleep(10000);
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "CPU_PRDY, de-asserting PREQ");
    write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 0, &result);
    if (result != ST_OK) {
      ASD_log(ASD_LogLevel_Error, stream, option, "Failed to de-assert PREQ");
      return ST_ERR;
    }
  }
  return result;
}

STATUS bypass_jtag_message(uint8_t fru, struct asd_message* s_message) {
    return ST_ERR;
}

STATUS pin_hndlr_deinit_asd_gpio(Target_Control_Handle *state) {
    STATUS result = ST_ERR;
    bool is_deinitialized = true;
    gpio_desc_t *gpio;
    int ret;

    ret = -1;
    if ((gpio = gpio_open_by_shadow(state->gpios[BMC_PREQ_N].name))) {
        ret = gpio_set_direction(gpio, GPIO_DIRECTION_IN);
        gpio_close(gpio);
    }
    if (ret) {
        ASD_log(ASD_LogLevel_Error, stream, option, "deinit BMC_PREQ_N failed");
        is_deinitialized = false;
    }

    ret = -1;
    if ((gpio = gpio_open_by_shadow(state->gpios[BMC_DEBUG_EN_N].name))) {
        ret = gpio_set_direction(gpio, GPIO_DIRECTION_IN);
        gpio_close(gpio);
    }
    if (ret) {
        ASD_log(ASD_LogLevel_Error, stream, option, "deinit BMC_DEBUG_EN_N failed");
        is_deinitialized = false;
    }

    write_pin_value(state->fru, state->gpios[BMC_TCK_MUX_SEL], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_TCK_MUX_SEL is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
        "Failed to set BMC_TCK_MUX_SEL");
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

static void gpio_handle(gpiopoll_pin_t *gpio, gpio_value_t last, gpio_value_t curr)
{
  size_t idx;
  const struct gpiopoll_config *cfg = gpio_poll_get_config(gpio);
  if (!cfg) {
    return;
  }
  idx = atoi(cfg->description);
  if (idx < 3) {
    pthread_mutex_lock(&triggered_mutex);
    g_gpios_triggered[idx] = true;
    pthread_mutex_unlock(&triggered_mutex);
  }
}

static void *gpio_poll_thread(void *fru) {
  gpio_poll(polldesc, -1);
  return NULL;
}

STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle *state) {
    int value = 0;
    STATUS result = ST_ERR;
    gpio_desc_t *gpio;
    int ret;

    read_pin_value(state->fru, state->gpios[BMC_XDP_PRST_IN], &value, &result);
    if (result != ST_OK ) {
      ASD_log(ASD_LogLevel_Error, stream, option,
              "Failed check XDP state or XDP not available");
    } else if (value == 0) {
      ASD_log(ASD_LogLevel_Warning, stream, option,
              "XDP Presence Detected");
      return ST_ERR;
    }

    ret = -1;
    if ((gpio = gpio_open_by_shadow(state->gpios[BMC_PREQ_N].name))) {
        ret = gpio_set_init_value(gpio, GPIO_VALUE_HIGH);
        gpio_close(gpio);
    }
    if (!ret) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_PREQ_N is set to 1 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_PREQ_N");
        return ST_ERR;
    }

    ret = -1;
    if ((gpio = gpio_open_by_shadow(state->gpios[BMC_DEBUG_EN_N].name))) {
        ret = gpio_set_init_value(gpio, GPIO_VALUE_LOW);
        gpio_close(gpio);
    }
    if (!ret) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_DEBUG_EN_N is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_DEBUG_EN_N");
        return ST_ERR;
    }

    write_pin_value(state->fru, state->gpios[BMC_TCK_MUX_SEL], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_TCK_MUX_SEL is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_TCK_MUX_SEL");
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
        polldesc = gpio_poll_open(g_gpios, 3);
        if (!polldesc) {
          return ST_ERR;
        }
        pthread_create(&poll_thread, NULL, gpio_poll_thread, (void *)state->fru);
        gpios_polling = true;
    } else {
        pthread_mutex_lock(&triggered_mutex);
        g_gpios_triggered[JTAG_PLTRST_EVENT] = false;
        g_gpios_triggered[JTAG_PRDY_EVENT] = false;
        g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = false;
        pthread_mutex_unlock(&triggered_mutex);
    }

    return ST_OK;
}

static STATUS
on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    int value;
    static bool is_asserted = false;

    if ( g_gpios_triggered[JTAG_PLTRST_EVENT] == true ) {
        ASD_log(ASD_LogLevel_Debug, stream, option, "Platform reset asserted");
        *event = ASD_EVENT_PLRSTASSERT;
        if (state->event_cfg.reset_break)
        {
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "ResetBreak detected PLT_RESET "
                    "assert, asserting PREQ");
            write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 1, &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to assert PREQ");
            }
        }
        is_asserted = true;
    } else {
        if (is_asserted != true) *event = ASD_EVENT_NONE;
        else {
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "Platform reset de-asserted");
            *event = ASD_EVENT_PLRSTDEASSRT;
            is_asserted = false;
        }
    }

    return result;
}

static STATUS
on_prdy_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    *event = ASD_EVENT_NONE;

    if ( g_gpios_triggered[JTAG_PRDY_EVENT] == false ) {
        return result;
    }

    pthread_mutex_lock(&triggered_mutex);
    handle_preq_event(state, state->fru);
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "CPU_PRDY Asserted Event Detected.\n");
    g_gpios_triggered[JTAG_PRDY_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);
    *event = ASD_EVENT_PRDY_EVENT;
    return result;
}

static STATUS
on_xdp_present_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    int value;
    (void)state; /* unused */

    *event = ASD_EVENT_NONE;

    if ( g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] == false ) {
        return result;
    }

    *event = ASD_EVENT_XDP_PRESENT;
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "XDP Present state change detected");

    pthread_mutex_lock(&triggered_mutex);
    g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = false;
    pthread_mutex_unlock(&triggered_mutex);

    return result;
}

static STATUS
on_power_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result;
    int value;

    read_pin_value(state->fru, state->gpios[BMC_CPU_PWRGD], &value, &result);
    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to get gpio data for CPU_PWRGD: %d", result);
    }
    else if (value == 1)
    {
        *event = ASD_EVENT_NONE;
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Power fail");
#endif
        *event = ASD_EVENT_PWRFAIL;
    }
    return result;
}

STATUS pin_hndlr_init_target_gpios_attr(Target_Control_Handle *state) {
    for (int i = 0; i < NUM_GPIOS; i++)
    {
        state->gpios[i].number = -1;
        state->gpios[i].fd = -1;
        state->gpios[i].handler = NULL;
        state->gpios[i].type = PIN_NONE;
    }

    strcpy_safe(state->gpios[BMC_TCK_MUX_SEL].name,
                sizeof(state->gpios[BMC_TCK_MUX_SEL].name), "FM_JTAG_TCK_MUX_SEL",
                sizeof("FM_JTAG_TCK_MUX_SEL"));
    state->gpios[BMC_TCK_MUX_SEL].number = 1;
    state->gpios[BMC_TCK_MUX_SEL].fd = BMC_TCK_MUX_SEL;


    strcpy_safe(state->gpios[BMC_PREQ_N].name,
                sizeof(state->gpios[BMC_PREQ_N].name), "FM_BMC_PREQ_N",
                sizeof("FM_BMC_PREQ_N"));
    state->gpios[BMC_PREQ_N].number = 1;
    state->gpios[BMC_PREQ_N].fd = BMC_PREQ_N;

    strcpy_safe(state->gpios[BMC_PRDY_N].name,
                sizeof(state->gpios[BMC_PRDY_N].name), "IRQ_BMC_PRDY_N",
                sizeof("IRQ_BMC_PRDY_N"));
    state->gpios[BMC_PRDY_N].number = 1;
    state->gpios[BMC_PRDY_N].fd = BMC_PRDY_N;

    strcpy_safe(state->gpios[BMC_JTAG_SEL_N].name,
                sizeof(state->gpios[BMC_JTAG_SEL_N].name), "JTAG_MUX_SEL_0",
                sizeof("JTAG_MUX_SEL_0"));
    state->gpios[BMC_JTAG_SEL_N].number = 1;
    state->gpios[BMC_JTAG_SEL_N].fd = BMC_JTAG_SEL_N;

    strcpy_safe(state->gpios[BMC_CPU_PWRGD].name,
                sizeof(state->gpios[BMC_CPU_PWRGD].name), "PWRGD_SYS_PWROK",
                sizeof("PWRGD_SYS_PWROK"));
    state->gpios[BMC_CPU_PWRGD].number = 1;
    state->gpios[BMC_CPU_PWRGD].fd = BMC_CPU_PWRGD;

    strcpy_safe(state->gpios[BMC_PLTRST_B].name,
                sizeof(state->gpios[BMC_PLTRST_B].name), "RST_PLTRST_BMC_N",
                sizeof("RST_PLTRST_BMC_N"));
    state->gpios[BMC_PLTRST_B].number = 1;
    state->gpios[BMC_PLTRST_B].fd = BMC_PLTRST_B;

    strcpy_safe(state->gpios[BMC_SYSPWROK].name,
                sizeof(state->gpios[BMC_SYSPWROK].name), "PWRGD_SYS_PWROK",
                sizeof("PWRGD_SYS_PWROK"));
    state->gpios[BMC_SYSPWROK].number = 1;
    state->gpios[BMC_SYSPWROK].fd = BMC_SYSPWROK;

    strcpy_safe(state->gpios[BMC_PWR_DEBUG_N].name,
                sizeof(state->gpios[BMC_PWR_DEBUG_N].name), "FM_BMC_CPU_PWR_DEBUG_N",
                sizeof("FM_BMC_CPU_PWR_DEBUG_N"));
    state->gpios[BMC_PWR_DEBUG_N].number = 1;
    state->gpios[BMC_PWR_DEBUG_N].fd = BMC_PWR_DEBUG_N;

    strcpy_safe(state->gpios[BMC_DEBUG_EN_N].name,
                sizeof(state->gpios[BMC_DEBUG_EN_N].name), "FM_BMC_DEBUG_ENABLE_N",
                sizeof("FM_BMC_DEBUG_ENABLE_N"));
    state->gpios[BMC_DEBUG_EN_N].number = 1;
    state->gpios[BMC_DEBUG_EN_N].fd = BMC_DEBUG_EN_N;

    strcpy_safe(state->gpios[BMC_XDP_PRST_IN].name,
                sizeof(state->gpios[BMC_XDP_PRST_IN].name), "DBP_PRESENT_N",
                sizeof("DBP_PRESENT_N"));
    state->gpios[BMC_XDP_PRST_IN].number = 1;
    state->gpios[BMC_XDP_PRST_IN].fd = BMC_XDP_PRST_IN;

    strcpy_safe(state->gpios[RESET_BTN].name,
                sizeof(state->gpios[RESET_BTN].name), "RST_BMC_RSTBTN_OUT_R_N",
                sizeof("RST_BMC_RSTBTN_OUT_R_N"));
    state->gpios[RESET_BTN].number = 1;
    state->gpios[RESET_BTN].fd = RESET_BTN;

    state->gpios[BMC_CPU_PWRGD].handler =
        (TargetHandlerEventFunctionPtr)on_power_event;

    state->gpios[BMC_PLTRST_B].handler =
        (TargetHandlerEventFunctionPtr)on_platform_reset_event;

    state->gpios[BMC_PRDY_N].handler =
        (TargetHandlerEventFunctionPtr)on_prdy_event;

    state->gpios[BMC_XDP_PRST_IN].handler =
        (TargetHandlerEventFunctionPtr)on_xdp_present_event;

    return ST_OK;
}

short pin_hndlr_pin_events(Target_Control_GPIO gpio) {
    return POLL_GPIO;
}

STATUS pin_hndlr_read_gpio_event(Target_Control_Handle* state,
                                 struct pollfd poll_fd,
                                 ASD_EVENT* event) {
    //printf("state->gpios[poll_fd.fd].name :%s(%d)\n", state->gpios[poll_fd.fd].name, poll_fd.fd);
    state->gpios[poll_fd.fd].handler(state, event);
    return ST_OK;
}

STATUS pin_hndlr_provide_GPIOs_list(Target_Control_Handle* state, target_fdarr_t* fds,
                      int* num_fds) {
    int index = 0;
    short events = 0;

    // Only monitor 3 pins
    get_pin_events(state->gpios[BMC_PRDY_N], &events);
    if (state->event_cfg.report_PRDY && state->gpios[BMC_PRDY_N].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_PRDY_N].fd;
        (*fds)[index].events = events;
        index++;
    }
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "report_PRDY: %d, fd:%d, events: %d, index:%d\n",
            state->event_cfg.report_PRDY, state->gpios[BMC_PRDY_N].fd, events, index);

    get_pin_events(state->gpios[BMC_PLTRST_B], &events);
    if (state->gpios[BMC_PLTRST_B].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_PLTRST_B].fd;
        (*fds)[index].events = events;
        index++;
    }
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "PLRST: fd:%d, events: %d, index:%d",
            state->gpios[BMC_PLTRST_B].fd, events, index);

    get_pin_events(state->gpios[BMC_XDP_PRST_IN], &events);
    if (state->gpios[BMC_XDP_PRST_IN].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_XDP_PRST_IN].fd;
        (*fds)[index].events = events;
        index++;
    }
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "XDP_PRST_IN: fd:%d, events: %d, index:%d",
            state->gpios[BMC_XDP_PRST_IN].fd, events, index);

    *num_fds = index;
    return ST_OK;
}
