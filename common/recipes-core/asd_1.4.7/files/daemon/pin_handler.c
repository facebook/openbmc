#define _GNU_SOURCE
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <openbmc/libgpio.h>

#include "logging.h"
#include "target_handler.h"

extern int pin_get_gpio(uint8_t fru, Target_Control_GPIO *gpio);
extern int pin_set_gpio(uint8_t fru, Target_Control_GPIO *gpio, int val);

static gpio_evt m_gpios_evt[JTAG_EVENT_NUM] = {
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID}
};
static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;

static int m_gpios_pipe[JTAG_EVENT_NUM][2];

static const uint8_t evt_pins[JTAG_EVENT_NUM] = {
    BMC_PLTRST_B, BMC_CPU_PWRGD, BMC_PRDY_N, BMC_XDP_PRST_IN
};

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

#ifdef IPMB_JTAG_HNDLR
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

uint8_t pin_gpios[JTAG_EVENT_NUM] = {0};
#else
static void gpio_handle(gpiopoll_pin_t *gpio, gpio_value_t last, gpio_value_t curr)
{
    const struct gpiopoll_config *cfg = gpio_poll_get_config(gpio);
    uint8_t pin_num;
    int idx;

    if (cfg) {
        idx = atoi(cfg->description);
        if (idx >= 0 && idx < JTAG_EVENT_NUM) {
            pthread_mutex_lock(&triggered_mutex);
            m_gpios_evt[idx].value = curr;
            m_gpios_evt[idx].triggered = true;
            pthread_mutex_unlock(&triggered_mutex);

            pin_num = evt_pins[idx];
            if (write(m_gpios_pipe[idx][1], &pin_num, 1) != 1) {
                ASD_log(ASD_LogLevel_Warning, stream, option,
                        "Failed to write pipe[%d]", idx);
            }
        }
    }
}

struct gpiopoll_config pin_gpios[JTAG_EVENT_NUM] = {
    {"", "0", GPIO_EDGE_BOTH, gpio_handle, NULL},
    {"", "1", GPIO_EDGE_BOTH, gpio_handle, NULL},
    {"", "2", GPIO_EDGE_FALLING, gpio_handle, NULL},
    {"", "3", GPIO_EDGE_FALLING, gpio_handle, NULL}
};
#endif

static void *gpio_poll_thread(void *fru)
{
#ifdef IPMB_JTAG_HNDLR
    int sock, msgsock, len;
    size_t slen;
    struct sockaddr_un server, client;
    uint8_t i, buf[8] = {0};

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "gpio_poll_thread: socket() failed");
        exit(1);
    }

    ASD_log(ASD_LogLevel_Warning, stream, option,
            "fru%d gpio thread started", (int)fru);
    server.sun_family = AF_UNIX;
    sprintf(server.sun_path, SOCK_PATH_GPIO_EVT, (int)fru);
    unlink(server.sun_path);
    len = strlen(server.sun_path) + sizeof(server.sun_family);
    if (bind(sock, (struct sockaddr *)&server, len) < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "gpio_poll_thread: bind() failed, errno=%d", errno);
        exit(1);
    }

    ASD_log(ASD_LogLevel_Debug, stream, option,
            "Socket path %s", server.sun_path);

    if (listen(sock, 5) < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "gpio_poll_thread: listen() failed, errno=%d", errno);
        exit(1);
    }

    while (1) {
        slen = sizeof(client);
        if ((msgsock = accept(sock, (struct sockaddr *)&client, &slen)) < 0) {
            ASD_log(ASD_LogLevel_Warning, stream, option,
                    "gpio_poll_thread: accept() failed, errno=%d", errno);
            sleep(1);
            continue;
        }

        len = recv(msgsock, buf, sizeof(buf), 0);
        if (len <= 0) {
            ASD_log(ASD_LogLevel_Warning, stream, option,
                    "gpio_poll_thread: recv() failed with %d", len);
            sleep(1);
        } else {
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "message received, %u %u", buf[0], buf[1]);

            for (i = 0; i < JTAG_EVENT_NUM; i++) {
                if (buf[0] == pin_gpios[i]) {
                    pthread_mutex_lock(&triggered_mutex);
                    m_gpios_evt[i].value = buf[1];
                    m_gpios_evt[i].triggered = true;
                    pthread_mutex_unlock(&triggered_mutex);

                    buf[0] = evt_pins[i];
                    if (write(m_gpios_pipe[i][1], buf, 1) != 1) {
                        ASD_log(ASD_LogLevel_Warning, stream, option,
                                "Failed to write pipe[%u]", i);
                    }
                    break;
                }
            }
        }
        close(msgsock);
    }
    close(sock);
#else
    gpiopoll_desc_t *polldesc = gpio_poll_open(pin_gpios, JTAG_EVENT_NUM);
    if (polldesc) {
        gpio_poll(polldesc, -1);
    }
#endif

    pthread_exit(0);
}

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value,
                    STATUS* result)
{
    int val;

    if (result == NULL) {
        return;
    }
    if (value == NULL) {
        *result = ST_ERR;
        return;
    }

    if (gpio.fd < 0) {
        *value = 0;
        *result = ST_OK;
        return;
    }

    val = pin_get_gpio(fru, &gpio);
    if (val < 0) {
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

void write_pin_value(uint8_t fru, Target_Control_GPIO gpio, int value,
                     STATUS* result)
{
    if (result == NULL) {
        return;
    }

    if (gpio.fd < 0 || gpio.fd == BMC_SYSPWROK) {  // to avoid pulling BMC_SYSPWROK pin
        *result = ST_OK;
        return;
    }

    if (gpio.active_low) {
        value = (value) ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;
    } else {
        value = (value) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    }

    *result = (!pin_set_gpio(fru, &gpio, value)) ? ST_OK : ST_ERR;
}

STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle* state)
{
    STATUS result = ST_ERR;
    static bool gpios_polling = false;
    static pthread_t poll_thread;

    if (state == NULL) {
        return ST_ERR;
    }

    write_pin_value(state->fru, state->gpios[BMC_JTAG_SEL], 1, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_JTAG_SEL is set to 1 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_JTAG_SEL");
        return ST_ERR;
    }

    pthread_mutex_lock(&triggered_mutex);
    m_gpios_evt[JTAG_PLTRST_EVENT].triggered = false;
    m_gpios_evt[JTAG_PWRGD_EVENT].triggered = false;
    m_gpios_evt[JTAG_PRDY_EVENT].triggered = false;
    m_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    if (gpios_polling == false) {
        int i, arg = state->fru;

        for (i = 0; i < JTAG_EVENT_NUM; i++) {
            if (pipe2(m_gpios_pipe[i], O_NONBLOCK) < 0) {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to create pipe[%d]", i);
                return ST_ERR;
            }
        }

        pthread_create(&poll_thread, NULL, gpio_poll_thread, (void *)arg);
        gpios_polling = true;
    }
    state->gpios[BMC_PLTRST_B].fd = m_gpios_pipe[JTAG_PLTRST_EVENT][0];
    state->gpios[BMC_CPU_PWRGD].fd = m_gpios_pipe[JTAG_PWRGD_EVENT][0];
    state->gpios[BMC_PRDY_N].fd = m_gpios_pipe[JTAG_PRDY_EVENT][0];
    state->gpios[BMC_XDP_PRST_IN].fd = m_gpios_pipe[JTAG_XDP_PRESENT_EVENT][0];

    return ST_OK;
}

STATUS pin_hndlr_deinit_asd_gpio(Target_Control_Handle* state)
{
    STATUS result = ST_ERR;

    if (state == NULL) {
        return ST_ERR;
    }

    write_pin_value(state->fru, state->gpios[BMC_JTAG_SEL], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_JTAG_SEL is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_JTAG_SEL");
        return ST_ERR;
    }

    return ST_OK;
}

STATUS on_power_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    int value;

    if (event == NULL) {
        return ST_ERR;
    }

    if (m_gpios_evt[JTAG_PWRGD_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

    pthread_mutex_lock(&triggered_mutex);
    value = m_gpios_evt[JTAG_PWRGD_EVENT].value;
    m_gpios_evt[JTAG_PWRGD_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    if (value == 1)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Power restored");
#endif
        *event = ASD_EVENT_PWRRESTORE;
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

STATUS on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    int value;

    if (event == NULL) {
        return ST_ERR;
    }

    if (m_gpios_evt[JTAG_PLTRST_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

    pthread_mutex_lock(&triggered_mutex);
    value = !m_gpios_evt[JTAG_PLTRST_EVENT].value;
    m_gpios_evt[JTAG_PLTRST_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    if (value == 0)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "Platform reset de-asserted");
#endif
        *event = ASD_EVENT_PLRSTDEASSRT;
        if (state && state->event_cfg.reset_break)
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "ResetBreak detected PLT_RESET "
                    "assert, asserting PREQ");
#endif
            write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 1, &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to assert PREQ");
            }
        }
    }
    else
    {
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

    if (event == NULL) {
        return ST_ERR;
    }

    if (m_gpios_evt[JTAG_PRDY_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "CPU_PRDY Asserted Event Detected.");
#endif
    *event = ASD_EVENT_PRDY_EVENT;

    pthread_mutex_lock(&triggered_mutex);
    m_gpios_evt[JTAG_PRDY_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    if (state && state->event_cfg.break_all)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "BreakAll detected PRDY, asserting PREQ");
#endif
        write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 1, &result);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to assert PREQ");
        }
        else if (!state->event_cfg.reset_break)
        {
            usleep(10000);
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "CPU_PRDY, de-asserting PREQ");
#endif
            write_pin_value(state->fru, state->gpios[BMC_PREQ_N], 0, &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to deassert PREQ");
            }
        }
    }

    return result;
}

STATUS on_xdp_present_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    (void)state; /* unused */

    if (event == NULL) {
        return ST_ERR;
    }

    if (m_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "XDP Present state change detected");
#endif
    *event = ASD_EVENT_XDP_PRESENT;

    pthread_mutex_lock(&triggered_mutex);
    m_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    return result;
}
