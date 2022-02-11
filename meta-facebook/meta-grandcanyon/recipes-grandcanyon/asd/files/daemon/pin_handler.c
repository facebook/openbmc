#include "pin_handler.h"

//for socket and pthread
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

//#include "mem_helper.h"
#include "openbmc/pal.h"

//Set alias
#define BMC_JTAG_SEL_N BMC_RSMRST_B

#define SOCK_PATH_LEN      64
#define MAX_SOCK_REQ_LEN   256

// PLTRST,  PRDY, XDP_PRESENT
enum  MONITOR_EVENTS {
  JTAG_PLTRST_EVENT = 0,
  JTAG_PRDY_EVENT,
  JTAG_XDP_PRESENT_EVENT,
  JTAG_EVENT_NUM,
};

static pthread_mutex_t triggered_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_gpios_triggered[JTAG_EVENT_NUM] = {false, false, false};
         
static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value,
                                  STATUS* result) {
  if (value == NULL) {
    ASD_log(ASD_LogLevel_Error, stream, option, 
            "%s(): invalid value parameter\n", __func__);
    return ST_ERR;
  }
  if (result == NULL) {
    ASD_log(ASD_LogLevel_Error, stream, option, 
            "%s(): invalid status result parameter\n", __func__);
    return ST_ERR;
  }

  *value = 0;
  if (bic_get_one_gpio_status(gpio.number, (uint8_t *)value) < 0) {
    *result = ST_ERR;
  } else {
    *result = ST_OK;
  }
}

void write_pin_value(uint8_t fru, Target_Control_GPIO gpio, int value,
                                   STATUS* result) {
  if (result == NULL) {
    ASD_log(ASD_LogLevel_Error, stream, option, 
            "%s(): invalid status result parameter\n", __func__);
    return ST_ERR;
  }
  if (bic_set_gpio(gpio.number, (uint8_t)value) < 0) {
    *result = ST_ERR;
  } else {
    *result = ST_OK;
  }
}

void get_pin_events(Target_Control_GPIO gpio, short* events) {
  if (events == NULL) {
    ASD_log(ASD_LogLevel_Error, stream, option, 
            "%s(): invalid events parameter\n", __func__);
    return ST_ERR;
  }
  *events = POLL_GPIO; /*Not used*/
}

static STATUS handle_preq_event(Target_Control_Handle* state, uint8_t fru) {
  if (state == NULL) {
    ASD_log(ASD_LogLevel_Error, stream, option, 
            "%s(): state shuold not be NULL\n", __func__);
    return ST_ERR;
  }
  STATUS result = ST_OK;
  ASD_log(ASD_LogLevel_Debug, stream, option,
          "BreakAll detected PRDY, asserting PREQ");

  bic_set_gpio(FM_BMC_PREQ_N_NODE_R1, 1);
  if (result != ST_OK)
  {
     ASD_log(ASD_LogLevel_Error, stream, option,
             "Failed to assert PREQ");
  }
  else if ( !state->event_cfg.reset_break )
  {
    usleep(10000);
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "CPU_PRDY, de-asserting PREQ");
    bic_set_gpio(FM_BMC_PREQ_N_NODE_R1, 0);
    if (result != ST_OK)
    {
      ASD_log(ASD_LogLevel_Error, stream, option,
              "Failed to de-assert PREQ");
    }
  }
  return result;
}

STATUS bypass_jtag_message(uint8_t fru, struct asd_message* s_message) {
    enum {
      ONE_PKT   = 0x2,
      FIRST_SEG = 0x3,
      DATA_SEG  = 0x4,
      LAST_SEG  = 0x5,
      MAX_SIZE  = 244,
      IPMB_JTAG_HEADER = 0xa, //netfn, cmd, IANA, ID, msg header 
    };

    /* Not ready */
    uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x9c, 0x9c, 0x00}; // IANA ID
    uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
    uint8_t rlen = 0;
    uint16_t tlen = 3;
    bool seg_transfer = false;
    int size = 0;

    if (s_message == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): bypass message is missing\n", __func__);
      return ST_ERR;
    }

    size = ((s_message->header.size_msb & 0x1F) << 8) | (s_message->header.size_lsb & 0xFF);
    if (size < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "get message size failed %d", size);
        return ST_ERR;
    }

    int xfer_cnt = size / MAX_SIZE;
    if ( (size % MAX_SIZE) != 0 ) xfer_cnt++;

    // start
    int cnt = 0;
    int offset = 0;
    while ( cnt < xfer_cnt ) {
        if ( cnt == 0 ) {
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
        } else if ( cnt == (xfer_cnt -1) ) {
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
        if (bic_ipmb_wrapper(NETFN_OEM_1S_REQ, CMD_OEM_1S_ASD_MSG_OUT, tbuf, tlen, rbuf, &rlen) < 0) {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "passthrough_jtag_message failed");
            return ST_ERR;
        }
        cnt++;
        size -= MAX_SIZE;
        offset += MAX_SIZE;
    }

    return ST_OK;
}

STATUS pin_hndlr_deinit_asd_gpio(Target_Control_Handle *state) {
    STATUS result = ST_ERR;
    bool is_deinitialized = true;

    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    write_pin_value(state->fru, state->gpios[BMC_DEBUG_EN_N], 1, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_DEBUG_EN_N is set to 1 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
        "Failed to set BMC_DEBUG_EN_N");
        is_deinitialized = false;
    }

    write_pin_value(state->fru, state->gpios[BMC_JTAG_SEL_N], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_JTAG_SEL_N is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_JTAG_SEL_N");
        is_deinitialized = false;
    }

    return (is_deinitialized == true)?ST_OK:ST_ERR;
}

static void *gpio_poll_thread(void *fru) {
  int sock = 0, msgsock = 0, n = 0, len = 0, gpio_pin = 0, ret=0;
  size_t t = 0;
  struct sockaddr_un server, client;
  uint8_t req_buf[MAX_SOCK_REQ_LEN] = {0};
  char sock_path[SOCK_PATH_LEN] = {0};

  if (fru == NULL) {
    ASD_log(ASD_LogLevel_Error, stream, option, 
            "%s(): invalid FRUID\n", __func__);
    return ST_ERR;
  }
  if ((sock = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
  {
    ASD_log(ASD_LogLevel_Error, stream, option,
            "ASD_BIC: socket() failed\n");
    exit(1);
  }

  printf("fru%d gpio thread started....\n", (int)fru);
  server.sun_family = AF_UNIX;
  sprintf(sock_path, "%s_%d", SOCK_PATH_ASD_BIC, (int)fru);
  strcpy(server.sun_path, sock_path);
  unlink (server.sun_path);
  len = strlen (server.sun_path) + sizeof(server.sun_family);
  if (bind(sock, (struct sockaddr *) &server, len) == -1)
  {
    ASD_log(ASD_LogLevel_Error, stream, option,
            "ASD_BIC: bind() failed, errno=%d", errno);
    exit(1);
  }

  ASD_log(ASD_LogLevel_Debug, stream, option,
         "ASD_BIC: Socket has name %s", server.sun_path);

  if (listen(sock, 5) == -1)
  {
    ASD_log(ASD_LogLevel_Error, stream, option,
            "ASD_BIC: listen() failed, errno=%d", errno);
    exit(1);
  }

  while (1) {
    t = sizeof (client);
    if ((msgsock = accept(sock, (struct sockaddr *) &client, &t)) < 0) {
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
        pthread_mutex_lock(&triggered_mutex);
        switch (gpio_pin) {
          case RST_PLTRST_BMC_N:
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "ASD_BIC: PLTRST_N event");
            g_gpios_triggered[JTAG_PLTRST_EVENT] = true;
            break;
          case IRQ_BMC_PRDY_NODE_OD_N:
            g_gpios_triggered[JTAG_PRDY_EVENT] = true;
            break;
          case DBP_PRESENT_R2_N:
            g_gpios_triggered[JTAG_XDP_PRESENT_EVENT] = true;
            break;
          default:
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "ASD BIC: unknown GPIO pin # received, %d", gpio_pin);
        }
        pthread_mutex_unlock(&triggered_mutex);
    }
    close(msgsock);
  }
  close(sock);

  return NULL;
}

STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle *state) {
    int value = 0;
    STATUS result = ST_ERR;

    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    read_pin_value(state->fru, state->gpios[BMC_XDP_PRST_IN], &value, &result);
    if (result != ST_OK ) {
      ASD_log(ASD_LogLevel_Error, stream, option,
              "Failed check XDP state or XDP not available");
    } else if (value == 0) {
      ASD_log(ASD_LogLevel_Warning, stream, option,
              "XDP Presence Detected");
      return ST_ERR;
    }

    write_pin_value(state->fru, state->gpios[BMC_DEBUG_EN_N], 0, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_DEBUG_EN_N is set to 0 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_DEBUG_EN_N");
        return ST_ERR;
    }

    write_pin_value(state->fru, state->gpios[BMC_JTAG_SEL_N], 1, &result);
    if (result == ST_OK) {
        ASD_log(ASD_LogLevel_Warning, stream, option,
                "BMC_JTAG_SEL_N is set to 1 successfully");
    } else {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to set BMC_JTAG_SEL_N");
        return ST_ERR;
    }

    static bool gpios_polling = false;
    static pthread_t poll_thread;
    if (gpios_polling == false) {
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
    int value = 0;
    static bool is_asserted = false;

    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    if (event == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): event shuold not be NULL\n", __func__);
      return ST_ERR;
    }

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

    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    if (event == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): event shuold not be NULL\n", __func__);
      return ST_ERR;
    }
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
    int value = 0;
    (void)state; /* unused */

    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    if (event == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): event shuold not be NULL\n", __func__);
      return ST_ERR;
    }

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
    STATUS result = ST_OK;
    int value = 0;

    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    if (event == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): event shuold not be NULL\n", __func__);
      return ST_ERR;
    }
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
    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
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
    state->gpios[BMC_TCK_MUX_SEL].number = FM_JTAG_TCK_MUX_SEL;
    state->gpios[BMC_TCK_MUX_SEL].fd = BMC_TCK_MUX_SEL;


    strcpy_safe(state->gpios[BMC_PREQ_N].name,
                sizeof(state->gpios[BMC_PREQ_N].name), "FM_BMC_PREQ_N_NODE_R1",
                sizeof("FM_BMC_PREQ_N_NODE_R1"));
    state->gpios[BMC_PREQ_N].number = FM_BMC_PREQ_N_NODE_R1;
    state->gpios[BMC_PREQ_N].fd = BMC_PREQ_N;

    strcpy_safe(state->gpios[BMC_PRDY_N].name,
                sizeof(state->gpios[BMC_PRDY_N].name), "IRQ_BMC_PRDY_NODE_OD_N",
                sizeof("IRQ_BMC_PRDY_NODE_OD_N"));
    state->gpios[BMC_PRDY_N].number = IRQ_BMC_PRDY_NODE_OD_N;
    state->gpios[BMC_PRDY_N].fd = BMC_PRDY_N;

    strcpy_safe(state->gpios[BMC_JTAG_SEL_N].name,
                sizeof(state->gpios[BMC_JTAG_SEL_N].name), "FM_BIC_JTAG_SEL_N",
                sizeof("BMC_JTAG_SEL_N"));
    state->gpios[BMC_JTAG_SEL_N].number = BMC_JTAG_SEL;
    state->gpios[BMC_JTAG_SEL_N].fd = BMC_JTAG_SEL_N;

    strcpy_safe(state->gpios[BMC_CPU_PWRGD].name,
                sizeof(state->gpios[BMC_CPU_PWRGD].name), "PWRGD_BMC_PS_PWROK_R",
                sizeof("PWRGD_BMC_PS_PWROK_R"));
    state->gpios[BMC_CPU_PWRGD].number = PWRGD_BMC_PS_PWROK_R;
    state->gpios[BMC_CPU_PWRGD].fd = BMC_CPU_PWRGD;

    strcpy_safe(state->gpios[BMC_PLTRST_B].name,
                sizeof(state->gpios[BMC_PLTRST_B].name), "RST_PLTRST_BMC_N",
                sizeof("RST_PLTRST_BMC_N"));
    state->gpios[BMC_PLTRST_B].number = RST_PLTRST_BMC_N;
    state->gpios[BMC_PLTRST_B].fd = BMC_PLTRST_B;

    strcpy_safe(state->gpios[BMC_SYSPWROK].name,
                sizeof(state->gpios[BMC_SYSPWROK].name), "PWRGD_SYS_PWROK",
                sizeof("PWRGD_SYS_PWROK"));
    state->gpios[BMC_SYSPWROK].number = PWRGD_SYS_PWROK;
    state->gpios[BMC_SYSPWROK].fd = BMC_SYSPWROK;

    strcpy_safe(state->gpios[BMC_PWR_DEBUG_N].name,
                sizeof(state->gpios[BMC_PWR_DEBUG_N].name), "FM_BMC_CPU_PWR_DEBUG_N",
                sizeof("FM_BMC_CPU_PWR_DEBUG_N"));
    state->gpios[BMC_PWR_DEBUG_N].number = FM_BMC_CPU_PWR_DEBUG_N;
    state->gpios[BMC_PWR_DEBUG_N].fd = BMC_PWR_DEBUG_N;

    strcpy_safe(state->gpios[BMC_DEBUG_EN_N].name,
                sizeof(state->gpios[BMC_DEBUG_EN_N].name), "FM_BMC_DEBUG_ENABLE_N",
                sizeof("FM_BMC_DEBUG_ENABLE_N"));
    state->gpios[BMC_DEBUG_EN_N].number = FM_BMC_DEBUG_ENABLE_N;
    state->gpios[BMC_DEBUG_EN_N].fd = BMC_DEBUG_EN_N;

    strcpy_safe(state->gpios[BMC_XDP_PRST_IN].name,
                sizeof(state->gpios[BMC_XDP_PRST_IN].name), "DBP_PRESENT_R2_N",
                sizeof("DBP_PRESENT_R2_N"));
    state->gpios[BMC_XDP_PRST_IN].number = DBP_PRESENT_R2_N;
    state->gpios[BMC_XDP_PRST_IN].fd = BMC_XDP_PRST_IN;

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
    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    if (event == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): event shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    //printf("state->gpios[poll_fd.fd].name :%s(%d)\n", state->gpios[poll_fd.fd].name, poll_fd.fd);
    state->gpios[poll_fd.fd].handler(state, event);
    return ST_OK;
}

STATUS pin_hndlr_provide_GPIOs_list(Target_Control_Handle* state, target_fdarr_t* fds,
                      int* num_fds) {

    int index = 0;
    short events = 0;

    if (state == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): state shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    if (fds == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): no target fd\n", __func__);
      return ST_ERR;
    }
    if (num_fds == NULL) {
      ASD_log(ASD_LogLevel_Error, stream, option, 
              "%s(): fd number shuold not be NULL\n", __func__);
      return ST_ERR;
    }
    // Only monitor 3 pins
    get_pin_events(state->gpios[BMC_PRDY_N], &events);
    // Tony
    if ( state->event_cfg.report_PRDY && state->gpios[BMC_PRDY_N].fd != -1 )
    {
        (*fds)[index].fd = state->gpios[BMC_PRDY_N].fd;
        (*fds)[index].events = events;
        index++;
    }
    ASD_log(ASD_LogLevel_Info, stream, option,
            "report_PRDY: %d, fd:%d, events: %d, index:%d\n",
            state->event_cfg.report_PRDY, state->gpios[BMC_PRDY_N].fd, events, index);

    get_pin_events(state->gpios[BMC_PLTRST_B], &events);
    if (state->gpios[BMC_PLTRST_B].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_PLTRST_B].fd;
        (*fds)[index].events = events;
        index++;
    }
    ASD_log(ASD_LogLevel_Info, stream, option,
            "PLRST: fd:%d, events: %d, index:%d",
            state->gpios[BMC_PLTRST_B].fd, events, index);

    get_pin_events(state->gpios[BMC_XDP_PRST_IN], &events);
    if (state->gpios[BMC_XDP_PRST_IN].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_XDP_PRST_IN].fd;
        (*fds)[index].events = events;
        index++;
    }
    ASD_log(ASD_LogLevel_Info, stream, option,
            "XDP_PRST_IN: fd:%d, events: %d, index:%d",
            state->gpios[BMC_XDP_PRST_IN].fd, events, index);

    *num_fds = index;
    return ST_OK;
}
