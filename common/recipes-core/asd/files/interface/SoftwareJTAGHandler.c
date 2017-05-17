/*
* Stub functions for the Software JTAGJHandler
* Copyright (c) 2017, Facebook Inc.
* Copyright (c) 2017, Intel Corporation.
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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "SoftwareJTAGHandler.h"

/* Platform specific code must override this file with the actual
 * platform specific implementation to communicate with the JTAG
 * of the specified CPU */

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru)
{
  return NULL;
}

STATUS JTAG_initialize(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;
    return ST_OK;
}

STATUS JTAG_deinitialize(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;
    return ST_OK;
}

STATUS JTAG_set_padding(JTAG_Handler* state, const JTAGPaddingTypes padding, const int value)
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

//
// Request the TAP to go to the target state
//
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state)
{
    if (state == NULL)
        return ST_ERR;
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
    return ST_OK;
}

STATUS JTAG_set_clock_frequency(JTAG_Handler* state, unsigned int frequency)
{
    if (state == NULL)
        return ST_ERR;
    return ST_OK;
}
