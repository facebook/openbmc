/*
Copyright (c) 2017, Intel Corporation
Copyright (c) 2021, Facebook Inc.
 
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
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <openbmc/libgpio.h>
#include <openbmc/pal_def.h>
#include "SoftwareJTAGHandler.h"
#include "mem_helper.h"

typedef uint8_t __u8;
typedef uint32_t __u32;
typedef unsigned long long __u64;

#define jtag_tlr JtagTLR
#define jtag_rti JtagRTI
#define jtag_sel_dr JtagSelDR
#define jtag_cap_dr JtagCapDR
#define jtag_shf_dr JtagShfDR
#define jtag_ex1_dr JtagEx1DR
#define jtag_pau_dr JtagPauDR
#define jtag_ex2_dr JtagEx2DR
#define jtag_upd_dr JtagUpdDR
#define jtag_sel_ir JtagSelIR
#define jtag_cap_ir JtagCapIR
#define jtag_shf_ir JtagShfIR
#define jtag_ex1_ir JtagEx1IR
#define jtag_pau_ir JtagPauIR
#define jtag_ex2_ir JtagEx2IR
#define jtag_upd_ir JtagUpdIR

#define IRMAXPADSIZE 2000
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define BITS_PER_BYTE 8

struct jtag_xfer {
    __u8 type;
    __u8 direction;
    __u8 endstate;
    __u8 padding;
    __u32 length;
    __u64 tdio;
};

struct jtag_tap_state {
    __u8 reset;
    __u8 endstate;
    __u8 tck;
};

struct jtag_mode {
    __u32 feature;
    __u32 mode;
};

/* ioctl interface */
#define __JTAG_IOCTL_MAGIC 0xb2

#define JTAG_SIOCSTATE _IOW(__JTAG_IOCTL_MAGIC, 0, struct jtag_tap_state)
#define JTAG_IOCXFER _IOWR(__JTAG_IOCTL_MAGIC, 3, struct jtag_xfer)
#define JTAG_SIOCMODE _IOW(__JTAG_IOCTL_MAGIC, 5, unsigned int)
#define JTAG_IOCBITBANG _IOW(__JTAG_IOCTL_MAGIC, 6, unsigned int)

#define JTAG_XFER_MODE 0
#define JTAG_XFER_HW_MODE 1
#define JTAG_XFER_SW_MODE 0

enum jtag_reset {
    JTAG_NO_RESET = 0,
    JTAG_FORCE_RESET = 1,
};

enum jtag_xfer_type {
    JTAG_SIR_XFER = 0,
    JTAG_SDR_XFER = 1,
};

enum jtag_xfer_direction {
    JTAG_READ_XFER = 1,
    JTAG_WRITE_XFER = 2,
    JTAG_READ_WRITE_XFER = 3,
};

enum {
    JTAG_TARGET_CPU = 0,
    JTAG_TARGET_CPLD,
};

static
STATUS JTAG_set_target(int target)
{
    gpio_value_t expected_value, value = GPIO_VALUE_INVALID;
    gpio_desc_t *gpio;
    STATUS ret = ST_ERR;

    /* To have the JTAG master communicating to the CPU or CPLD */
    if (NULL == (gpio = gpio_open_by_shadow(FM_BMC_CPU_PWR_DEBUG_N))) {
      syslog(LOG_ERR, "Failed to open %s\n", FM_BMC_CPU_PWR_DEBUG_N);
      return ST_ERR;
    }

    expected_value = GPIO_VALUE_LOW;
    gpio_get_value(gpio, &value);
    if (value != expected_value) {
      if (gpio_set_direction(gpio, GPIO_DIRECTION_OUT)) {
        syslog(LOG_ERR, "Failed to set %s as an output\n", FM_BMC_CPU_PWR_DEBUG_N);
        goto bail;
      }

      gpio_set_value(gpio, expected_value);
      gpio_get_value(gpio, &value);
      if (value != expected_value) {
        syslog(LOG_WARNING, "Writing %d to %s failed! ASD is most probably disabled\n",
               expected_value, FM_BMC_CPU_PWR_DEBUG_N);
        gpio_set_direction(gpio, GPIO_DIRECTION_IN);
        goto bail;
      }
    }

    value = (target == JTAG_TARGET_CPLD) ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;
    if (gpio_set_value_by_shadow(FM_JTAG_BMC_MUX_SEL, value)) {
        syslog(LOG_ERR, "Failed to set %s as %d", FM_JTAG_BMC_MUX_SEL, value);
        goto bail;
    }

    ret = ST_OK;
bail:
    gpio_close(gpio);
    return ret;
}

static
STATUS JTAG_set_mux(scanChain chain)
{
    gpio_value_t value = (chain == SCAN_CHAIN_0) ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;

    if (gpio_set_value_by_shadow(FM_JTAG_TCK_MUX_SEL, value)) {
        syslog(LOG_ERR, "Failed to set %s as %d", FM_JTAG_TCK_MUX_SEL, value);
        return ST_ERR;
    }

    return ST_OK;
}


static
void initialize_jtag_chains(JTAG_Handler* state)
{
    for (int i = 0; i < MAX_SCAN_CHAINS; i++) {
        state->chains[i].shift_padding.drPre = 0;
        state->chains[i].shift_padding.drPost = 0;
        state->chains[i].shift_padding.irPre = 0;
        state->chains[i].shift_padding.irPost = 0;
        state->chains[i].tap_state = jtag_tlr;
        state->chains[i].scan_state = JTAGScanState_Done;
    }
}

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru)
{
    scanChain chain;
    JTAG_Handler* state = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    if (state == NULL) {
        return NULL;
    }

    chain = (fru & 0x80) ? SCAN_CHAIN_1 : SCAN_CHAIN_0;
    state->active_chain = &state->chains[chain];
    initialize_jtag_chains(state);
    state->sw_mode = true;
    memset(state->padDataOne, ~0, sizeof(state->padDataOne));
    memset(state->padDataZero, 0, sizeof(state->padDataZero));
    state->JTAG_driver_handle = -1;
    state->fru = fru;

    for (unsigned int i = 0; i < MAX_WAIT_CYCLES; i++) {
        state->bitbang_data[i].tms = 0;
        state->bitbang_data[i].tdi = 0;
    }

    if (JTAG_set_mux(chain) != ST_OK) {
      syslog(LOG_ERR, "Setting TCK_MUX failed\n");
    }

    return state;
}

STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
    gpio_value_t chain;
    struct jtag_mode jtag_mode;

    if (state == NULL)
        return ST_ERR;

    state->sw_mode = sw_mode;
    chain = (state->fru & 0x80) ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;

    state->JTAG_driver_handle = open("/dev/jtag0", O_RDWR);
    if (state->JTAG_driver_handle == -1) {
        syslog(LOG_ERR, "Can't open /dev/jtag0, please install driver");
        return ST_ERR;
    }

    jtag_mode.feature = JTAG_XFER_MODE;
    jtag_mode.mode = sw_mode ? JTAG_XFER_SW_MODE : JTAG_XFER_HW_MODE;
    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCMODE, &jtag_mode)) {
        syslog(LOG_ERR, "Failed JTAG_SIOCMODE to set xfer mode");
        close(state->JTAG_driver_handle);
        state->JTAG_driver_handle = -1;
        return ST_ERR;
    }

    if (JTAG_set_target(JTAG_TARGET_CPU) != ST_OK) {
        syslog(LOG_ERR, "Setting JTAG to CPU failed!");
        close(state->JTAG_driver_handle);
        state->JTAG_driver_handle = -1;
        return ST_ERR;
    }

    if (gpio_set_value_by_shadow(FM_JTAG_TCK_MUX_SEL, chain)) {
        syslog(LOG_ERR, "Failed to set FM_JTAG_TCK_MUX_SEL as %d", chain);
        close(state->JTAG_driver_handle);
        state->JTAG_driver_handle = -1;
        JTAG_set_target(JTAG_TARGET_CPLD);
        return ST_ERR;
    }

    if (JTAG_set_tap_state(state, jtag_tlr) != ST_OK) {
        syslog(LOG_ERR, "Failed to reset tap state.");
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

//
// Request the TAP to go to the target state
//
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state)
{
    if (state == NULL)
        return ST_ERR;

    struct jtag_tap_state tap_state_t;
    tap_state_t.endstate = tap_state;
    tap_state_t.reset =
        (tap_state == jtag_tlr) ? JTAG_FORCE_RESET : JTAG_NO_RESET;
    tap_state_t.tck = 1;

    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCSTATE, &tap_state_t) < 0) {
        syslog(LOG_ERR, "ioctl JTAG_SIOCSTATE failed");
        return ST_ERR;
    }

    state->active_chain->tap_state = tap_state;

    if ((tap_state == jtag_rti) || (tap_state == jtag_pau_dr))
        if (JTAG_wait_cycles(state, 5) != ST_OK)
            return ST_ERR;

    return ST_OK;
}

//
//  Optionally write and read the requested number of
//  bits and go to the requested target state
//
static
STATUS perform_shift(JTAG_Handler* state, uint32_t number_of_bits,
                     uint32_t input_bytes, uint8_t* input,
                     uint32_t output_bytes, uint8_t* output,
                     JtagStates current_tap_state,
                     JtagStates end_tap_state)
{
    struct jtag_xfer xfer;
    unsigned char tdio[MAX_DATA_SIZE];

    xfer.endstate = end_tap_state;
    xfer.type =
        (current_tap_state == jtag_shf_ir) ? JTAG_SIR_XFER : JTAG_SDR_XFER;
    xfer.length = number_of_bits;
    xfer.direction = JTAG_READ_WRITE_XFER;
    xfer.padding = 0;

    if (output != NULL) {
        if (memcpy_safe(output, output_bytes, input,
                        DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE))) {
            syslog(LOG_ERR, "memcpy_safe: input to output copy buffer failed.");
        }
        xfer.tdio = (__u64)(uintptr_t)output;
    } else {
        if (memcpy_safe(tdio, DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE),
                        input, DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE))) {
            syslog(LOG_ERR, "memcpy_safe: input to tdio buffer copy failed.");
        }
        xfer.tdio = (__u64)(uintptr_t)tdio;
    }
    if (ioctl(state->JTAG_driver_handle, JTAG_IOCXFER, &xfer) < 0) {
        syslog(LOG_ERR, "ioctl JTAG_IOCXFER failed");
        return ST_ERR;
    }

    state->active_chain->tap_state = end_tap_state;

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

    switch (current_state) {
      case jtag_shf_ir:
        preFix = state->active_chain->shift_padding.irPre;
        postFix = state->active_chain->shift_padding.irPost;
        padData = state->padDataOne;
        break;
      case jtag_shf_dr:
        preFix = state->active_chain->shift_padding.drPre;
        postFix = state->active_chain->shift_padding.drPost;
        padData = state->padDataZero;
        break;
      default:
        syslog(LOG_ERR, "Shift called but the tap is not in a ShiftIR/DR tap state");
        return ST_ERR;
    }

    if (state->active_chain->scan_state == JTAGScanState_Done) {
        state->active_chain->scan_state = JTAGScanState_Run;
        if (preFix) {
            if (perform_shift(state, preFix, IRMAXPADSIZE / 8, padData, 0, NULL,
                              current_state, current_state) != ST_OK)
                return ST_ERR;
        }
    }

    if ((postFix) && (current_state != end_tap_state)) {
        state->active_chain->scan_state = JTAGScanState_Done;
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, current_state,
                          current_state) != ST_OK)
            return ST_ERR;
        if (perform_shift(state, postFix, IRMAXPADSIZE / 8, padData, 0, NULL,
                          current_state, end_tap_state) != ST_OK)
            return ST_ERR;
    } else {
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, current_state,
                          end_tap_state) != ST_OK)
            return ST_ERR;
        if (current_state != end_tap_state) {
            state->active_chain->scan_state = JTAGScanState_Done;
        }
    }
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
// Wait for the requested cycles.
//
// Note: It is the responsibility of the caller to make sure that
// this call is made from RTI, PauDR, PauIR states only. Otherwise
// will have side effects !!!
//
STATUS JTAG_wait_cycles(JTAG_Handler* state, unsigned int number_of_cycles)
{
    if (state == NULL)
        return ST_ERR;

    if (number_of_cycles > MAX_WAIT_CYCLES)
        return ST_ERR;

    if (state->sw_mode) {
        for (unsigned int i = 0; i < number_of_cycles; i++) {
            if (ioctl(state->JTAG_driver_handle, JTAG_IOCBITBANG, &state->bitbang_data[i]) < 0) {
                syslog(LOG_ERR, "ioctl JTAG_IOCBITBANG failed");
                return ST_ERR;
            }
        }
    }
    return ST_OK;
}

STATUS JTAG_set_padding(JTAG_Handler* state, const JTAGPaddingTypes padding,
                        const int value)
{
    if (state == NULL)
        return ST_ERR;
    return ST_OK;
}

//
// Reset the Tap and wait in idle state
//
STATUS JTAG_tap_reset(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;
    return ST_OK;
}

STATUS JTAG_set_jtag_tck(JTAG_Handler* state, unsigned int tck)
{
    if (state == NULL)
        return ST_ERR;
    return ST_OK;
}

STATUS JTAG_set_active_chain(JTAG_Handler* state, scanChain chain)
{
    if (state == NULL)
        return ST_ERR;
    return ST_OK;
}

