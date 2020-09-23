#include "pin_handler.h"

//#include "mem_helper.h"
//#include "openbmc/pal.h"

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

STATUS on_power_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_prdy_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_xdp_present_event(Target_Control_Handle* state, ASD_EVENT* event);

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value,
                                  STATUS* result) {
  *result = ST_ERR;
}

void write_pin_value(uint8_t fru, Target_Control_GPIO gpio, int value,
                                   STATUS* result) {
  *result = ST_ERR;
}

void get_pin_events(Target_Control_GPIO gpio, short* events) {
  *events = POLL_GPIO;
}


STATUS bypass_jtag_message(uint8_t fru, struct asd_message* s_message) {

  if (s_message == NULL) {
    return ST_ERR;
  }

  return ST_ERR;
}

STATUS pin_hndlr_deinit_asd_gpio(Target_Control_Handle *state) {
  return ST_ERR;
}

STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle *state) {
  return ST_ERR;
}

STATUS pin_hndlr_init_target_gpios_attr(Target_Control_Handle *state) {
    for (int i = 0; i < NUM_GPIOS; i++)
    {
        state->gpios[i].number = -1;
        state->gpios[i].fd = -1;
        state->gpios[i].handler = NULL;
        state->gpios[i].type = PIN_NONE;
    }

    return ST_ERR;
}

STATUS
on_power_event(Target_Control_Handle* state, ASD_EVENT* event) {
  STATUS result;
  int value;
  
  read_pin_value(state->fru, state->gpios[BMC_CPU_PWRGD], &value, &result);
  if (result != ST_OK) {
    ASD_log(ASD_LogLevel_Error, stream, option,
            "Failed to get gpio data for CPU_PWRGD: %d", result);
  } else if (value == 1) {
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option, "Power restored");
#endif
    *event = ASD_EVENT_PWRRESTORE; 
  } else {
#ifdef ENABLE_DEBUG_LOGGING
     ASD_log(ASD_LogLevel_Debug, stream, option, "Power fail");    
#endif
    *event = ASD_EVENT_PWRFAIL;
  }
}

STATUS on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result;
    int value;

    read_pin_value(state->fru, state->gpios[BMC_PLTRST_B], &value, &result);
    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to get event status for PLTRST: %d", result);
    }
    else if (value == 1)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Platform reset asserted");
#endif
        *event = ASD_EVENT_PLRSTASSERT;
        if (state->event_cfg.reset_break)
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
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "Platform reset de-asserted");
#endif
        *event = ASD_EVENT_PLRSTDEASSRT;
    }

    return result;
}

STATUS on_prdy_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "CPU_PRDY Asserted Event Detected.");
#endif
    *event = ASD_EVENT_PRDY_EVENT;
    if (state->event_cfg.break_all)
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

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "XDP Present state change detected");
#endif
    *event = ASD_EVENT_XDP_PRESENT;

    return result;
}

short pin_hndlr_pin_events(Target_Control_GPIO gpio) {
    return POLL_GPIO;
}

STATUS pin_hndlr_read_gpio_event(Target_Control_Handle* state, 
                                 struct pollfd poll_fd,
                                 ASD_EVENT* event) {
    return ST_ERR;
}

STATUS pin_hndlr_provide_GPIOs_list(Target_Control_Handle* state, target_fdarr_t* fds,
                      int* num_fds) {

    //the list will be used in process_all_gpio_events
    int index = 0;
    *num_fds = index;
    return ST_OK;
}
