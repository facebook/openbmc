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

// Supported JTAG Scan Chains
typedef enum {
    SCAN_CHAIN_0 = 0,
    SCAN_CHAIN_1,
    MAX_SCAN_CHAINS
} scanChain;

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

typedef struct JTAG_Chain_State {
    JtagStates tap_state;
    JTAGShiftPadding shift_padding;
    JTAGScanState scan_state;
} JTAG_Chain_State;

typedef struct JTAG_Handler {
    JTAG_Chain_State chains[MAX_SCAN_CHAINS];
    JTAG_Chain_State* active_chain;
    unsigned char padDataOne[MAXPADSIZE];
    unsigned char padDataZero[MAXPADSIZE];
    int JTAG_driver_handle;
    int fru;
    bool sw_mode;
} JTAG_Handler;

typedef enum {
    JFLOW_BMC = 1,
    JFLOW_BIC = 2
} JFLOW;

struct message_header {
    uint32_t origin_id : 3;
    uint32_t reserved  : 1;
    uint32_t enc_bit   : 1;
    uint32_t type      : 3;
    uint32_t size_lsb  : 8;
    uint32_t size_msb  : 5;
    uint32_t tag       : 3;
    uint32_t cmd_stat  : 8;
} __attribute__((packed));

struct spi_message {
    struct message_header header;
    unsigned char* buffer;
} __attribute__((packed));

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru);
STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode);
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
STATUS JTAG_set_jtag_tck(JTAG_Handler* state, unsigned int tck);
STATUS JTAG_set_active_chain(JTAG_Handler* state, scanChain chain);

#ifdef CONFIG_JTAG_MSG_FLOW
STATUS JTAG_set_device(uint8_t fru, uint8_t dev_id);
STATUS JTAG_init_passthrough(JTAG_Handler *state, uint8_t jflow, STATUS (*callback)(struct spi_message *));
STATUS passthrough_jtag_message(JTAG_Handler *state, struct spi_message *s_message);
#endif

#ifdef __cplusplus
}
#endif

#endif  // _SOFTWARE_JTAG_HANDLER_H_
