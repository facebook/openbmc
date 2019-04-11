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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "SoftwareJTAGHandler.h"
#include <syslog.h>
#include <sys/mman.h>
#include <openbmc/gpio.h>
#include <openbmc/pal.h>

// Enable to print IR/DR trace to syslog
//#define DEBUG

#define JTAGIOC_BASE    'T'

#define AST_JTAG_SIOCFREQ         _IOW( JTAGIOC_BASE, 3, unsigned int)
#define AST_JTAG_GIOCFREQ         _IOR( JTAGIOC_BASE, 4, unsigned int)
#define AST_JTAG_BITBANG          _IOWR(JTAGIOC_BASE, 5, struct tck_bitbang)
#define AST_JTAG_SET_TAPSTATE     _IOW( JTAGIOC_BASE, 6, struct tap_state_param)
#define AST_JTAG_READWRITESCAN    _IOWR(JTAGIOC_BASE, 7, struct scan_xfer)
#define AST_JTAG_SLAVECONTLR      _IOW( JTAGIOC_BASE, 8, struct controller_mode_param)
#define AST_JTAG_SET_TCK          _IOW( JTAGIOC_BASE, 9, struct set_tck_param)
#define AST_JTAG_GET_TCK          _IOR( JTAGIOC_BASE, 10, struct get_tck_param)

typedef enum xfer_mode {
    HW_MODE = 0,
    SW_MODE
} xfer_mode;

struct tck_bitbang {
    unsigned char     tms;
    unsigned char     tdi;        // TDI bit value to write
    unsigned char     tdo;        // TDO bit value to read
};

struct scan_xfer {
    xfer_mode        mode;        // Hardware or software mode
    unsigned int     tap_state;   // Current tap state
    unsigned int     length;      // number of bits to clock
    unsigned char    *tdi;        // data to write to tap (optional)
    unsigned int     tdi_bytes;
    unsigned char    *tdo;        // data to read from tap (optional)
    unsigned int     tdo_bytes;
    unsigned int     end_tap_state;
};

struct set_tck_param {
    xfer_mode        mode;        // Hardware or software mode
    unsigned int     tck;         // This can be treated differently on systems as needed. It can be a divisor or actual frequency as needed.
};

struct get_tck_param {
    xfer_mode        mode;        // Hardware or software mode
    unsigned int     tck;         // This can be treated differently on systems as needed. It can be a divisor or actual frequency as needed.
};

struct tap_state_param {
    xfer_mode        mode;        // Hardware or software mode
    unsigned int     from_state;
    unsigned int     to_state;
};

struct controller_mode_param {
    xfer_mode        mode;        // Hardware or software mode
    unsigned int     controller_mode;
};

enum {
  JTAG_TARGET_CPU,
  JTAG_TARGET_CPLD
};

STATUS JTAG_set_cntlr_mode(JTAG_Handler *state, const JTAGDriverState setMode);
STATUS JTAG_clock_cycle(int handle, unsigned char tms, unsigned char tdi);
STATUS perform_shift(JTAG_Handler *state, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     JtagStates current_tap_state, JtagStates end_tap_state);


void initialize_jtag_chains(JTAG_Handler* state) {
    for (int i=0; i<MAX_SCAN_CHAINS; i++) {
        state->chains[i].shift_padding.drPre = 0;
        state->chains[i].shift_padding.drPost = 0;
        state->chains[i].shift_padding.irPre = 0;
        state->chains[i].shift_padding.irPost = 0;
        state->chains[i].tap_state = JtagTLR;
        state->chains[i].scan_state = JTAGScanState_Done;
    }
}

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru)
{
    JTAG_Handler *state;

    if (fru != FRU_MB) {
      return NULL;
    }

    state = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    if (state == NULL) {
        return NULL;
    }

    state->active_chain = &state->chains[SCAN_CHAIN_0];
    initialize_jtag_chains(state);
    state->sw_mode = true;
    memset(state->padDataOne, ~0, sizeof(state->padDataOne));
    memset(state->padDataZero, 0, sizeof(state->padDataZero));

    state->JTAG_driver_handle = -1;

    return state;
}

STATUS JTAG_clock_cycle(int handle, unsigned char tms, unsigned char tdi)
{
    struct tck_bitbang bitbang = {0};

    bitbang.tms = tms;
    bitbang.tdi = tdi;

    if (ioctl(handle, AST_JTAG_BITBANG, &bitbang) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_BITBANG failed");
        return ST_ERR;
    }
    return ST_OK;
}

// user level access to set the AST2500 JTAG controller in slave or master mode
STATUS JTAG_set_cntlr_mode(JTAG_Handler* state, const JTAGDriverState setMode)
{
    struct controller_mode_param params = {0};
    params.mode = state->sw_mode ? SW_MODE : HW_MODE;

    if ((setMode < JTAGDriverState_Master) || (setMode > JTAGDriverState_Slave)) {
        syslog(LOG_ERR, "An invalid JTAG controller state was used");
        return ST_ERR;
    }

    params.controller_mode = setMode;
    syslog(LOG_DEBUG, "Setting JTAG controller mode to %s.",
            setMode == JTAGDriverState_Master ? "MASTER" : "SLAVE");
    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SLAVECONTLR, &params) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_SLAVECONTLR failed");
        return ST_ERR;
    }
    return ST_OK;
}

static STATUS JTAG_set_target(int target)
{
    gpio_value_en expected_value;
    gpio_st gpio;
    STATUS ret = ST_ERR;

    /* Change GPIOY2 to have the JTAG master communicating
     * to the CPU instead of CPLD */
    if (gpio_open(&gpio, gpio_num("GPIOY2"))) {
      syslog(LOG_ERR, "Failed to open GPIOY2\n");
      return ST_ERR;
    }

    if (gpio_change_direction(&gpio, GPIO_DIRECTION_OUT)) {
      syslog(LOG_ERR, "Failed to set GPIOY2 as an output\n");
      goto bail;
    }

    if (target == JTAG_TARGET_CPU) {
      // Set internal strap.
      system("devmem 0x1E6E207C w 0x80000");
      expected_value = GPIO_VALUE_LOW;
    } else {
      expected_value = GPIO_VALUE_HIGH;
    }

    gpio_write(&gpio, expected_value);
    if (gpio_read(&gpio) != expected_value) {
      syslog(LOG_WARNING, "Writing %d to GPIOY2 failed! ATSD is most probably disabled\n", expected_value);
      goto bail;
    }
    ret = ST_OK;
bail:
    gpio_close(&gpio);
    return ret;
}

static STATUS JTAG_set_mux(void)
{
    gpio_value_en expected_value = GPIO_VALUE_LOW;
    gpio_st gpio;
    STATUS ret = ST_ERR;

    if (gpio_open(&gpio, gpio_num("GPIOR2"))) {
      syslog(LOG_ERR, "Failed to open GPIOR2\n");
      return ST_ERR;
    }

    if (gpio_change_direction(&gpio, GPIO_DIRECTION_OUT)) {
      syslog(LOG_ERR, "Failed to set GPIOR2 as an output\n");
      goto bail;
    }

    // TODO Does this need to be switched?
    gpio_write(&gpio, expected_value);
    if (gpio_read(&gpio) != expected_value) {
      syslog(LOG_WARNING, "Writing %d to GPIOR2 failed! ATSD is most probably disabled\n", expected_value);
      goto bail;
    }
    ret = ST_OK;
bail:
    gpio_close(&gpio);
    return ret;

}

STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
    if (state == NULL)
        return ST_ERR;

    state->sw_mode = sw_mode;
    if (JTAG_set_target(JTAG_TARGET_CPU) != ST_OK) {
      syslog(LOG_ERR, "Setting JTAG to CPU failed!\n");
      return ST_ERR;
    }

    if (JTAG_set_mux() != ST_OK) {
      syslog(LOG_ERR, "Setting TCK_MUX failed\n");
      goto bail_err;
    }

    initialize_jtag_chains(state);
    state->JTAG_driver_handle = open("/dev/ast-jtag", O_RDWR);
    if (state->JTAG_driver_handle == -1) {
        syslog(LOG_ERR, "Can't open /dev/ast-jtag, please install driver");
        goto bail_err;
    }

    if (JTAG_set_cntlr_mode(state, JTAGDriverState_Master) != ST_OK) {
        syslog(LOG_ERR, "Failed to set JTAG mode to master.");
        goto bail_err;
    }
    return ST_OK;
bail_err:
    JTAG_set_target(JTAG_TARGET_CPLD);
    return ST_ERR;
}

STATUS JTAG_deinitialize(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;

    STATUS result = JTAG_set_cntlr_mode(state, JTAGDriverState_Slave);
    if (result != ST_OK) {
        syslog(LOG_ERR, "Failed to set JTAG mode to slave.");
    }

    close(state->JTAG_driver_handle);
    state->JTAG_driver_handle = -1;

    JTAG_set_target(JTAG_TARGET_CPLD);
    return result;
}

STATUS JTAG_set_padding(JTAG_Handler* state, const JTAGPaddingTypes padding, const int value)
{
    if (state == NULL || value > MAXPADSIZE)
        return ST_ERR;

    if (padding == JTAGPaddingTypes_DRPre) {
        state->active_chain->shift_padding.drPre = value;
    } else if (padding == JTAGPaddingTypes_DRPost) {
        state->active_chain->shift_padding.drPost = value;
    } else if (padding == JTAGPaddingTypes_IRPre) {
        state->active_chain->shift_padding.irPre = value;
    } else if (padding == JTAGPaddingTypes_IRPost) {
        state->active_chain->shift_padding.irPost = value;
    } else {
        syslog(LOG_ERR, "Unknown padding value: %d", value);
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
    return JTAG_set_tap_state(state, JtagTLR);
}

//
// Request the TAP to go to the target state
//
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state)
{
    if (state == NULL)
        return ST_ERR;

    struct tap_state_param params = {0};
    params.mode = state->sw_mode ? SW_MODE : HW_MODE;
    params.from_state = state->active_chain->tap_state;
    params.to_state = tap_state;

    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SET_TAPSTATE, &params) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_SET_TAPSTATE failed");
        return ST_ERR;
    }

    state->active_chain->tap_state = tap_state;

    if ((tap_state == JtagRTI) || (tap_state == JtagPauDR))
        if (JTAG_wait_cycles(state, 5) != ST_OK)
            return ST_ERR;
#ifdef DEBUG
    syslog(LOG_DEBUG, "TapState: %d", state->active_chain->tap_state);
#endif
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
    unsigned char* padData = state->padDataOne;
    JtagStates current_state;
    JTAG_get_tap_state(state, &current_state);

    if (current_state == JtagShfIR) {
        preFix = state->active_chain->shift_padding.irPre;
        postFix = state->active_chain->shift_padding.irPost;
        padData = state->padDataOne;
    } else if (current_state == JtagShfDR) {
        preFix = state->active_chain->shift_padding.drPre;
        postFix = state->active_chain->shift_padding.drPost;
        padData = state->padDataZero;
    } else {
        syslog(LOG_ERR, "Shift called but the tap is not in a ShiftIR/DR tap state");
        return ST_ERR;
    }

    if (state->active_chain->scan_state == JTAGScanState_Done) {
        state->active_chain->scan_state = JTAGScanState_Run;
        if (preFix) {
            if (perform_shift(state, preFix, MAXPADSIZE, padData,
                              0, NULL, current_state, current_state) != ST_OK)
                return ST_ERR;
        }
    }

    if ((postFix) && (current_state != end_tap_state)) {
        state->active_chain->scan_state = JTAGScanState_Done;
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, current_state, current_state) != ST_OK)
            return ST_ERR;
        if (perform_shift(state, postFix, MAXPADSIZE, padData, 0,
                          NULL, current_state, end_tap_state) != ST_OK)
            return ST_ERR;
    } else {
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, current_state, end_tap_state) != ST_OK)
            return ST_ERR;
        if (current_state != end_tap_state) {
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
                     JtagStates current_tap_state, JtagStates end_tap_state)
{
    struct scan_xfer scan_xfer = {0};
    scan_xfer.mode = state->sw_mode ? SW_MODE : HW_MODE;
    scan_xfer.tap_state = current_tap_state;
    scan_xfer.length = number_of_bits;
    scan_xfer.tdi_bytes = input_bytes;
    scan_xfer.tdi = input;
    scan_xfer.tdo_bytes = output_bytes;
    scan_xfer.tdo = output;
    scan_xfer.end_tap_state = end_tap_state;

    if (ioctl(state->JTAG_driver_handle, AST_JTAG_READWRITESCAN, &scan_xfer) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_READWRITESCAN failed!");
        return ST_ERR;
    }
    state->active_chain->tap_state = end_tap_state;
#if 0
    if (input != NULL)
        ASD_log_shift((current_tap_state == JtagShfDR), true, number_of_bits, input_bytes, input);
    if (output != NULL)
        ASD_log_shift((current_tap_state == JtagShfDR), false, number_of_bits, output_bytes, output);
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
    if (state == NULL)
        return ST_ERR;
    if (state->sw_mode) {
        for (unsigned int i = 0; i < number_of_cycles; i++) {
            if (JTAG_clock_cycle(state->JTAG_driver_handle, 0, 0) != ST_OK)
                return ST_ERR;
        }
    }
    return ST_OK;
}

STATUS JTAG_set_jtag_tck(JTAG_Handler* state, unsigned int tck)
{
    if (state == NULL)
        return ST_ERR;

    struct set_tck_param params = {0};
    params.mode = state->sw_mode ? SW_MODE : HW_MODE;
    params.tck = tck;

    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SET_TCK, &params) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_SET_TCK failed");
        return ST_ERR;
    }
    return ST_OK;
}

STATUS JTAG_set_active_chain(JTAG_Handler* state, scanChain chain) {
    if (state == NULL)
        return ST_ERR;

    if (chain < 0 || chain >= MAX_SCAN_CHAINS) {
        syslog(LOG_ERR, "Invalid scan chain.");
        return ST_ERR;
    }

    state->active_chain = &state->chains[chain];
    return ST_OK;
}
