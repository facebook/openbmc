/*
Copyright (c) 2019, Intel Corporation

 
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

#include "jtag_handler.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include "mem_helper.h"

#if 0
static const char* JtagStatesString[] = {
    "TLR",   "RTI",   "SelDR", "CapDR", "ShfDR", "Ex1DR", "PauDR", "Ex2DR",
    "UpdDR", "SelIR", "CapIR", "ShfIR", "Ex1IR", "PauIR", "Ex2IR", "UpdIR"};
#endif
enum {
  JTAG_TARGET_CPU = 0,
  JTAG_TARGET_CPLD
};

#ifdef JTAG_LEGACY_DRIVER
STATUS JTAG_clock_cycle(int handle, unsigned char tms, unsigned char tdi);
#endif
STATUS perform_shift(JTAG_Handler* state, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     JtagStates current_tap_state,
                     JtagStates end_tap_state);

void initialize_jtag_chains(JTAG_Handler* state)
{
    for (int i = 0; i < MAX_SCAN_CHAINS; i++)
    {
        state->chains[i].shift_padding.drPre = 0;
        state->chains[i].shift_padding.drPost = 0;
        state->chains[i].shift_padding.irPre = 0;
        state->chains[i].shift_padding.irPost = 0;
        state->chains[i].tap_state = JtagTLR;
        state->chains[i].scan_state = JTAGScanState_Done;
    }
}

static STATUS JTAG_set_mux(scanChain chain)
{
    gpio_value_t value = (chain == SCAN_CHAIN_0) ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;

    if (gpio_set_value_by_shadow("FM_JTAG_TCK_MUX_SEL", value)) {
        syslog(LOG_ERR, "Failed to set FM_JTAG_TCK_MUX_SEL as %d", value);
        return ST_ERR;
    }

    return ST_OK;
}

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru)
{
    scanChain chain;
    JTAG_Handler* state = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    if (state == NULL)
    {
        return NULL;
    }

    chain = (fru & 0x80) ? SCAN_CHAIN_1 : SCAN_CHAIN_0;
    state->active_chain = &state->chains[chain];
    initialize_jtag_chains(state);
    state->sw_mode = true;
    memset(state->padDataOne, ~0, sizeof(state->padDataOne));
    memset(state->padDataZero, 0, sizeof(state->padDataZero));
    state->JTAG_driver_handle = -1;

    for (unsigned int i = 0; i < MAX_WAIT_CYCLES; i++)
    {
        state->bitbang_data[i].tms = 0;
        state->bitbang_data[i].tdi = 0;
    }

    if (JTAG_set_mux(chain) != ST_OK) {
      syslog(LOG_ERR, "Setting TCK_MUX failed\n");
    }

    return state;
}

#ifdef JTAG_LEGACY_DRIVER
STATUS JTAG_clock_cycle(int handle, unsigned char tms, unsigned char tdi)
{
    struct tck_bitbang bitbang = {0};

    bitbang.tms = tms;
    bitbang.tdi = tdi;

    if (ioctl(handle, AST_JTAG_BITBANG, &bitbang) < 0)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "ioctl AST_JTAG_BITBANG failed");
        return ST_ERR;
    }
    return ST_OK;
}
#endif

static STATUS JTAG_set_target(int target)
{
    gpio_value_t expected_value, value = GPIO_VALUE_INVALID;
    gpio_desc_t *gpio;
    STATUS ret = ST_ERR;
    const char *mux_sel_gpios[2] = {
      "JTAG_MUX_SEL_0",
      "JTAG_MUX_SEL_1"
    };
    const unsigned int sel_values[2] = {
      2, // CPU:  JTAG_MUX_SEL_1 = High,  JTAG_MUX_SEL_0 = Low
      3, // CPLD: JTAG_MUX_SEL_1 = High,  JTAG_MUX_SEL_0 = High
    };
    unsigned int mux_sel_value = sel_values[target];

    /* To have the JTAG master communicating to the CPU or CPLD */
    if (NULL == (gpio = gpio_open_by_shadow("FM_MODULAR_ASD_EN"))) {
      syslog(LOG_ERR, "Failed to open FM_MODULAR_ASD_EN\n");
      return ST_ERR;
    }

    expected_value = GPIO_VALUE_HIGH;
    gpio_get_value(gpio, &value);
    if (value != expected_value) {
      if (gpio_set_direction(gpio, GPIO_DIRECTION_OUT)) {
        syslog(LOG_ERR, "Failed to set FM_MODULAR_ASD_EN as an output\n");
        goto bail;
      }

      gpio_set_value(gpio, expected_value);
      gpio_get_value(gpio, &value);
      if (value != expected_value) {
        syslog(LOG_WARNING, "Writing %d to FM_MODULAR_ASD_EN failed! ASD is most probably disabled\n",
               expected_value);
        gpio_set_direction(gpio, GPIO_DIRECTION_IN);
        goto bail;
      }
    }

    if (gpio_set_value_by_shadow_list(mux_sel_gpios, 2, mux_sel_value)) {
      goto bail;
    }
    ret = ST_OK;
bail:
    gpio_close(gpio);
    return ret;
}

STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
#ifndef JTAG_LEGACY_DRIVER
    struct jtag_mode jtag_mode;
#endif

    if (state == NULL)
        return ST_ERR;

    state->sw_mode = sw_mode;

    if (JTAG_set_target(JTAG_TARGET_CPU) != ST_OK) {
      syslog(LOG_ERR, "Setting JTAG to CPU failed!\n");
      return ST_ERR;
    }

//    ASD_log(ASD_LogLevel_Info, stream, option, "JTAG mode set to '%s'.",
//            state->sw_mode ? "software" : "hardware");

#ifdef JTAG_LEGACY_DRIVER
    state->JTAG_driver_handle = open("/dev/jtag", O_RDWR);
#else
    state->JTAG_driver_handle = open("/dev/jtag0", O_RDWR);
#endif
    if (state->JTAG_driver_handle == -1)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "Can't open /dev/jtag, please install driver");
        JTAG_set_target(JTAG_TARGET_CPLD);
        return ST_ERR;
    }

#ifndef JTAG_LEGACY_DRIVER
    jtag_mode.feature = JTAG_XFER_MODE;
    jtag_mode.mode = sw_mode ? JTAG_XFER_SW_MODE : JTAG_XFER_HW_MODE;
    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCMODE, &jtag_mode))
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "Failed JTAG_SIOCMODE to set xfer mode");
        close(state->JTAG_driver_handle);
        state->JTAG_driver_handle = -1;
        JTAG_set_target(JTAG_TARGET_CPLD);
        return ST_ERR;
    }
#endif

    if (JTAG_set_tap_state(state, jtag_tlr) != ST_OK)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "Failed to reset tap state.");
        close(state->JTAG_driver_handle);
        state->JTAG_driver_handle = -1;
        JTAG_set_target(JTAG_TARGET_CPLD);
        return ST_ERR;
    }

    initialize_jtag_chains(state);
    return ST_OK;
}

STATUS JTAG_deinitialize(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;

    close(state->JTAG_driver_handle);
    state->JTAG_driver_handle = -1;

    JTAG_set_target(JTAG_TARGET_CPLD);
    return ST_OK;
}

STATUS JTAG_set_padding(JTAG_Handler* state, const JTAGPaddingTypes padding,
                        const int value)
{
    if (state == NULL)
        return ST_ERR;

    if ((padding == JTAGPaddingTypes_DRPre ||
         padding == JTAGPaddingTypes_DRPost) &&
        value > DRMAXPADSIZE)
    {
        return ST_ERR;
    }

    if ((padding == JTAGPaddingTypes_IRPre ||
         padding == JTAGPaddingTypes_IRPost) &&
        value > IRMAXPADSIZE)
    {
        return ST_ERR;
    }

    if (padding == JTAGPaddingTypes_DRPre)
    {
        state->active_chain->shift_padding.drPre = value;
    }
    else if (padding == JTAGPaddingTypes_DRPost)
    {
        state->active_chain->shift_padding.drPost = value;
    }
    else if (padding == JTAGPaddingTypes_IRPre)
    {
        state->active_chain->shift_padding.irPre = value;
    }
    else if (padding == JTAGPaddingTypes_IRPost)
    {
        state->active_chain->shift_padding.irPost = value;
    }
    else
    {
//        ASD_log(ASD_LogLevel_Error, stream, option, "Unknown padding value: %d",
//                value);
        return ST_ERR;
    }
    return ST_OK;
}

//
// Reset the Tap and wait in idle state
//
STATUS JTAG_tap_reset(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;
    return JTAG_set_tap_state(state, jtag_tlr);
}

//
// Request the TAP to go to the target state
//
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state)
{
    if (state == NULL)
        return ST_ERR;
#ifdef JTAG_LEGACY_DRIVER
    struct tap_state_param params;
    params.mode = state->sw_mode ? SW_MODE : HW_MODE;
    params.from_state = state->active_chain->tap_state;
    params.to_state = tap_state;
#else
    struct jtag_tap_state tap_state_t;
    tap_state_t.endstate = tap_state;
    tap_state_t.reset =
        (tap_state == JtagTLR) ? JTAG_FORCE_RESET : JTAG_NO_RESET;
    tap_state_t.tck = 1;
#endif

#ifdef JTAG_LEGACY_DRIVER
    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SET_TAPSTATE, &params)
#else
    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCSTATE, &tap_state_t)
#endif
        < 0)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "ioctl AST_JTAG_SET_TAPSTATE failed");
        return ST_ERR;
    }

    state->active_chain->tap_state = tap_state;

    if ((tap_state == JtagRTI) || (tap_state == JtagPauDR))
        if (JTAG_wait_cycles(state, 5) != ST_OK)
            return ST_ERR;

//    ASD_log(ASD_LogLevel_Info, stream, option, "Goto state: %s (%d)",
//            tap_state >=
//                    (sizeof(JtagStatesString) / sizeof(JtagStatesString[0]))
//                ? "Unknown"
//                : JtagStatesString[tap_state],
//            tap_state);
    return ST_OK;
}

//
// Retrieve the current the TAP state
//
STATUS JTAG_get_tap_state(JTAG_Handler* state, JtagStates* tap_state)
{
    if (state == NULL || tap_state == NULL)
        return ST_ERR;
    *tap_state = state->active_chain->tap_state;
    return ST_OK;
}

//
//  Optionally write and read the requested number of
//  bits and go to the requested target state
//
STATUS JTAG_shift(JTAG_Handler* state, unsigned int number_of_bits,
                  unsigned int input_bytes, unsigned char* input,
                  unsigned int output_bytes, unsigned char* output,
                  JtagStates end_tap_state)
{
    if (state == NULL)
        return ST_ERR;

    unsigned int preFix = 0;
    unsigned int postFix = 0;
    unsigned char* padData = NULL;
    JtagStates current_state;
    JTAG_get_tap_state(state, &current_state);

    if (current_state == JtagShfIR)
    {
        preFix = state->active_chain->shift_padding.irPre;
        postFix = state->active_chain->shift_padding.irPost;
        padData = state->padDataOne;
    }
    else if (current_state == JtagShfDR)
    {
        preFix = state->active_chain->shift_padding.drPre;
        postFix = state->active_chain->shift_padding.drPost;
        padData = state->padDataZero;
    }
    else
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "Shift called but the tap is not in a ShiftIR/DR tap state");
        return ST_ERR;
    }

    if (state->active_chain->scan_state == JTAGScanState_Done)
    {
        state->active_chain->scan_state = JTAGScanState_Run;
        if (preFix)
        {
            if (perform_shift(state, preFix, IRMAXPADSIZE / 8, padData, 0, NULL,
                              current_state, current_state) != ST_OK)
                return ST_ERR;
        }
    }

    if ((postFix) && (current_state != end_tap_state))
    {
        state->active_chain->scan_state = JTAGScanState_Done;
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, current_state,
                          current_state) != ST_OK)
            return ST_ERR;
        if (perform_shift(state, postFix, IRMAXPADSIZE / 8, padData, 0, NULL,
                          current_state, end_tap_state) != ST_OK)
            return ST_ERR;
    }
    else
    {
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, current_state,
                          end_tap_state) != ST_OK)
            return ST_ERR;
        if (current_state != end_tap_state)
        {
            state->active_chain->scan_state = JTAGScanState_Done;
        }
    }
    return ST_OK;
}

//
//  Optionally write and read the requested number of
//  bits and go to the requested target state
//
STATUS perform_shift(JTAG_Handler* state, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     JtagStates current_tap_state,
                     JtagStates end_tap_state)
{
#ifdef JTAG_LEGACY_DRIVER
    struct scan_xfer scan_xfer;
    scan_xfer.mode = state->sw_mode ? SW_MODE : HW_MODE;
    scan_xfer.tap_state = current_tap_state;
    scan_xfer.length = number_of_bits;
    scan_xfer.tdi_bytes = input_bytes;
    scan_xfer.tdi = input;
    scan_xfer.tdo_bytes = output_bytes;
    scan_xfer.tdo = output;
    scan_xfer.end_tap_state = end_tap_state;

    if (ioctl(state->JTAG_driver_handle, AST_JTAG_READWRITESCAN, &scan_xfer) <
        0)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "ioctl AST_JTAG_READWRITESCAN failed.");
        return ST_ERR;
    }
#else
    struct jtag_xfer xfer;

    unsigned char tdio[MAX_DATA_SIZE];

    xfer.endstate = end_tap_state;
    xfer.type =
        (current_tap_state == JtagShfIR) ? JTAG_SIR_XFER : JTAG_SDR_XFER;
    xfer.length = number_of_bits;
    xfer.direction = JTAG_READ_WRITE_XFER;

    if (output != NULL)
    {
        if (memcpy_safe(output, output_bytes, input,
                        DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE)))
        {
//            ASD_log(ASD_LogLevel_Error, stream, option,
//                    "memcpy_safe: input to output copy buffer failed.");
        }
        xfer.tdio = (__u64)(uintptr_t)output;
    }
    else
    {
        if (memcpy_safe(tdio, DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE),
                        input, DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE)))
        {
//            ASD_log(ASD_LogLevel_Error, stream, option,
//                    "memcpy_safe: input to tdio buffer copy failed.");
        }
        xfer.tdio = (__u64)(uintptr_t)tdio;
    }
    if (ioctl(state->JTAG_driver_handle, JTAG_IOCXFER, &xfer) < 0)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "ioctl JTAG_IOCXFER failed");
        return ST_ERR;
    }
#endif

    state->active_chain->tap_state = end_tap_state;

#ifdef ENABLE_DEBUG_LOGGING
    if (input != NULL)
        ASD_log_shift(ASD_LogLevel_Debug, stream, option, number_of_bits,
                      input_bytes, input,
                      (current_tap_state == jtag_shf_dr) ? "Shift DR TDI"
                                                         : "Shift IR TDI");
    if (output != NULL)
        ASD_log_shift(ASD_LogLevel_Debug, stream, option, number_of_bits,
                      output_bytes, output,
                      (current_tap_state == jtag_shf_dr) ? "Shift DR TDO"
                                                         : "Shift IR TDO");
#endif
    return ST_OK;
}

//
// Wait for the requested cycles.
//
// Note: It is the responsibility of the caller to make sure that
// this call is made from RTI, PauDR, PauIR states only. Otherwise
// will have side effects !!!
//
STATUS JTAG_wait_cycles(JTAG_Handler* state, unsigned int number_of_cycles)
{
#ifdef JTAG_LEGACY_DRIVER
    if (state == NULL)
        return ST_ERR;
    if (state->sw_mode)
    {
        for (unsigned int i = 0; i < number_of_cycles; i++)
        {
            if (JTAG_clock_cycle(state->JTAG_driver_handle, 0, 0) != ST_OK)
                return ST_ERR;
        }
    }
#else
    if (state == NULL)
        return ST_ERR;

    if (number_of_cycles > MAX_WAIT_CYCLES)
        return ST_ERR;

    if (state->sw_mode)
    {
        for (unsigned int i = 0; i < number_of_cycles; i++)
        {
            if (ioctl(state->JTAG_driver_handle, JTAG_IOCBITBANG, &state->bitbang_data[i]) < 0)
            {
//                ASD_log(ASD_LogLevel_Error, stream, option,
//                        "ioctl JTAG_IOCBITBANG failed");
                return ST_ERR;
            }
        }
    }
#endif
    return ST_OK;
}

STATUS JTAG_set_jtag_tck(JTAG_Handler* state, unsigned int tck)
{
    if (state == NULL)
        return ST_ERR;
#ifdef JTAG_LEGACY_DRIVER
    struct set_tck_param params;
    params.mode = state->sw_mode ? SW_MODE : HW_MODE;
    params.tck = tck;

    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SET_TCK, &params) < 0)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "ioctl AST_JTAG_SET_TCK failed");
        return ST_ERR;
    }
#else
    unsigned int frq = APB_FREQ / tck;

    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCFREQ, &frq) < 0)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option,
//                "ioctl JTAG_SIOCFREQ failed");
        return ST_ERR;
    }
#endif
    return ST_OK;
}

STATUS JTAG_set_active_chain(JTAG_Handler* state, scanChain chain)
{
    if (state == NULL)
        return ST_ERR;

    if (chain < 0 || chain >= MAX_SCAN_CHAINS)
    {
//        ASD_log(ASD_LogLevel_Error, stream, option, "Invalid scan chain.");
        return ST_ERR;
    }

    state->active_chain = &state->chains[chain];

    return ST_OK;
}
