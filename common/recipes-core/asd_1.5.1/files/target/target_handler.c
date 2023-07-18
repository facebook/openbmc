/*
Copyright (c) 2021, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "target_handler.h"

#include <fcntl.h>
//#include <gpiod.h>
#include <poll.h>
#include <safe_mem_lib.h>
#include <safe_str_lib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <openbmc/pal.h>

#include "asd_common.h"
//#include "gpio.h"
#include "logging.h"

#define JTAG_CLOCK_CYCLE_MILLISECONDS 1000

static inline void get_pin_events(Target_Control_GPIO gpio, short* events)
{
    *events = POLLIN | POLLPRI;
}

void read_pin_value(uint8_t fru, Target_Control_GPIO gpio, int* value, STATUS* result);
void write_pin_value(uint8_t fru, Target_Control_GPIO gpio, int value, STATUS* result);
STATUS pin_hndlr_init_asd_gpio(Target_Control_Handle* state);
STATUS pin_hndlr_deinit_asd_gpio(Target_Control_Handle* state);

STATUS initialize_gpios(Target_Control_Handle* state);
STATUS deinitialize_gpios(Target_Control_Handle* state);
STATUS on_power_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_prdy_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_xdp_present_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS platform_init(Target_Control_Handle* state);

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

Target_Control_Handle* TargetHandler()
{
    Target_Control_Handle* state =
        (Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));

    if (state == NULL)
        return NULL;

#ifdef HAVE_DBUS
    state->dbus = dbus_helper();
    if (state->dbus == NULL)
    {
        // Continue with target handler creation even without dbus
        ASD_log(ASD_LogLevel_Error, stream, option, "dbus cannot be allocated");
    }
#endif

    state->initialized = false;

    explicit_bzero(&state->gpios, sizeof(state->gpios));

    for (int i = 0; i < NUM_GPIOS; i++)
    {
        state->gpios[i].number = -1;
        state->gpios[i].fd = -1;
        //state->gpios[i].line = NULL;
        //state->gpios[i].chip = NULL;
        state->gpios[i].handler = NULL;
        state->gpios[i].active_low = false;
        state->gpios[i].type = PIN_NONE;
    }

    /*******************************************************************************
        Not all pins are defined for each applicable platform.
        Please see At-Scale Debug Documentation Appendix on Pin Descriptions.
    *******************************************************************************/

    platform_init(state);

    state->event_cfg.break_all = false;
    state->event_cfg.report_MBP = false;
    state->event_cfg.report_PLTRST = false;
    state->event_cfg.report_PRDY = false;
    state->event_cfg.reset_break = false;
    state->xdp_present = false;

    state->gpios[BMC_CPU_PWRGD].handler =
        (TargetEventFunctionPtr)on_power_event;
    state->gpios[BMC_PLTRST_B].handler =
        (TargetEventFunctionPtr)on_platform_reset_event;
    state->gpios[BMC_PRDY_N].handler =
        (TargetEventFunctionPtr)on_prdy_event;
    state->gpios[BMC_XDP_PRST_IN].handler =
        (TargetEventFunctionPtr)on_xdp_present_event;

    // Change is_master_probe accordingly on your BMC implementations.
    // <MODIFY>
    state->is_master_probe = false;
    // </MODIFY>

    return state;
}

STATUS target_initialize(Target_Control_Handle* state, bool xdp_fail_enable)
{
    STATUS result;
    int value = 0;
    if (state == NULL || state->initialized)
        return ST_ERR;

    result = initialize_gpios(state);

    if (result == ST_OK)
    {
        read_pin_value(state->fru, state->gpios[BMC_XDP_PRST_IN], &value, &result);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed check XDP state or XDP not available");
        }
        else if (value == 1)
        {
            state->xdp_present = true;
            if (xdp_fail_enable)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Exiting due XDP presence detected");
                result = ST_ERR;
            }
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "XDP presence detected");
        }
    }

    // specifically drive debug enable to assert
    if (result == ST_OK)
    {
        write_pin_value(state->fru, state->gpios[BMC_DEBUG_EN_N], 1, &result);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to assert debug enable");
        }
    }

#ifdef HAVE_DBUS
    if (result == ST_OK)
    {
        result = dbus_initialize(state->dbus);
    }
#endif

    if (result == ST_OK)
        state->initialized = true;
    else
        deinitialize_gpios(state);

    return result;
}

STATUS initialize_gpios(Target_Control_Handle* state)
{
    STATUS result = ST_OK;

    result = pin_hndlr_init_asd_gpio(state);
    if (result == ST_OK)
        ASD_log(ASD_LogLevel_Info, stream, option,
                "GPIOs initialized successfully");
    else
        ASD_log(ASD_LogLevel_Error, stream, option,
                "GPIOs initialization failed");
    return result;
}

STATUS target_deinitialize(Target_Control_Handle* state)
{
    if (state == NULL || !state->initialized)
        return ST_ERR;

#ifdef HAVE_DBUS
    if (state->dbus != NULL)
    {
        dbus_deinitialize(state->dbus);
        free(state->dbus);
        state->dbus = NULL;
    }
#endif

    return deinitialize_gpios(state);
}

STATUS deinitialize_gpios(Target_Control_Handle* state)
{
    STATUS result = ST_OK;

    result = pin_hndlr_deinit_asd_gpio(state);
    ASD_log(ASD_LogLevel_Info, stream, option,
            (result == ST_OK) ? "GPIOs deinitialized successfully"
                              : "GPIOs deinitialized failed");
    return result;
}

STATUS target_event(Target_Control_Handle* state, struct pollfd poll_fd,
                    ASD_EVENT* event)
{
    STATUS result = ST_OK;
    uint8_t pin_num = 0;

    if (state == NULL || !state->initialized || event == NULL)
        return ST_ERR;

    *event = ASD_EVENT_NONE;

    if (poll_fd.revents & (POLLIN | POLLPRI))
    {
        if ((read(poll_fd.fd, &pin_num, sizeof(pin_num)) == sizeof(pin_num)) &&
            (pin_num < NUM_GPIOS) && state->gpios[pin_num].handler)
        {
            result = state->gpios[pin_num].handler(state, event);
        }
    }

    return result;
}

STATUS target_write(Target_Control_Handle* state, const Pin pin,
                    const bool assert)
{
    STATUS result = ST_OK;
    Target_Control_GPIO gpio;
    int value;

    if (state == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_write, null or uninitialized state");
        return ST_ERR;
    }
    switch (pin)
    {
        case PIN_RESET_BUTTON:
            ASD_log(ASD_LogLevel_Info, stream, option,
                    "Pin Write: %s reset button",
                    assert ? "assert" : "deassert");
            if (result == ST_OK && assert == true)
            {
                ASD_log(ASD_LogLevel_Info, stream, option, "dbus_power_reset");
                result = pal_set_server_power(state->fru, SERVER_POWER_RESET);
            }
            break;
        case PIN_POWER_BUTTON:
            ASD_log(ASD_LogLevel_Info, stream, option,
                    "Pin Write: %s power button",
                    assert ? "assert" : "deassert");
            if (assert)
            {
                if (result == ST_OK)
                {
                    gpio = state->gpios[BMC_CPU_PWRGD];
                    read_pin_value(state->fru, gpio, &value, &result);
#ifdef HAVE_DBUS
                    if (state->gpios[BMC_CPU_PWRGD].type == PIN_DBUS)
                        result = dbus_get_powerstate(state->dbus, &value);
#endif
                    if (result != ST_OK)
                    {
#ifdef ENABLE_DEBUG_LOGGING
                        ASD_log(ASD_LogLevel_Debug, stream, option,
                                "Failed to read gpio %s %d", gpio.name,
                                gpio.number);
#endif
                    }
                    else
                    {
                        if (value)
                        {
                            result = pal_set_server_power(state->fru, SERVER_POWER_OFF);
                            ASD_log(ASD_LogLevel_Info, stream, option,
                                    "dbus_power_off");
                        }
                        else
                        {
                            result = pal_set_server_power(state->fru, SERVER_POWER_ON);
                            ASD_log(ASD_LogLevel_Info, stream, option,
                                    "dbus_power_on");
                        }
                    }
                }
            }
            break;
        case PIN_PREQ:
        case PIN_TCK_MUX_SELECT:
        case PIN_SYS_PWR_OK:
        case PIN_EARLY_BOOT_STALL:
            gpio = state->gpios[ASD_PIN_TO_GPIO[pin]];
            ASD_log(ASD_LogLevel_Info, stream, option, "Pin Write: %s %s %d",
                    assert ? "assert" : "deassert", gpio.name, gpio.number);
            write_pin_value(state->fru, gpio, (uint8_t)(assert ? 1 : 0), &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to set %s %s %d",
                        assert ? "assert" : "deassert", gpio.name, gpio.number);
            }
            break;
        default:
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "Pin write: unsupported pin '%s'", gpio.name);
#endif
            result = ST_ERR;
            break;
    }

    return result;
}

STATUS target_read(Target_Control_Handle* state, Pin pin, bool* asserted)
{
    STATUS result;
    Target_Control_GPIO gpio;
    int value;
    if (state == NULL || asserted == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_read, null or uninitialized state");
        return ST_ERR;
    }
    *asserted = false;

    switch (pin)
    {
        case PIN_PWRGOOD:
            gpio = state->gpios[ASD_PIN_TO_GPIO[pin]];
            read_pin_value(state->fru, gpio, &value, &result);
#ifdef HAVE_DBUS
            if (state->gpios[BMC_CPU_PWRGD].type == PIN_DBUS)
            {
                result = dbus_get_powerstate(state->dbus, &value);
            }
#endif
            if (result != ST_OK)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option,
                        "Failed to read PIN %s %d", gpio.name, gpio.number);
#endif
            }
            else
            {
                *asserted = (value != 0);
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Info, stream, option,
                        "Pin read: %s powergood",
                        *asserted ? "asserted" : "deasserted");
#endif
            }
            break;
        case PIN_PRDY:
        case PIN_PREQ:
        case PIN_SYS_PWR_OK:
        case PIN_EARLY_BOOT_STALL:
            gpio = state->gpios[ASD_PIN_TO_GPIO[pin]];
            read_pin_value(state->fru, gpio, &value, &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to read gpio %s %d", gpio.name, gpio.number);
            }
            else
            {
                *asserted = (value != 0);
                ASD_log(ASD_LogLevel_Info, stream, option, "Pin read: %s %s %d",
                        *asserted ? "asserted" : "deasserted", gpio.name,
                        gpio.number);
            }
            break;
        default:
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "Pin read: unsupported gpio read for pin: %d", pin);
#endif
            result = ST_ERR;
    }

    return result;
}

STATUS target_write_event_config(Target_Control_Handle* state,
                                 const WriteConfig event_cfg, const bool enable)
{
    STATUS status = ST_OK;
    if (state == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_write_event_config, null or uninitialized state");
        return ST_ERR;
    }

    switch (event_cfg)
    {
        case WRITE_CONFIG_BREAK_ALL:
        {
            if (state->event_cfg.break_all != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "BREAK_ALL %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.break_all = enable;
            }
            break;
        }
        case WRITE_CONFIG_RESET_BREAK:
        {
            if (state->event_cfg.reset_break != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "RESET_BREAK %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.reset_break = enable;
            }
            break;
        }
        case WRITE_CONFIG_REPORT_PRDY:
        {
#ifdef ENABLE_DEBUG_LOGGING
            if (state->event_cfg.report_PRDY != enable)
            {
                ASD_log(ASD_LogLevel_Debug, stream, option, "REPORT_PRDY %s",
                        enable ? "enabled" : "disabled");
            }
#endif
            // do a read to ensure no outstanding prdys are present before
            // wait for prdy is enabled.
            int dummy = 0;
            STATUS retval = ST_ERR;
            read_pin_value(state->fru, state->gpios[BMC_PRDY_N], &dummy, &retval);
            state->event_cfg.report_PRDY = enable;
            break;
        }
        case WRITE_CONFIG_REPORT_PLTRST:
        {
            if (state->event_cfg.report_PLTRST != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "REPORT_PLTRST %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.report_PLTRST = enable;
            }
            break;
        }
        case WRITE_CONFIG_REPORT_MBP:
        {
            if (state->event_cfg.report_MBP != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "REPORT_MBP %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.report_MBP = enable;
            }
            break;
        }
        default:
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Invalid event config %d", event_cfg);
            status = ST_ERR;
        }
    }
    return status;
}

STATUS target_wait_PRDY(Target_Control_Handle* state, const uint8_t log2time)
{
    // The design for this calls for waiting for PRDY or until a timeout
    // occurs. The timeout is computed using the PRDY timeout setting
    // (log2time) and the JTAG TCLK.

    int timeout_ms = 0;
    struct pollfd pfd = {0};
    int poll_result = 0;
    STATUS result = ST_OK;
    short events = 0;

    if (state == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_wait_PRDY, null or uninitialized state");
        return ST_ERR;
    }

    // The timeout for commands that wait for a PRDY pulse is defined to be in
    // uSec, we need to convert to mSec, so we divide by 1000. For
    // values less than 1 ms that get rounded to 0 we need to wait 1ms.
    timeout_ms = (1 << log2time) / JTAG_CLOCK_CYCLE_MILLISECONDS;
    if (timeout_ms <= 0)
    {
        timeout_ms = 1;
    }
    get_pin_events(state->gpios[BMC_PRDY_N], &events);
    pfd.events = events;
    pfd.fd = state->gpios[BMC_PRDY_N].fd;
    poll_result = poll(&pfd, 1, timeout_ms);
    if (poll_result == 0)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "Wait PRDY timed out occurred");
#endif
        // future: we should return something to indicate a timeout
    }
    else if (poll_result > 0)
    {
        if (pfd.revents & events)
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Trace, stream, option,
                    "Wait PRDY complete, detected PRDY");
#endif
        }
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_wait_PRDY poll failed: %d.", poll_result);
        result = ST_ERR;
    }
    return result;
}

STATUS target_get_fds(Target_Control_Handle* state, target_fdarr_t* fds,
                      int* num_fds)
{
    int index = 0;
    short events = 0;

    if (state == NULL || !state->initialized || fds == NULL || num_fds == NULL)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_get_fds, null or uninitialized state");
#endif
        return ST_ERR;
    }

    get_pin_events(state->gpios[BMC_PRDY_N], &events);
    if (state->event_cfg.report_PRDY && state->gpios[BMC_PRDY_N].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_PRDY_N].fd;
        (*fds)[index].events = events;
        index++;
    }

    get_pin_events(state->gpios[BMC_PLTRST_B], &events);
    if (state->gpios[BMC_PLTRST_B].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_PLTRST_B].fd;
        (*fds)[index].events = events;
        index++;
    }

    get_pin_events(state->gpios[BMC_CPU_PWRGD], &events);
    if (state->gpios[BMC_CPU_PWRGD].fd != -1) {
        (*fds)[index].fd = state->gpios[BMC_CPU_PWRGD].fd;
        (*fds)[index].events = events;
        index++;
    }

    get_pin_events(state->gpios[BMC_XDP_PRST_IN], &events);
    if (state->gpios[BMC_XDP_PRST_IN].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_XDP_PRST_IN].fd;
        (*fds)[index].events = events;
        index++;
    }

#ifdef HAVE_DBUS
    if (state->dbus && state->dbus->fd != -1)
    {
        (*fds)[index].fd = state->dbus->fd;
        (*fds)[index].events = POLLIN;
        index++;
    }
#endif

    *num_fds = index;

    return ST_OK;
}

// target_wait_sync - This command will only be issued in a multiple
//   probe configuration where there are two or more TAP masters. The
//   WaitSync command is used to tell all TAP masters to wait until a
//   sync indication is received. The exact flow of sync signaling is
//   implementation specific. Command processing will continue after
//   either the Sync indication is received or the SyncTimeout is
//   reached. The SyncDelay is intended to be used in an implementation
//   where there is a single sync signal routed from a single designated
//   TAP Master to all other TAP Masters. The SyncDelay is used as an
//   implicit means to ensure that all other TAP Masters have reached
//   the WaitSync before the Sync signal is asserted.
//
// Parameters:
//  timeout - the SyncTimeout provides a timeout value to all slave
//    probes. If a slave probe does not receive the sync signal during
//    this timeout period, then a timeout occurs. The value is in
//    milliseconds (Range 0ms - 65s).
//  delay - the SyncDelay is meant to be used in a single master probe
//    sync singal implmentation. Upon receiving the WaitSync command,
//    the probe will delay for the Sync Delay Value before sending
//    the sync signal. This is to ensure that all slave probes
//    have reached WaitSync state prior to the sync being sent.
//    The value is in milliseconds (Range 0ms - 65s).
//
// Returns:
//  ST_OK if operation completed successfully.
//  ST_ERR if operation failed.
//  ST_TIMEOUT if failed to detect sync signal
STATUS target_wait_sync(Target_Control_Handle* state, const uint16_t timeout,
                        const uint16_t delay)
{
    STATUS result = ST_OK;
    if (state == NULL || !state->initialized)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Trace, stream, option,
                "target_wait_sync, null or uninitialized state");
#endif
        return ST_ERR;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "WaitSync(%s) - delay=%u ms - timeout=%u ms",
            state->is_master_probe ? "master" : "slave", delay, timeout);
#endif

    if (state->is_master_probe)
    {
        usleep((__useconds_t)(delay * 1000)); // convert from us to ms
        // Once delay has occurred, send out the sync signal.

        // <MODIFY>
        // hard code a error until code is implemented
        result = ST_ERR;
        // </MODIFY>
    }
    else
    {
        // Wait for sync signal to arrive.
        // timeout if sync signal is not received in the
        // milliseconds provided by the timeout parameter

        // <MODIFY>
        usleep((__useconds_t)(timeout * 1000)); // convert from us to ms
        // hard code a error/timeout until code is implemented
        result = ST_TIMEOUT;
        // when sync is detected, set result to ST_OK
        // </MODIFY>
    }

    return result;
}

STATUS target_get_i2c_i3c_config(bus_options* busopt)
{
    STATUS result = ST_ERR;

    if (busopt == NULL)
        return ST_ERR;

#ifdef HAVE_DBUS
    Dbus_Handle* dbus = dbus_helper();
    if (dbus)
    {
        // Connect to the system bus
        int retcode = sd_bus_open_system(&dbus->bus);
        if (retcode >= 0)
        {
            dbus->fd = sd_bus_get_fd(dbus->bus);
            if (dbus->fd < 0)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "sd_bus_get_fd failed");
            }
            else
            {
                result = dbus_get_platform_bus_config(dbus, busopt);
                if (result != ST_OK)
                {
                    ASD_log(ASD_LogLevel_Error, stream, option,
                            "dbus_get_platform_bus_config failed");
                }
            }
            dbus_deinitialize(dbus);
        }
        free(dbus);
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "failed to get dbus handle");
    }
#endif

#ifdef PLATFORM_IxC_LOCAL_CONFIG
    ASD_log(ASD_LogLevel_Info, stream, option,
            "Using local(override) i2c/i3c bus configuration");

    busopt->enable_i2c = false;
    busopt->enable_i3c = false;

    for (int i = 0; i < MAX_IxC_BUSES; i++)
    {
        busopt->bus_config_type[i] = BUS_CONFIG_NOT_ALLOWED;
        busopt->bus_config_map[i] = 0;
    }
    result = ST_OK;
#endif

#ifdef ENABLE_DEBUG_LOGGING
    for (int i = 0; i < MAX_IxC_BUSES; i++)
    {
        ASD_log(ASD_LogLevel_Debug, stream, option, "Bus %d: %s",
                busopt->bus_config_map[i],
                BUS_CONFIG_TYPE_STRINGS[busopt->bus_config_type[i]]);
    }
#endif
    return result;
}
