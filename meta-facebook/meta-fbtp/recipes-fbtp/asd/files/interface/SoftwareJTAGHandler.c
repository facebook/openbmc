/*
* Functions for the Software JTAGJHandler
* Copyright (c) 2017, Intel Corporation.
*
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
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
#define AST_JTAG_SET_TAPSTATE     _IOW( JTAGIOC_BASE, 6, unsigned int)
#define AST_JTAG_READWRITESCAN    _IOWR(JTAGIOC_BASE, 7, struct scan_xfer)
#define AST_JTAG_SLAVECONTLR      _IOW( JTAGIOC_BASE, 8, unsigned int)

struct tck_bitbang {
    unsigned char     tms;
    unsigned char     tdi;        // TDI bit value to write
    unsigned char     tdo;        // TDO bit value to read
};

struct scan_xfer {
    unsigned int     length;      // number of bits to clock
    unsigned char    *tdi;        // data to write to tap (optional)
    unsigned int     tdi_bytes;
    unsigned char    *tdo;        // data to read from tap (optional)
    unsigned int     tdo_bytes;
    unsigned int     end_tap_state;
};

enum {
  JTAG_TARGET_CPU,
  JTAG_TARGET_CPLD
};

STATUS JTAG_set_cntlr_mode(int handle, const JTAGDriverState setMode);
STATUS JTAG_clock_cycle(int handle, unsigned char tms, unsigned char tdi);
STATUS perform_shift(int handle, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     JtagStates current_tap_state, JtagStates end_tap_state);

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

    state->shift_padding.drPre = 0;
    state->shift_padding.drPost = 0;
    state->shift_padding.irPre = 0;
    state->shift_padding.irPost = 0;

    state->tap_state = JtagTLR;
    memset(state->padDataOne, ~0, sizeof(state->padDataOne));
    memset(state->padDataZero, 0, sizeof(state->padDataZero));
    state->scan_state = JTAGScanState_Done;

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
STATUS JTAG_set_cntlr_mode(int handle, const JTAGDriverState setMode)
{
    if ((setMode < JTAGDriverState_Master) || (setMode > JTAGDriverState_Slave)) {
        syslog(LOG_ERR, "An invalid JTAG controller state was used");
        return ST_ERR;
    }
    syslog(LOG_DEBUG, "Setting JTAG controller mode to %s.",
            setMode == JTAGDriverState_Master ? "MASTER" : "SLAVE");
    if (ioctl(handle, AST_JTAG_SLAVECONTLR, setMode) < 0) {
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

STATUS JTAG_initialize(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;

    if (JTAG_set_target(JTAG_TARGET_CPU) != ST_OK) {
      return ST_ERR;
    }

    if (JTAG_set_mux() != ST_OK) {
      syslog(LOG_ERR, "Setting TCK_MUX failed\n");
      goto bail_err;
    }

    state->JTAG_driver_handle = open("/dev/ast-jtag", O_RDWR);
    if (state->JTAG_driver_handle == -1) {
        syslog(LOG_ERR, "Can't open /dev/ast-jtag, please install driver");
        goto bail_err;
    }

    if (JTAG_set_cntlr_mode(state->JTAG_driver_handle, JTAGDriverState_Master) != ST_OK) {
        syslog(LOG_ERR, "Failed to set JTAG mode to master.");
        goto bail_err;
    }
    if (JTAG_set_tap_state(state, JtagTLR) != ST_OK ||
        JTAG_set_tap_state(state, JtagRTI) != ST_OK) {
        syslog(LOG_ERR, "Failed to set initial TAP state.");
        goto bail_err;
    }
    if (JTAG_set_padding(state, JTAGPaddingTypes_DRPre, 0) != ST_OK ||
        JTAG_set_padding(state, JTAGPaddingTypes_DRPost, 0) != ST_OK ||
        JTAG_set_padding(state, JTAGPaddingTypes_IRPre, 0) != ST_OK ||
        JTAG_set_padding(state, JTAGPaddingTypes_IRPost, 0) != ST_OK) {
        syslog(LOG_ERR, "Failed to set initial padding states.");
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

    STATUS result = JTAG_set_cntlr_mode(state->JTAG_driver_handle, JTAGDriverState_Master);
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
    if (state == NULL)
        return ST_ERR;

    if (padding == JTAGPaddingTypes_DRPre) {
        state->shift_padding.drPre = value;
    } else if (padding == JTAGPaddingTypes_DRPost) {
        state->shift_padding.drPost = value;
    } else if (padding == JTAGPaddingTypes_IRPre) {
        state->shift_padding.irPre = value;
    } else if (padding == JTAGPaddingTypes_IRPost) {
        state->shift_padding.irPost = value;
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

    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SET_TAPSTATE, tap_state) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_SET_TAPSTATE failed");
        return ST_ERR;
    }

    // move the [soft] state to the requested tap state.
    state->tap_state = tap_state;

    if ((tap_state == JtagRTI) || (tap_state == JtagPauDR))
        if (JTAG_wait_cycles(state, 5) != ST_OK)
            return ST_ERR;

    syslog(LOG_DEBUG, "TapState: %d", state->tap_state);
    return ST_OK;
}

//
// Retrieve the current the TAP state
//
STATUS JTAG_get_tap_state(JTAG_Handler* state, JtagStates* tap_state)
{
    if (state == NULL || tap_state == NULL)
        return ST_ERR;
    *tap_state = state->tap_state;
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

    if (state->tap_state == JtagShfIR) {
        preFix = state->shift_padding.irPre;
        postFix = state->shift_padding.irPost;
        padData = state->padDataOne;
    } else if (state->tap_state == JtagShfDR) {
        preFix = state->shift_padding.drPre;
        postFix = state->shift_padding.drPost;
        padData = state->padDataZero;
    } else {
        syslog(LOG_ERR, "Shift called but the tap is not in a ShiftIR/DR tap state");
        return ST_ERR;
    }

    if (state->scan_state == JTAGScanState_Done) {
        state->scan_state = JTAGScanState_Run;
        if (preFix) {
            if (perform_shift(state->JTAG_driver_handle, preFix, MAXPADSIZE, padData,
                              0, NULL, state->tap_state, state->tap_state) != ST_OK)
                return ST_ERR;
        }
    }

    if ((postFix) && (state->tap_state != end_tap_state)) {
        state->scan_state = JTAGScanState_Done;
        if (perform_shift(state->JTAG_driver_handle, number_of_bits, input_bytes, input,
                          output_bytes, output, state->tap_state, state->tap_state) != ST_OK)
            return ST_ERR;
        if (perform_shift(state->JTAG_driver_handle, postFix, MAXPADSIZE, padData, 0,
                          NULL, state->tap_state, end_tap_state) != ST_OK)
            return ST_ERR;
    } else {
        if (perform_shift(state->JTAG_driver_handle, number_of_bits, input_bytes, input,
                          output_bytes, output, state->tap_state, end_tap_state) != ST_OK)
            return ST_ERR;
        if (state->tap_state != end_tap_state) {
            state->scan_state = JTAGScanState_Done;
        }
    }
    return ST_OK;
}

//
//  Optionally write and read the requested number of
//  bits and go to the requested target state
//
STATUS perform_shift(int handle, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     JtagStates current_tap_state, JtagStates end_tap_state)
{
    struct scan_xfer scan_xfer = {0};
    scan_xfer.length = number_of_bits;
    scan_xfer.tdi_bytes = input_bytes;
    scan_xfer.tdi = input;
    scan_xfer.tdo_bytes = output_bytes;
    scan_xfer.tdo = output;
    scan_xfer.end_tap_state = end_tap_state;

    if (ioctl(handle, AST_JTAG_READWRITESCAN, &scan_xfer) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_READWRITESCAN failed!");
        return ST_ERR;
    }

#ifdef DEBUG
    {
        unsigned int number_of_bytes = (number_of_bits + 7) / 8;
        const char* shiftStr = (current_tap_state == JtagShfDR) ? "DR" : "IR";
        syslog(LOG_DEBUG, "%s size: %d", shiftStr, number_of_bits);
        if (input != NULL && number_of_bytes <= input_bytes) {
            syslog_buffer(LOG_DEBUG, input, number_of_bytes,
                           (current_tap_state == JtagShfDR) ? "DR TDI" : "IR TDI");
        }
        if (output != NULL && number_of_bytes <= output_bytes) {
            syslog_buffer(LOG_DEBUG, output, number_of_bytes,
                           (current_tap_state == JtagShfDR) ? "DR TDO" : "IR TDO");
        }
        syslog(LOG_DEBUG, "%s: End tap state: %d", shiftStr, end_tap_state);
    }
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
    for (unsigned int i = 0; i < number_of_cycles; i++) {
        if (JTAG_clock_cycle(state->JTAG_driver_handle, 0, 0) != ST_OK)
            return ST_ERR;
    }
    return ST_OK;
}

STATUS JTAG_set_clock_frequency(JTAG_Handler* state, unsigned int frequency)
{
    if (state == NULL)
        return ST_ERR;
    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SIOCFREQ, frequency) < 0) {
        syslog(LOG_ERR, "ioctl AST_JTAG_SIOCFREQ failed");
        return ST_ERR;
    }
    return ST_OK;
}
