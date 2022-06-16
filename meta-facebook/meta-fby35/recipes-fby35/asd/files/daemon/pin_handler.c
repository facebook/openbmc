#include <pthread.h>
#include <safe_str_lib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <openbmc/libgpio.h>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>

#include "asd_msg.h"
#include "logging.h"

// Set alias
#define BMC_JTAG_SEL BMC_RSMRST_B

// PLTRST, PWRGD, PRDY, XDP_PRESENT
enum MONITOR_EVENTS {
    JTAG_PLTRST_EVENT = 0,
    JTAG_PWRGD_EVENT,
    JTAG_PRDY_EVENT,
    JTAG_XDP_PRESENT_EVENT,
    JTAG_EVENT_NUM,
};

enum {
    ONE_PKT   = 0x2,
    FIRST_SEG = 0x3,
    DATA_SEG  = 0x4,
    LAST_SEG  = 0x5,
    MAX_SIZE  = 244,
    IPMB_JTAG_HEADER = 0xa,  // netfn, cmd, IANA, ID, msg header
};

struct gpio_evt {
    bool triggered;
    gpio_value_t value;
};

static struct gpio_evt g_gpios_evt[JTAG_EVENT_NUM] = {
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID},
    {false, GPIO_VALUE_INVALID}
};
static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

STATUS passthrough_jtag_message(ASD_MSG* state, struct asd_message* s_message) {
    /* Not ready */
    uint8_t fru;
    uint8_t tbuf[MAX_IPMB_RES_LEN] = {0x00}; // IANA ID
    uint8_t rbuf[8] = {0x00};
    uint8_t rlen = 0;
    uint16_t tlen = 3;
    bool seg_transfer = false;
    int size;

    // Fill the IANA ID
    memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);

    if (state == NULL || s_message == NULL) {
        return ST_ERR;
    }

    fru = state->target_handler->fru;
    ASD_log(ASD_LogLevel_Error, stream, option,
            "passthrough_jtag_message start, slot%u", fru);

    size = ((s_message->header.size_msb & 0x1F) << 8) | (s_message->header.size_lsb & 0xFF);
    int xfer_cnt = size / MAX_SIZE;
    if ((size % MAX_SIZE) != 0) xfer_cnt++;

    // start
    int cnt = 0;
    int offset = 0;
    while (cnt < xfer_cnt) {
        if (cnt == 0) {
            // copy hdr
            memcpy(&tbuf[4], &s_message->header, sizeof(s_message->header));
            if (xfer_cnt == 1) {
                tbuf[3] = ONE_PKT;
                memcpy(&tbuf[8], s_message->buffer, size); // copy all data
                tlen = size + 8;
            } else {
                tbuf[3] = FIRST_SEG;
                memcpy(&tbuf[8], &s_message->buffer[offset], MAX_SIZE); // copy portion data
                tlen = MAX_SIZE + 8;
            }
        } else if (cnt == (xfer_cnt -1)) {
            tbuf[3] = LAST_SEG;
            memcpy(&tbuf[4], &s_message->buffer[offset], size);
            tlen = size + 4;
        } else {
            tbuf[3] = DATA_SEG;
            memcpy(&tbuf[4], &s_message->buffer[offset], MAX_SIZE);
            tlen = MAX_SIZE + 4;
        }
#if 0
        printf("[%d]Send: ", tlen);
        for (int i = 0; i < tlen; i++) {
            printf("%02X ", tbuf[i]);
        }
        printf("\n");
#endif
        if (bic_ipmb_wrapper(fru, NETFN_OEM_1S_REQ, CMD_OEM_1S_ASD_MSG_OUT, tbuf, tlen, rbuf, &rlen) < 0) {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "passthrough_jtag_message failed, slot%u", fru);
            return ST_ERR;
        }
        cnt++;
        size -= MAX_SIZE;
        offset += MAX_SIZE;
    }

    return ST_OK;
}

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value, STATUS* result) {
    uint8_t val = 0;

    if (gpio.number < 0) {
        *value = 0;
        *result = ST_OK;
        return;
    }

    if (bic_get_one_gpio_status(fru, gpio.number, &val)) {
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
    uint8_t val;

    if (gpio.number < 0 ||
        gpio.number == PWRGD_SYS_PWROK) {  // to avoid pulling PWRGD_SYS_PWROK pin
        *result = ST_OK;
        return;
    }

    if (gpio.active_low) {
        val = (value) ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;
    } else {
        val = (value) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    }

    *result = (!bic_set_gpio(fru, gpio.number, val)) ? ST_OK : ST_ERR;
}

void get_pin_events(Target_Control_GPIO gpio, short* events) {
    *events = POLL_GPIO; /*Not used*/
}

static void *gpio_poll_thread(void *fru) {
    int sock, msgsock, n, len, gpio_pin, ret = 0;
    size_t t;
    struct sockaddr_un server, client;
    uint8_t req_buf[256] = {0};

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ASD_BIC: socket() failed\n");
        exit(1);
    }

    printf("fru%d gpio thread started....\n", (int)fru);
    server.sun_family = AF_UNIX;
    sprintf(server.sun_path, "%s_%d", SOCK_PATH_ASD_BIC, (int)fru);
    unlink (server.sun_path);
    len = strlen(server.sun_path) + sizeof(server.sun_family);
    if (bind(sock, (struct sockaddr *)&server, len) == -1) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ASD_BIC: bind() failed, errno=%d", errno);
        exit(1);
    }

    ASD_log(ASD_LogLevel_Debug, stream, option,
            "ASD_BIC: Socket has name %s", server.sun_path);

    if (listen(sock, 5) == -1) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ASD_BIC: listen() failed, errno=%d", errno);
        exit(1);
    }

    while (1) {
        t = sizeof(client);
        if ((msgsock = accept(sock, (struct sockaddr *)&client, &t)) < 0) {
            ret = errno;
            ASD_log(ASD_LogLevel_Warning, stream, option,
                    "ASD_BIC: accept() failed with ret: %x, errno: %x\n",
                    msgsock, ret);
            sleep(1);
            continue;
        }

        n = recv(msgsock, req_buf, sizeof(req_buf), 0);
        if (n <= 0) {
            ASD_log(ASD_LogLevel_Warning, stream, option,
                    "ASD_BIC: recv() failed with %d\n", n);
            sleep(1);
            continue;
        } else {
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "message received, %d %d", req_buf[0], req_buf[1]);

            gpio_pin = req_buf[0];
            switch (gpio_pin) {
                case RST_PLTRST_BUF_N:
                    pthread_mutex_lock(&triggered_mutex);
                    g_gpios_evt[JTAG_PLTRST_EVENT].value = req_buf[1];
                    g_gpios_evt[JTAG_PLTRST_EVENT].triggered = true;
                    pthread_mutex_unlock(&triggered_mutex);
                    break;
                case PWRGD_CPU_LVC3:
                    pthread_mutex_lock(&triggered_mutex);
                    g_gpios_evt[JTAG_PWRGD_EVENT].value = req_buf[1];
                    g_gpios_evt[JTAG_PWRGD_EVENT].triggered = true;
                    pthread_mutex_unlock(&triggered_mutex);
                    break;
                case H_BMC_PRDY_BUF_N:
                    pthread_mutex_lock(&triggered_mutex);
                    g_gpios_evt[JTAG_PRDY_EVENT].triggered = true;
                    pthread_mutex_unlock(&triggered_mutex);
                    break;
                case FM_DBP_PRESENT_N:
                    pthread_mutex_lock(&triggered_mutex);
                    g_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered = true;
                    pthread_mutex_unlock(&triggered_mutex);
                    break;
                default:
                    ASD_log(ASD_LogLevel_Error, stream, option,
                            "ASD BIC: unknown GPIO pin # received, %d", gpio_pin);
            }
        }
        close(msgsock);
    }

    close(sock);
    pthread_exit(0);
}

STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle *state) {
    STATUS result = ST_ERR;

    write_pin_value(state->fru, state->gpios[BMC_DEBUG_EN_N], 1, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_DEBUG_EN_N is set to 1 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_DEBUG_EN_N");
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

    static bool gpios_polling = false;
    static pthread_t poll_thread;
    if (gpios_polling == false) {
        pthread_create(&poll_thread, NULL, gpio_poll_thread, (void *)state->fru);
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

    write_pin_value(state->fru, state->gpios[BMC_DEBUG_EN_N], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_DEBUG_EN_N is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
        "Failed to set BMC_DEBUG_EN_N");
        is_deinitialized = false;
    }

    write_pin_value(state->fru, state->gpios[BMC_JTAG_SEL], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_JTAG_SEL is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_JTAG_SEL");
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
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Power restored");
#endif
        *event = ASD_EVENT_PWRRESTORE;
    }
    else {
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
            usleep(10000);
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
    STATUS result = ST_OK;
    (void)state; /* unused */

    if (g_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered == false) {
        *event = ASD_EVENT_NONE;
        return result;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "XDP Present state change detected");
#endif
    *event = ASD_EVENT_XDP_PRESENT;

    pthread_mutex_lock(&triggered_mutex);
    g_gpios_evt[JTAG_XDP_PRESENT_EVENT].triggered = false;
    pthread_mutex_unlock(&triggered_mutex);

    return result;
}

STATUS platform_init(Target_Control_Handle* state)
{
    strcpy_s(state->gpios[BMC_TCK_MUX_SEL].name,
             sizeof(state->gpios[BMC_TCK_MUX_SEL].name), "FM_JTAG_TCK_MUX_SEL_R");
    state->gpios[BMC_TCK_MUX_SEL].number = FM_JTAG_TCK_MUX_SEL_R;
    state->gpios[BMC_TCK_MUX_SEL].fd = BMC_TCK_MUX_SEL;

    strcpy_s(state->gpios[BMC_PREQ_N].name,
             sizeof(state->gpios[BMC_PREQ_N].name), "DBP_CPU_PREQ_BIC_N");
    state->gpios[BMC_PREQ_N].number = DBP_CPU_PREQ_BIC_N;
    state->gpios[BMC_PREQ_N].fd = BMC_PREQ_N;
    state->gpios[BMC_PREQ_N].active_low = true;

    strcpy_s(state->gpios[BMC_PRDY_N].name,
             sizeof(state->gpios[BMC_PRDY_N].name), "H_BMC_PRDY_BUF_N");
    state->gpios[BMC_PRDY_N].number = H_BMC_PRDY_BUF_N;
    state->gpios[BMC_PRDY_N].fd = BMC_PRDY_N;
    state->gpios[BMC_PRDY_N].active_low = true;

    strcpy_s(state->gpios[BMC_JTAG_SEL].name,
                sizeof(state->gpios[BMC_JTAG_SEL].name), "BMC_JTAG_SEL_R");
    state->gpios[BMC_JTAG_SEL].number = BMC_JTAG_SEL_R;
    state->gpios[BMC_JTAG_SEL].fd = BMC_JTAG_SEL;

    strcpy_s(state->gpios[BMC_CPU_PWRGD].name,
             sizeof(state->gpios[BMC_CPU_PWRGD].name), "PWRGD_CPU_LVC3");
    state->gpios[BMC_CPU_PWRGD].number = PWRGD_CPU_LVC3;
    state->gpios[BMC_CPU_PWRGD].fd = BMC_CPU_PWRGD;

    strcpy_s(state->gpios[BMC_PLTRST_B].name,
             sizeof(state->gpios[BMC_PLTRST_B].name), "RST_PLTRST_BUF_N");
    state->gpios[BMC_PLTRST_B].number = RST_PLTRST_BUF_N;
    state->gpios[BMC_PLTRST_B].fd = BMC_PLTRST_B;
    state->gpios[BMC_PLTRST_B].active_low = true;

    strcpy_s(state->gpios[BMC_SYSPWROK].name,
             sizeof(state->gpios[BMC_SYSPWROK].name), "PWRGD_SYS_PWROK");
    state->gpios[BMC_SYSPWROK].number = PWRGD_SYS_PWROK;
    state->gpios[BMC_SYSPWROK].fd = BMC_SYSPWROK;

    strcpy_s(state->gpios[BMC_DEBUG_EN_N].name,
             sizeof(state->gpios[BMC_DEBUG_EN_N].name), "FM_BMC_DEBUG_ENABLE_N");
    state->gpios[BMC_DEBUG_EN_N].number = FM_BMC_DEBUG_ENABLE_N;
    state->gpios[BMC_DEBUG_EN_N].fd = BMC_DEBUG_EN_N;
    state->gpios[BMC_DEBUG_EN_N].active_low = true;

    strcpy_s(state->gpios[BMC_XDP_PRST_IN].name,
             sizeof(state->gpios[BMC_XDP_PRST_IN].name), "FM_DBP_PRESENT_N");
    state->gpios[BMC_XDP_PRST_IN].number = FM_DBP_PRESENT_N;
    state->gpios[BMC_XDP_PRST_IN].fd = BMC_XDP_PRST_IN;
    state->gpios[BMC_XDP_PRST_IN].active_low = true;

    return ST_OK;
}
