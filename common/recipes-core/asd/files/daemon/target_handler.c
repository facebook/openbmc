/*
Copyright (c) 2017, Intel Corporation
 
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

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <openbmc/pal.h>

#include "asd/pin_interface.h"
#include "logging.h"
#include "target_handler.h"

#define JTAG_CLOCK_CYCLE_MICROSEC 1

#ifdef CONFIG_JTAG_MSG_FLOW
extern uint8_t jtag_msg_flow;
#endif

void* worker_thread(void* args);
STATUS checkXDPstate(Target_Control_Handle* state);

Target_Control_Handle* TargetHandler(uint8_t fru, TargetHandlerEventFunctionPtr event_cb)
{
    if (event_cb == NULL)
        return NULL;
    Target_Control_Handle* state = (Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));
   if (state == NULL)
        return NULL;
    state->fru = fru;
    state->event_cb = event_cb;
    state->thread_started = false;
    state->exit_thread = false;
    state->event_cfg.break_all = false;
    state->event_cfg.reset_break = false;
    state->event_cfg.report_PRDY = false;
    state->event_cfg.report_PLTRST = false;
    state->event_cfg.report_MBP = false;
    return state;
}

STATUS target_initialize(Target_Control_Handle* state)
{
    ASD_log(LogType_Debug, "target_initialize()");
    if (state == NULL)
        return ST_ERR;

    state->event_cfg.break_all = false;
    state->event_cfg.reset_break = false;
    state->event_cfg.report_PRDY = false;
    state->event_cfg.report_PLTRST = false;
    state->event_cfg.report_MBP = false;

    if (pin_initialize(state->fru) != ST_OK) {
      ASD_log(LogType_Error, "Failed to initialize pins");
      return ST_ERR;
    }

    if (checkXDPstate(state) != ST_OK) {
        ASD_log(LogType_Error, "Failed check XDP state or XDP not available");
        return ST_ERR;
    }

    if (pthread_mutex_init(&state->write_config_mutex, NULL) != 0) {
        ASD_log(LogType_Error, "Failed to init write config mutex");
        return ST_ERR;
    }

    state->exit_thread = false;
    if (pthread_create(&state->worker_thread, NULL, &worker_thread, state)) {
        ASD_log(LogType_Error, "Error creating thread");
        return ST_ERR;
    }
    state->thread_started = true;

    return ST_OK;
}

STATUS target_deinitialize(Target_Control_Handle* state)
{
    ASD_log(LogType_Debug, "target_deinitialize()");
    if (state == NULL)
        return ST_ERR;
    state->exit_thread = true;
    if (state->thread_started) {
        pthread_join(state->worker_thread, NULL);
        pthread_mutex_destroy(&state->write_config_mutex);
        state->thread_started = false;
    }

    return pin_deinitialize(state->fru);
}

void* worker_thread(void* args)
{
    Target_Control_Handle* state = (Target_Control_Handle*)args;
    STATUS status = ST_OK;
    bool asserted = false;
    bool triggered = false;
    uint8_t last_power_state = true;
    uint8_t curr_power_state = true;
    ASD_log(LogType_Debug, "pin Monitoring Thread Started");

    // Get server power once to set a base line state.
    if (pal_get_server_power(state->fru, &last_power_state)) {
      ASD_log(LogType_Error, "Failed to get server power");
      last_power_state = false;
    }

    while (!state->exit_thread) {
        pthread_mutex_lock(&state->write_config_mutex);

#ifdef CONFIG_JTAG_MSG_FLOW
      if (jtag_msg_flow != JFLOW_BIC) {
#endif
        // Power
        if (!pal_get_server_power(state->fru, &curr_power_state) && 
            last_power_state != curr_power_state) {
            last_power_state = curr_power_state;
            asserted = (bool)curr_power_state;
            if (asserted) {
                ASD_log(LogType_Debug, "Power restored");
                // send info back to the plugin that power was restored
                state->event_cb(PIN_EVENT, ASD_EVENT_PWRRESTORE);
            } else {
                ASD_log(LogType_Debug, "Power fail");
                // send info back to the plugin that power failed
                state->event_cb(PIN_EVENT, ASD_EVENT_PWRFAIL);
            }
        }

        // Platform reset
        status = platform_reset_is_event_triggered(state->fru, &triggered);
        if (status != ST_OK) {
            ASD_log(LogType_Error, "Failed to get pin event status for PLTRST: %d", status);
        } else if (triggered) {
            status = platform_reset_is_asserted(state->fru, &asserted);
            if (status != ST_OK) {
                ASD_log(LogType_Error, "Failed to get pin data for PLTRST: %d", status);
            } else if (asserted) {
                ASD_log(LogType_Debug, "Platform reset asserted");
                state->event_cb(PIN_EVENT, ASD_EVENT_PLRSTASSERT);  // Reset asserted
                if (state->event_cfg.report_PRDY && state->event_cfg.reset_break) {
                    ASD_log(LogType_Debug,
                            "ResetBreak detected PLT_RESET "
                            "assert, asserting PREQ");
                    if (preq_assert(state->fru, true) != ST_OK) {
                        ASD_log(LogType_Error, "Failed to assert PREQ");
                    }
                }
            } else {
                ASD_log(LogType_Debug, "Platform reset de-asserted");
                state->event_cb(PIN_EVENT, ASD_EVENT_PLRSTDEASSRT);  // Reset de-asserted
#ifdef HANDLE_RESET_BREAK_ON_PLATFORM_RESET_DEASSERT
                // OpenIPC handles the Reset Break on Platform Reset Deassert.
                // If we ever need to control it from the BMC (IE: if network performance
                // isn't sufficient), then we would re-enable this code.
                if (state->event_cfg.report_PRDY && state->event_cfg.reset_break) {
                    ASD_log(LogType_Debug,
                            "ResetBreak detected PLT_RESET "
                            "deassert, asserting PREQ again");
                    // wait 10 ms
                    usleep(10000);
                    status = prdy_is_event_triggered(state->fru, &triggered);
                    if (status != ST_OK) {
                        ASD_log(LogType_Error, "Failed to get pin event status for CPU_PRDY: %d", status);
                    } else if (triggered) {
                        ASD_log(LogType_Debug, "CPU_PRDY asserted event cleared.");
                    }
                    // deassert, assert, then again deassert PREQ
                    if (preq_assert(state->fru, false) != ST_OK) {
                        ASD_log(LogType_Error, "Failed to deassert PREQ");
                    }
                    if (preq_assert(state->fru, true) != ST_OK) {
                        ASD_log(LogType_Error, "Failed to assert PREQ");
                    }
                    if (preq_assert(state->fru, false) != ST_OK) {
                        ASD_log(LogType_Error, "Failed to deassert PREQ");
                    }
                }
#endif
            }
        }

        // PRDY Event handling
        if (state->event_cfg.report_PRDY) {
            status = prdy_is_event_triggered(state->fru, &triggered);
            if (status != ST_OK) {
                ASD_log(LogType_Error, "Failed to get gpio event status for CPU_PRDY: %d", status);
            } else if (triggered) {
                ASD_log(LogType_Debug, "CPU_PRDY Asserted Event Detected. Sending PRDY event to plugin");
                state->event_cb(PIN_EVENT, ASD_EVENT_PRDY_EVENT);
                if (state->event_cfg.break_all) {
                    ASD_log(LogType_Debug, "BreakAll detected PRDY, asserting PREQ");
                    if (preq_assert(state->fru, true) != ST_OK) {
                        ASD_log(LogType_Error, "Failed to assert PREQ");
                    }
                    usleep(10000);
                    if (preq_assert(state->fru, false) != ST_OK) {
                        ASD_log(LogType_Error, "Failed to deassert PREQ");
                    }
                }
            }
        }
#ifdef CONFIG_JTAG_MSG_FLOW
      }
#endif

        // XDP present detection - This should go last because it can deinit all the pins.
        status = xdp_present_is_event_triggered(state->fru, &triggered);
        if (status != ST_OK) {
            ASD_log(LogType_Error, "Failed to event status for XDP_PRESENT: %d", status);
        } else if (triggered) {
            ASD_log(LogType_Debug, "XDP Present state change detected");
            if(checkXDPstate(state) != ST_OK) {
                state->exit_thread = true;
                state->thread_started = false;
                pthread_mutex_destroy(&state->write_config_mutex);
                return NULL;
            }
        }

        pthread_mutex_unlock(&state->write_config_mutex);
        usleep(5000);
    }
    return NULL;
}

STATUS target_write(Target_Control_Handle* state, const Pin pin, const bool assert)
{
    STATUS retVal = ST_OK;

    switch (pin) {
        case PIN_EARLY_BOOT_STALL: {
            ASD_log(LogType_Debug, "target_write(): PIN_EARLY_BOOT_STALL");
            ASD_log(LogType_Debug, "Pin Write: %s CPU_PWR_DEBUG",
                    assert ? "assert" : "deassert");
            if (power_debug_assert(state->fru, assert) != ST_OK) {
                ASD_log(LogType_Error, "Failed to set PWR_DEBUG");
                retVal = ST_ERR;
            }
            break;
        }
        case PIN_POWER_BUTTON: {
            ASD_log(LogType_Debug, "target_write(): PIN_POWER_BUTTON");
            ASD_log(LogType_Debug, "Pin Write: %s POWER_BUTTON",
                    assert ? "assert" : "deassert");
            if (pal_set_server_power(state->fru, assert ? 
                  SERVER_POWER_ON : SERVER_POWER_OFF) < 0) {
                ASD_log(LogType_Error, "jtag_socket_svr: Failed to set power state!");
                retVal = ST_ERR;
            }
            break;
        }
        case PIN_RESET_BUTTON: {
            ASD_log(LogType_Debug, "target_write(): PIN_RESET_BUTTON");
            ASD_log(LogType_Debug, "Pin Write: %s RESET_BUTTON",
                    assert ? "assert" : "deassert");
            // Reset doesn't have a de-asserted state; only care if it's been asserted
            if (assert && pal_set_server_power(state->fru, SERVER_POWER_RESET) < 0) {
                ASD_log(LogType_Error, "Failed to reset target!");
                retVal = ST_ERR;
            }
            break;
        }
        case PIN_PREQ: {
            ASD_log(LogType_Debug, "target_write(): PIN_PREQ");
            ASD_log(LogType_Debug, "Pin Write: %s PREQ",
                    assert ? "assert" : "deassert");
            if (preq_assert(state->fru, assert) != ST_OK) {
                ASD_log(LogType_Error, "Failed to set PREQ pin");
                retVal = ST_ERR;
            }
            break;
        }
        default: {
            ASD_log(LogType_Debug, "Pin write: unsupported pin 0x%02x", pin);
            retVal = ST_ERR;
        }
    }

    return retVal;
}

STATUS target_read(Target_Control_Handle* state, const ReadType statusRegister,
                   const uint8_t pin, bool* asserted)
{
    if (asserted == NULL)
        return ST_ERR;
    // the value of the "pin" is passed back via the "asserted" argument
    *asserted = false;

    if (statusRegister == READ_TYPE_PROBE) {
        if (pin == PRDY_EVENT_DETECTED) {
            ASD_log(LogType_Debug, "target_read(): READ_TYPE_PROBE/PRDY_EVENT_DETECTED");
            bool triggered = false;
            pthread_mutex_lock(&state->write_config_mutex);
            STATUS status = prdy_is_event_triggered(state->fru, &triggered);
            if (status != ST_OK) {
                ASD_log(LogType_Error, "Failed to get pin event status for CPU_PRDY: %d", status);
                pthread_mutex_unlock(&state->write_config_mutex);
                return status;
            } else if (triggered) {
                ASD_log(LogType_Debug, "CPU_PRDY state change detected, clearing PRDY event.");
                *asserted = true;
            }
            pthread_mutex_unlock(&state->write_config_mutex);
        } else {
            ASD_log(LogType_Error, "Unknown Probe Status pin.");
        }
    } else if (statusRegister == READ_TYPE_PIN) {
        switch (pin) {
            case PIN_POWER_BUTTON:
            case PIN_PWRGOOD: {  // Power Good
                ASD_log(LogType_Debug, "target_read(): READ_TYPE_PIN/PIN_POWER_BUTTON/PIN_PWRGOOD");
                uint8_t power_state;
                if (pal_get_server_power(state->fru, &power_state)) {
                    ASD_log(LogType_Error, "get target power state failed!");
                    return ST_ERR;
                }
                *asserted = (bool)power_state;
                ASD_log(LogType_Debug, "Pin read: Power Good %s",
                        *asserted ? "asserted" : "deasserted");
                break;
            }
            case PIN_PREQ: {  // PREQ
                ASD_log(LogType_Debug, "target_read(): READ_TYPE_PIN/PIN_PREQ");
                bool pinState = false;
                STATUS status = preq_is_asserted(state->fru, &pinState);
                if (status != ST_OK) {
                    ASD_log(LogType_Error, "preq_is_asserted failed: %d", status);
                    return ST_ERR;
                }
                *asserted = !pinState;  // this pin asserts low.
                ASD_log(LogType_Error, "%s: PRDY read, %d\n",__func__, *asserted);
                ASD_log(LogType_Debug, "Pin read: PREQ %s",
                        *asserted ? "asserted" : "deasserted");
                break;
            }
            case PIN_RESET_BUTTON: {  // Reset Button
                ASD_log(LogType_Debug, "target_read(): READ_TYPE_PIN/PIN_RESET_BUTTON");
                ASD_log(LogType_Debug, "Pin read: Reset Button Not Supported");
                break;
            }
            case PIN_EARLY_BOOT_STALL: {  // Early Boot Stall
                ASD_log(LogType_Debug, "target_read(): READ_TYPE_PIN/PIN_EARLY_BOOT_STALL");
                STATUS status = power_debug_is_asserted(state->fru, asserted);
                if (status != ST_OK) {
                    ASD_log(LogType_Error, "Failed to get state for CPU_PWR_DEBUG: %d", status);
                    return ST_ERR;
                }
                ASD_log(LogType_Debug, "Pin read: Early Boot Stall %s",
                        *asserted ? "asserted" : "deasserted");
                break;
            }
            case PIN_MICROBREAK: {  // MBR - MicroBreak
                ASD_log(LogType_Debug, "Pin read: MBR Not Supported");
                break;
            }
            case PIN_PRDY: {  // PRDY
                bool pinState = false;
                STATUS status = prdy_is_asserted(state->fru, &pinState);
                if (status != ST_OK) {
                    ASD_log(LogType_Error, "get pin data for CPU_PRDY failed: %d", status);
                    return ST_ERR;
                }
                *asserted = !pinState;  // this pin asserts low.
                ASD_log(LogType_Debug, "Pin read: CPU_PRDY %s",
                        *asserted ? "asserted" : "deasserted");
                break;
            }
            default: {
                ASD_log(LogType_Error, "Pin status read: unsupported pin 0x%02x", pin);
                return ST_ERR;
            }
        }
    } else {
        ASD_log(LogType_Debug, "Pin read: unsupported read status register 0x%02x",
                statusRegister);
    }
    return ST_OK;
}

STATUS target_write_event_config(Target_Control_Handle* state, const WriteConfig event_cfg,
                                 const bool enable)
{
    STATUS status = ST_OK;
    pthread_mutex_lock(&state->write_config_mutex);
    switch (event_cfg) {
        case WRITE_CONFIG_BREAK_ALL: {
            ASD_log(LogType_Debug, "BREAK_ALL %s", enable ? "enabled" : "disabled");
            state->event_cfg.break_all = enable;
            break;
        }
        case WRITE_CONFIG_RESET_BREAK: {
            ASD_log(LogType_Debug, "RESET_BREAK %s", enable ? "enabled" : "disabled");
            state->event_cfg.reset_break = enable;
            break;
        }
        case WRITE_CONFIG_REPORT_PRDY: {
            ASD_log(LogType_Debug, "REPORT_PRDY %s", enable ? "enabled" : "disabled");
            if (state->event_cfg.report_PRDY != enable) {
                bool triggered = false;
                status = prdy_is_event_triggered(state->fru, &triggered);
                if (status != ST_OK) {
                    ASD_log(LogType_Error, "Failed to get pin event status for CPU_PRDY: %d", status);
                    pthread_mutex_unlock(&state->write_config_mutex);
                    return status;
                } else if (triggered) {
                    ASD_log(LogType_Debug, "Prior to changing REPORT_PRDY, cleared outstanding PRDY event.");
                }
            }
            state->event_cfg.report_PRDY = enable;
            break;
        }
        case WRITE_CONFIG_REPORT_PLTRST: {
            ASD_log(LogType_Debug, "REPORT_PLTRST %s", enable ? "enabled" : "disabled");
            state->event_cfg.report_PLTRST = enable;
            break;
        }
        case WRITE_CONFIG_REPORT_MBP: {
            ASD_log(LogType_Debug, "REPORT_MBP %s", enable ? "enabled" : "disabled");
            state->event_cfg.report_MBP = enable;
            break;
        }
        default: {
            ASD_log(LogType_Error, "Invalid event config %d", event_cfg);
            status = ST_ERR;
        }
    }
    pthread_mutex_unlock(&state->write_config_mutex);
    return status;
}


STATUS target_wait_PRDY(Target_Control_Handle* state, const uint8_t log2time) {
    // The design for this calls for waiting for PRDY or until a timeout occurs.
    // The timeout is computed using the PRDY timeout setting (log2time) and
    // the JTAG TCLK.
    struct timeval start, end;
    long utime=0, seconds=0, useconds=0;
    // The timeout for commands that wait for a PRDY pulse is defined by the period of
    // the JTAG clock multiplied by 2^log2time.
    int timeout = JTAG_CLOCK_CYCLE_MICROSEC * (1 << log2time);

    gettimeofday(&start, NULL);

    pthread_mutex_lock(&state->write_config_mutex);
    bool detected = false;
    while(1) {
        STATUS status = prdy_is_event_triggered(state->fru, &detected);
        if (status != ST_OK) {
            ASD_log(LogType_Error, "Failed to get_pin_event_status for CPU_PRDY: %d", status);
        } else if (detected) {
            break;
        }
        gettimeofday(&end, NULL);
        seconds  = end.tv_sec  - start.tv_sec;
        useconds = end.tv_usec - start.tv_usec;
        utime = ((seconds) * 1000000 + useconds) + 0.5;

        if (utime >= timeout || timeout <= 4) {
            // If the timeout is <= 4 microseconds, this is such a small period of time where it
            // becomes more reasonable to simply timeout and not sleep. Any sleep on most boards
            // will likely result in a thread context switch totaling far more than the desired
            // timeout value.
            break;
        } else {
            // It should be noted that in reality many boards will sleep longer than the 1ms
            // specified below.
            usleep(1000);
        }
    }
    pthread_mutex_unlock(&state->write_config_mutex);
    if(detected)
        ASD_log(LogType_Debug, "Wait PRDY complete, detected PRDY");
    else
        ASD_log(LogType_Debug, "Wait PRDY timed out after %f ms with timeout setting of %f ms.", (float)utime / 1000, (float)timeout/1000);
    return ST_OK;
}

STATUS checkXDPstate(Target_Control_Handle* state)
{
    STATUS result = ST_OK;
    bool asserted = false;
    result = xdp_present_is_asserted(state->fru, &asserted);
    if (result != ST_OK) {
        ASD_log(LogType_Error, "get state failed for XDP_PRESENT pin: %d", result);
        return result;
    }

    if (asserted) {
        // Probe is connected to the XDP and all pin's need to be set as
        //  inputs so that they do not interfere with the probe and the
        //  BMC JTAG interface needs to be set as a slave device
        if (state->event_cb(XDP_PRESENT_EVENT, 0) != ST_OK) {
            ASD_log(LogType_Error, "Failed to send XDP Present event.");
        }
        return ST_ERR;
    }
    return ST_OK;
}

STATUS target_jtag_chain_select(Target_Control_Handle* state, const uint8_t scan_chain) {
    STATUS status = ST_ERR;
    if (scan_chain == SCAN_CHAIN_0) {
        ASD_log(LogType_Debug, "Scan Chain Select: JTAG");
        status = tck_mux_select_assert(state->fru, false);
    } else if (scan_chain == SCAN_CHAIN_1) {
        ASD_log(LogType_Debug, "Scan Chain Select: PCH");
        status = tck_mux_select_assert(state->fru, true);
    } else {
        ASD_log(LogType_Error, "Scan Chain Select: Unknown Scan Chain -> %d", scan_chain);
    }
    return status;
}
