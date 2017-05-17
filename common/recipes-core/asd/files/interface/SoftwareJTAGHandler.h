/*
* Header for the Software JTAGJHandler
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

#ifndef _SOFTWARE_JTAG_HANDLER_H_
#define _SOFTWARE_JTAG_HANDLER_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAXPADSIZE 512

typedef enum {
    JtagTLR,
    JtagRTI,
    JtagSelDR,
    JtagCapDR,
    JtagShfDR,
    JtagEx1DR,
    JtagPauDR,
    JtagEx2DR,
    JtagUpdDR,
    JtagSelIR,
    JtagCapIR,
    JtagShfIR,
    JtagEx1IR,
    JtagPauIR,
    JtagEx2IR,
    JtagUpdIR
} JtagStates;

typedef enum {
    ST_OK,
    ST_ERR
} STATUS;

typedef enum {
    JTAGDriverState_Master = 0,
    JTAGDriverState_Slave
} JTAGDriverState;

typedef enum {
    JTAGPaddingTypes_IRPre,
    JTAGPaddingTypes_IRPost,
    JTAGPaddingTypes_DRPre,
    JTAGPaddingTypes_DRPost
} JTAGPaddingTypes;

typedef enum {
    JTAGScanState_Done = 0,
    JTAGScanState_Run
} JTAGScanState;

typedef struct JTAGShiftPadding {
    int drPre;
    int drPost;
    int irPre;
    int irPost;
} JTAGShiftPadding;

typedef struct JTAG_Handler {
    JtagStates tap_state;
    JTAGShiftPadding shift_padding;
    unsigned char padDataOne[MAXPADSIZE];
    unsigned char padDataZero[MAXPADSIZE];
    JTAGScanState scan_state;
    int JTAG_driver_handle;
} JTAG_Handler;

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru);
STATUS JTAG_initialize(JTAG_Handler* state);
STATUS JTAG_deinitialize(JTAG_Handler* state);
STATUS JTAG_set_padding(JTAG_Handler* state, const JTAGPaddingTypes padding, const int value);
STATUS JTAG_tap_reset(JTAG_Handler* state);
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state);
STATUS JTAG_get_tap_state(JTAG_Handler* state, JtagStates* tap_state);
STATUS JTAG_shift(JTAG_Handler* state, unsigned int number_of_bits,
                  unsigned int input_bytes, unsigned char* input,
                  unsigned int output_bytes, unsigned char* output,
                  JtagStates end_tap_state);
STATUS JTAG_wait_cycles(JTAG_Handler* state, unsigned int number_of_cycles);
STATUS JTAG_set_clock_frequency(JTAG_Handler* state, unsigned int frequency);

#ifdef __cplusplus
}
#endif

#endif  // _SOFTWARE_JTAG_HANDLER_H_
