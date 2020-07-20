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
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
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


// Enable FBY3-specific debug messages
//#define FBY3_DEBUG

// ignore errors communicating with BIC
//#define FBY3_IGNORE_ERROR

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))

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

const char *tap_states_name[] = {
    "TLR",
    "RTI",
    "SelDR",
    "CapDR",
    "ShfDR",
    "Ex1DR",
    "PauDR",
    "Ex2DR",
    "UpdDR",
    "SelIR",
    "CapIR",
    "ShfIR",
    "Ex1IR",
    "PauIR",
    "Ex2IR",
    "UpdIR",
};


typedef struct
// this structure represents a TMS cycle, as expressed in a set of bits and a count of bits (note: there are no start->end state transitions that require more than 1 byte of TMS cycles)
{
    unsigned char tmsbits;
    unsigned char count;
} TmsCycle;


// this is the complete set TMS cycles for going from any TAP state to any other TAP state, following a “shortest path” rule
const TmsCycle _tmsCycleLookup[][16] = {
/*   start*/ /*TLR      RTI      SelDR    CapDR    SDR      Ex1DR    PDR      Ex2DR    UpdDR    SelIR    CapIR    SIR      Ex1IR    PIR      Ex2IR    UpdIR    destination*/
/*     TLR*/{ {0x00,0},{0x00,1},{0x02,2},{0x02,3},{0x02,4},{0x0a,4},{0x0a,5},{0x2a,6},{0x1a,5},{0x06,3},{0x06,4},{0x06,5},{0x16,5},{0x16,6},{0x56,7},{0x36,6} },
/*     RTI*/{ {0x07,3},{0x00,0},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x0d,4},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5} },
/*   SelDR*/{ {0x03,2},{0x03,3},{0x00,0},{0x00,1},{0x00,2},{0x02,2},{0x02,3},{0x0a,4},{0x06,3},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x0d,4} },
/*   CapDR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x00,0},{0x00,1},{0x01,1},{0x01,2},{0x05,3},{0x03,2},{0x0f,4},{0x0f,5},{0x0f,6},{0x2f,6},{0x2f,7},{0xaf,8},{0x6f,7} },
/*     SDR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x00,0},{0x01,1},{0x01,2},{0x05,3},{0x03,2},{0x0f,4},{0x0f,5},{0x0f,6},{0x2f,6},{0x2f,7},{0xaf,8},{0x6f,7} },
/*   Ex1DR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x02,3},{0x00,0},{0x00,1},{0x02,2},{0x01,1},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6} },
/*     PDR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x01,2},{0x05,3},{0x00,0},{0x01,1},{0x03,2},{0x0f,4},{0x0f,5},{0x0f,6},{0x2f,6},{0x2f,7},{0xaf,8},{0x6f,7} },
/*   Ex2DR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x00,1},{0x02,2},{0x02,3},{0x00,0},{0x01,1},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6} },
/*   UpdDR*/{ {0x07,3},{0x00,1},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x00,0},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5} },
/*   SelIR*/{ {0x01,1},{0x01,2},{0x05,3},{0x05,4},{0x05,5},{0x15,5},{0x15,6},{0x55,7},{0x35,6},{0x00,0},{0x00,1},{0x00,2},{0x02,2},{0x02,3},{0x0a,4},{0x06,3} },
/*   CapIR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6},{0x0f,4},{0x00,0},{0x00,1},{0x01,1},{0x01,2},{0x05,3},{0x03,2} },
/*     SIR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6},{0x0f,4},{0x0f,5},{0x00,0},{0x01,1},{0x01,2},{0x05,3},{0x03,2} },
/*   Ex1IR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5},{0x07,3},{0x07,4},{0x02,3},{0x00,0},{0x00,1},{0x02,2},{0x01,1} },
/*     PIR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6},{0x0f,4},{0x0f,5},{0x01,2},{0x05,3},{0x00,0},{0x01,1},{0x03,2} },
/*   Ex2IR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5},{0x07,3},{0x07,4},{0x00,1},{0x02,2},{0x02,3},{0x00,0},{0x01,1} },
/*   UpdIR*/{ {0x07,3},{0x00,1},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x0d,4},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x00,0} },
};




static int jtag_bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                  uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t *rxlen);



static STATUS jtag_bic_set_tap_state(uint8_t slot_id, JtagStates src_state, JtagStates tap_state);

static STATUS jtag_bic_shift_wrapper(uint8_t slot_id,
                              unsigned int write_bit_length, uint8_t* write_data,
                              unsigned int read_bit_length, uint8_t* read_data,
                              unsigned int last);

static STATUS jtag_bic_read_write_scan(JTAG_Handler* state, struct scan_xfer *scan_xfer);



static STATUS JTAG_clock_cycle(uint8_t slot_id, int number_of_cycles);
static STATUS perform_shift(JTAG_Handler* state , unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     JtagStates current_tap_state, JtagStates end_tap_state);


static int m_slot_id = -1;


// debug helper functions
#ifdef FBY3_DEBUG


void print_jtag_state(const char *func, JTAG_Handler *state)
{
    if (state == NULL) {
        printf("Error: null passed to print_state\n");
    }

    if (state->active_chain->tap_state > 15) {
        printf("Error! tap_state=%d(invalid!)\n", state->active_chain->tap_state);
    }

    else {
        printf("\n%s\n    -JTAG.tap_state=%d(%s), active_chain->shift_padding.drPre=%d, active_chain->shift_padding.drPost\
=%d, active_chain->shift_padding.irPre=%d, active_chain->shift_padding.irPost=%d scan_state=%d slot_id=%d\n", func,
            state->active_chain->tap_state,
            tap_states_name[state->active_chain->tap_state],
            state->active_chain->shift_padding.drPre,
            state->active_chain->shift_padding.drPost,
            state->active_chain->shift_padding.irPre,
            state->active_chain->shift_padding.irPost,
            state->active_chain->scan_state,
            state->fru
          );
    }
}
#endif
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

STATUS JTAG_set_device(uint8_t fru, uint8_t dev_id) {
  return ST_OK;
}

JTAG_Handler* SoftwareJTAGHandler(uint8_t fru)
{
    JTAG_Handler *state;

    if ( fby3_common_check_slot_id(fru) < 0 ) {
      syslog(LOG_ERR, "%s: invalid fru: %d", __FUNCTION__, fru);
      return NULL;
    }

    state = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    if (state == NULL) {
        return NULL;
    }

    initialize_jtag_chains(state);
    state->active_chain = &state->chains[SCAN_CHAIN_0];
    memset(state->padDataOne, ~0, sizeof(state->padDataOne));
    memset(state->padDataZero, 0, sizeof(state->padDataZero));

    state->fru = fru;

    return state;
}



STATUS JTAG_clock_cycle(uint8_t slot_id, int number_of_cycles)
{
    uint8_t tbuf[5] = {0x9c, 0x9c, 0x00}; // IANA ID
    uint8_t rbuf[4] = {0x00};
    uint8_t rlen = 0;
    uint8_t tlen = 5;


    if (number_of_cycles > 255)
    {
      syslog(LOG_ERR, "ASD: delay cycle = %d(> 255). slot%d",
           number_of_cycles, slot_id);
      number_of_cycles = 255;
    }

    // tbuf[0:2] = IANA ID
    // tbuf[3]   = tms bit length
    // tbuf[4]   = tmsbits
    tbuf[3] = number_of_cycles;
    tbuf[4] = 0x0;

    if (jtag_bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_TAP_STATE,
                           tbuf, tlen, rbuf, &rlen) < 0) {
        syslog(LOG_ERR, "wait cycle failed, slot%d", slot_id);
        return ST_ERR;
    }

    return ST_OK;
}


STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
#endif


    if (state == NULL)
        return ST_ERR;

    JTAG_tap_reset(state);
    if (JTAG_set_tap_state(state, JtagTLR) != ST_OK ||
        JTAG_set_tap_state(state, JtagRTI) != ST_OK) {
        syslog(LOG_ERR, "Failed to set initial TAP state for. slot%d",
               state->fru);
        goto bail_err;
    }

    if (JTAG_set_padding(state, JTAGPaddingTypes_DRPre, 0) != ST_OK ||
        JTAG_set_padding(state, JTAGPaddingTypes_DRPost, 0) != ST_OK ||
        JTAG_set_padding(state, JTAGPaddingTypes_IRPre, 0) != ST_OK ||
        JTAG_set_padding(state, JTAGPaddingTypes_IRPost, 0) != ST_OK) {
        syslog(LOG_ERR, "Failed to set initial padding states. slot%d",
               state->fru);
        goto bail_err;
    }
    return ST_OK;
bail_err:
    return ST_ERR;
}

STATUS JTAG_deinitialize(JTAG_Handler* state)
{
    STATUS result = ST_OK;

#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
#endif

    if (state == NULL)
        return ST_ERR;

    if (bic_asd_init(state->fru, 0xFF) < 0) {
        syslog(LOG_ERR, "1S_ASD_DEINIT failed, slot%d", state->fru);
    }

    return result;
}


STATUS JTAG_set_padding(JTAG_Handler* state, const JTAGPaddingTypes padding, const int value)
{
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -paddingType=%d, value=%d\n", padding, value);
#endif

    if (state == NULL)
        return ST_ERR;

    if (value > 0) {
      syslog(LOG_DEBUG, "ASD: __%s__: slot=%d, type=%d, value%d",
             __FUNCTION__, state->fru, padding, value);
    }

    if (padding == JTAGPaddingTypes_DRPre) {
        state->active_chain->shift_padding.drPre = value;
    } else if (padding == JTAGPaddingTypes_DRPost) {
        state->active_chain->shift_padding.drPost = value;
    } else if (padding == JTAGPaddingTypes_IRPre) {
        state->active_chain->shift_padding.irPre = value;
    } else if (padding == JTAGPaddingTypes_IRPost) {
        state->active_chain->shift_padding.irPost = value;
    } else {
        syslog(LOG_ERR, "Unknown padding value: %d, slot%d", value,
               state->fru);
        return ST_ERR;
    }
    return ST_OK;
}

//
// Reset the Tap and wait in idle state
//
STATUS JTAG_tap_reset(JTAG_Handler* state)
{
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
#endif

    if (state == NULL)
      return ST_ERR;
    //  ShftDR->TLR = 5  1s of TMS, this will force TAP to enter TLR
    //   from any state
    state->active_chain->tap_state = JtagShfDR;
    return JTAG_set_tap_state(state, JtagTLR);
}


//
// Request the TAP to go to the target state
//
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state)
{
    STATUS ret;

#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -dst_state=%d(", tap_state);
    if (tap_state <= 15) {
        printf("%s)\n", tap_states_name[tap_state]);
    }
    else {
        printf("invalid!!)\n");
    }
#endif

    if (state == NULL)
        return ST_ERR;

    ret = jtag_bic_set_tap_state(state->fru, state->active_chain->tap_state,
                                 tap_state);
    if (ret != ST_OK) {
        syslog(LOG_ERR, "ERROR: %s jtag_bic_set_tap_state failed! slot%d",
               __FUNCTION__, state->fru);
        return ST_ERR;
    }

    // move the [soft] state to the requested tap state.
    state->active_chain->tap_state = tap_state;

#ifdef FBY3_DEBUG
    syslog(LOG_DEBUG, "TapState: %d, slot%d", state->active_chain->tap_state,
           state->fru);
#endif
    return ST_OK;
}

//
// Retrieve the current the TAP state
//
STATUS JTAG_get_tap_state(JTAG_Handler* state, JtagStates* tap_state)
{
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -current state = %d\n", state->active_chain->tap_state);
#endif

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
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -number_of_bits=%d\n", number_of_bits);
    printf("    -input_bytes=%d\n", input_bytes);
    printf("    -output_bytes=%d\n", output_bytes);
    printf("    -end_tap_state=%d(%s)\n", end_tap_state,
                 tap_states_name[end_tap_state]);
#endif


    if (state == NULL)
        return ST_ERR;

    unsigned int preFix = 0;
    unsigned int postFix = 0;
    unsigned char* padData = state->padDataOne;

    if (state->active_chain->tap_state == JtagShfIR) {
        preFix = state->active_chain->shift_padding.irPre;
        postFix = state->active_chain->shift_padding.irPost;
        padData = state->padDataOne;
    } else if (state->active_chain->tap_state == JtagShfDR) {
        preFix = state->active_chain->shift_padding.drPre;
        postFix = state->active_chain->shift_padding.drPost;
        padData = state->padDataZero;
    } else {
        syslog(LOG_ERR, "Shift called but the tap is not in a ShiftIR/DR tap state, slot%d", state->fru);
        return ST_ERR;
    }

    if (state->active_chain->scan_state == JTAGScanState_Done) {
        state->active_chain->scan_state = JTAGScanState_Run;
        if (preFix) {
            if (perform_shift(state, preFix, MAXPADSIZE, padData,
                              0, NULL, state->active_chain->tap_state, state->active_chain->tap_state) != ST_OK)
                 return ST_ERR;
        }
    }

    if ((postFix) && (state->active_chain->tap_state != end_tap_state)) {
        state->active_chain->scan_state = JTAGScanState_Done;
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, state->active_chain->tap_state, state->active_chain->tap_state) != ST_OK)
             return ST_ERR;
        if (perform_shift(state, postFix, MAXPADSIZE, padData, 0,
                            NULL, state->active_chain->tap_state, end_tap_state) != ST_OK)
             return ST_ERR;
    } else {
        if (perform_shift(state, number_of_bits, input_bytes, input,
                          output_bytes, output, state->active_chain->tap_state, end_tap_state) != ST_OK)
             return ST_ERR;
        if (state->active_chain->tap_state != end_tap_state) {
            state->active_chain->scan_state = JTAGScanState_Done;
        }
    }

    return ST_OK;
}



//
//  Optionally write and read the requested number of
//  bits and go to the requested target state
//
static
STATUS perform_shift(JTAG_Handler* state, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     JtagStates current_tap_state, JtagStates end_tap_state)
 {

#ifdef FBY3_DEBUG
    int print_len = MIN(10, input_bytes);

    printf("\n    -%s -number_of_bits=%d", __FUNCTION__, number_of_bits);
    printf("   -input_bytes=%d", input_bytes);
    printf("   -output_bytes=%d\n", output_bytes);
    printf("                   -first %d bytes of data: ", print_len);
    for (int i=0; i<print_len; ++i) {
      printf("0x%02x ", input[i]);
    }
    printf("\n\n");
#endif


    struct scan_xfer scan_xfer = {0};
    scan_xfer.length = number_of_bits;
    scan_xfer.tdi_bytes = input_bytes;
    scan_xfer.tdi = input;
    scan_xfer.tdo_bytes = output_bytes;
    scan_xfer.tdo = output;
    scan_xfer.end_tap_state = end_tap_state;


    if (jtag_bic_read_write_scan(state, &scan_xfer) < 0) {
        syslog(LOG_ERR, "%s: ERROR, BIC_JTAG_READ_WRITE_SCAN failed, slot%d",
               __FUNCTION__, state->fru);
        return ST_ERR;
    }

    // go to end_tap_state as requested
    if (jtag_bic_set_tap_state(state->fru, state->active_chain->tap_state, end_tap_state)) {
        syslog(LOG_ERR, "%s: ERROR, failed to go state %d, slot%d", __FUNCTION__,
               end_tap_state, state->fru);
        return (ST_ERR);
    }

    state->active_chain->tap_state = end_tap_state;




#ifdef FBY3_DEBUG
    {
        unsigned int number_of_bytes = (number_of_bits + 7) / 8;
        const char* shiftStr = (current_tap_state == JtagShfDR) ? "DR" : "IR";
        syslog(LogType_Debug, "%s size: %d", shiftStr, number_of_bits);
        if (input != NULL && number_of_bytes <= input_bytes) {
            syslog_buffer(LogType_Debug, input, number_of_bytes,
                           (current_tap_state == JtagShfDR) ? "DR TDI" : "IR TDI");
        }
        if (output != NULL && number_of_bytes <= output_bytes) {
            syslog_buffer(LogType_Debug, output, number_of_bytes,
                           (current_tap_state == JtagShfDR) ? "DR TDO" : "IR TDO");
        }
        syslog(LogType_Debug, "%s: End tap state: %d", shiftStr, end_tap_state);
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
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -number_of_cycles=%d\n", number_of_cycles);
#endif

    if (state == NULL)
        return ST_ERR;

    if (JTAG_clock_cycle(state->fru, number_of_cycles) != ST_OK) {
            return ST_ERR;
    }

    return ST_OK;
}

STATUS JTAG_set_jtag_tck(JTAG_Handler* state, unsigned int tck)
{
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -tck=%d", tck);
#endif

    if (state == NULL)
        return ST_ERR;

    return ST_OK;
}


static
int jtag_bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                  uint8_t *txbuf, uint8_t txlen,
                  uint8_t *rxbuf, uint8_t *rxlen)
{
    int ret;
#ifdef FBY3_DEBUG
    printf("      -BMC->BIC, slot=%d, netfn=0x%02x, cmd=0x%02x, txbuf=0x", slot_id, netfn, cmd);
    // IANA ID
    printf("%02x%02x%02x ", txbuf[0], txbuf[1], txbuf[2]);

    // actual payload
    for (int i=3; i<txlen; ++i) {
        printf("%02x ", txbuf[i]);
    }
    printf(", txlen=%d\n", txlen);
#endif
    ret = bic_ipmb_wrapper(slot_id, netfn, cmd, txbuf, txlen, rxbuf, rxlen);
    if (ret) {
      syslog(LOG_ERR, "ERROR, jtag_bic_ipmb_wrapper failed, slot%d\n", slot_id);
    }


#ifdef FBY3_DEBUG
    printf("        returned data: rxlen=%d, rxbuf=0x%02x\n", *rxlen, rxbuf[0]);

#ifdef FBY3_IGNORE_ERROR
    if (ret) {
        printf("        ERROR: bic_ipmb_wrapper returns %d, ignore error and continue for now\n", ret);
        ret = 0;
    }
#endif
#endif

    return ret;
}

static
STATUS generateTMSbits(JtagStates src, JtagStates dst, uint8_t *length, uint8_t *tmsbits)
{
    // ensure that src and dst tap states are within 0 to 15.
    if ((src >= sizeof(_tmsCycleLookup[0])/sizeof(_tmsCycleLookup[0][0])) ||  // Column
        (dst >= sizeof(_tmsCycleLookup)/sizeof _tmsCycleLookup[0])) {  // row
        return ST_ERR;
    }

    *length  = _tmsCycleLookup[src][dst].count;
    *tmsbits = _tmsCycleLookup[src][dst].tmsbits;

    return ST_OK;
}

STATUS jtag_bic_set_tap_state(uint8_t slot_id, JtagStates src_state, JtagStates tap_state)
{
    uint8_t tbuf[5] = {0x9c, 0x9c, 0x00}; // IANA ID
    uint8_t rbuf[4] = {0x00};
    uint8_t rlen = 0;
    uint8_t tlen = 5;
    STATUS ret;

    // if we're already are requested state,
    if (src_state == tap_state)
        return ST_OK;

    // tbuf[0:2] = IANA ID
    // tbuf[3]   = tms bit length (max = 8)
    // tbuf[4]   = tmsbits

    // if goal is to set tap to JtagTLR,  send 8 TMS=1 will do it
    if (tap_state == JtagTLR) {
        tbuf[3] = 8;
        tbuf[4] = 0xff;
    }
    // ... otherwise, look up the TMS sequence to go from current state
    // to tap_state
    else {
        ret = generateTMSbits(src_state, tap_state, &(tbuf[3]), &(tbuf[4]));
        if (ret != ST_OK) {
            syslog(LOG_ERR, "ERROR: __%s__ failed to find path from state%d to state%d\n",
                   __FUNCTION__, src_state, tap_state);
            return ret;
        }
    }

    // add delay count for 2 special cases
    if ((tap_state == JtagRTI) || (tap_state == JtagPauDR)) {
        tbuf[3] += 5;
    }

#ifdef FBY3_DEBUG
    printf("      -%s src_state=%d(%s), dst_state=%d(%s), count=%d, tmsbits=0x%02x\n",
           __FUNCTION__, src_state, tap_states_name[src_state],
           tap_state, tap_states_name[tap_state], tbuf[3], tbuf[4]);
#endif

    ret = jtag_bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_TAP_STATE,
                           tbuf, tlen, rbuf, &rlen);

    return ret;
}

static
STATUS jtag_bic_shift_wrapper(uint8_t slot_id, unsigned int write_bit_length,
                              unsigned char* write_data, unsigned int read_bit_length,
                              unsigned char* read_data, unsigned int last_transaction)
{
    uint8_t tbuf[256] = {0x9c, 0x9c, 0x00}; // IANA ID
    uint8_t rbuf[256] = {0x00};
    uint8_t rlen = 0;
    uint8_t tlen = 0;
    STATUS ret;

    // round up to next byte boundary
    uint8_t  write_len_bytes = ((write_bit_length+7) >> 3);

#ifdef FBY3_DEBUG
    printf("        -%s\n", __FUNCTION__);
    printf("              - write_bit_length=%d (0x%02x)\n", write_bit_length, write_bit_length);
    printf("              - read_bit_length=%d (0x%02x)\n", read_bit_length, read_bit_length);
    printf("              - last_transaction=%d\n\n", last_transaction);
#endif
    // tbuf[0:2] = IANA ID
    // tbuf[3]   = write bit length, (LSB)
    // tbuf[4]   = write bit length, (MSB)
    // tbuf[5:n-1] = write data
    // tbuf[n]   = read bit length (LSB)
    // tbuf[n+1] = read bit length (MSB)
    // tbuf[n+2] = last transactions

    tbuf[3] = write_bit_length & 0xFF;
    tbuf[4] = (write_bit_length >> 8) & 0xFF;
    memcpy(&(tbuf[5]), write_data, write_len_bytes);
    tbuf[5 + write_len_bytes] = read_bit_length & 0xFF;
    tbuf[6 + write_len_bytes] = (read_bit_length >> 8) & 0xFF;
    tbuf[7 + write_len_bytes] = last_transaction;

    tlen    = write_len_bytes + 8;  //      write payload
                                    //  + 3 bytes IANA ID
                                    //  + 2 bytes WR length
                                    //  + 2 bytes RD length
                                    //  + 1 byte last_transaction

    ret = jtag_bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_JTAG_SHIFT,
                                tbuf, tlen, rbuf, &rlen);

#ifdef FBY3_DEBUG
    int print_len = MIN(10, (read_bit_length>>3));
    printf("        -%s\n", __FUNCTION__);
    printf("              - ret=%d, rlen=%d\n", ret, rlen);
    printf("              - first %d bytes of return data\n", print_len);
    printf("          ");
    for (int i=0; i<print_len; ++i) {
        printf("0x%02x ", rbuf[i]) ;
    }
    printf("\n\n");
#endif

    if (ret == ST_OK) {
        //Ignore IANA ID
        memcpy(read_data, &rbuf[3], rlen-3);
    }
    return ret;
}

// BIC JTAG driver
static
STATUS jtag_bic_read_write_scan(JTAG_Handler* state, struct scan_xfer *scan_xfer)
{
#define MAX_TRANSFER_BITS  0x400

    int write_bit_length    = (scan_xfer->tdi_bytes)<<3;
    int read_bit_length     = (scan_xfer->tdo_bytes)<<3;
    int transfer_bit_length = scan_xfer->length;
    int last_transaction    = 0;
    int slot_id             = state->fru;
    STATUS ret = ST_OK;
    unsigned char *tdi_buffer, *tdo_buffer;


#ifdef FBY3_DEBUG
    printf("        -%s initial value:\n", __FUNCTION__);
    printf("              - transfer_bit_length=%d\n", transfer_bit_length);
    printf("              - write_bit_length=%d\n", write_bit_length);
    printf("              - read_bit_length =%d\n\n", read_bit_length);
#endif

    if (write_bit_length < transfer_bit_length &&
        read_bit_length < transfer_bit_length)
    {
#ifdef FBY3_DEBUG
        printf("%s: ERROR: illegal input, read(%d)/write(%d) length < transfer length(%d)\n",
               __FUNCTION__, read_bit_length, write_bit_length, transfer_bit_length);
#endif
        syslog(LOG_ERR, "%s: ERROR: illegal input, read(%d)/write(%d) length < transfer length(%d)",
               __FUNCTION__, read_bit_length, write_bit_length, transfer_bit_length);
        return ST_ERR;
    }

    write_bit_length = MIN(write_bit_length, transfer_bit_length);
    read_bit_length  = MIN(read_bit_length, transfer_bit_length);
    tdi_buffer = scan_xfer->tdi;
    tdo_buffer = scan_xfer->tdo;
    while (transfer_bit_length)
    {
        int this_write_bit_length = MIN(write_bit_length, MAX_TRANSFER_BITS);
        int this_read_bit_length  = MIN(read_bit_length, MAX_TRANSFER_BITS);

        // check if we entered illegal state
        if (    this_write_bit_length < 0  || this_read_bit_length < 0
            || last_transaction == 1
            || (this_write_bit_length == 0 && this_read_bit_length ==0) ) {
            syslog(LOG_ERR, "ASD_SP02: slot=%d, ERROR: invalid read write length. read=%d, write=%d, last_transaction=%d\n",
                    slot_id, this_read_bit_length, this_write_bit_length,
                    last_transaction);
            return ST_ERR;
        }

        transfer_bit_length -= MAX(this_write_bit_length, this_read_bit_length);
        if (transfer_bit_length) {
            syslog(LOG_DEBUG, "ASD_SP01: slot=%d, multi loop transfer %d",
                   slot_id, transfer_bit_length);
        }

        write_bit_length -= this_write_bit_length;
        read_bit_length  -= this_read_bit_length;

        last_transaction =     (transfer_bit_length <= 0)
                            && (scan_xfer->end_tap_state != JtagShfDR)
                            && (scan_xfer->end_tap_state != JtagShfIR);

        ret = jtag_bic_shift_wrapper(slot_id, this_write_bit_length, tdi_buffer,
                                     this_read_bit_length, tdo_buffer,
                                     last_transaction);

        if (last_transaction) {
            state->active_chain->tap_state = (state->active_chain->tap_state == JtagShfDR) ? JtagEx1DR : JtagEx1IR;
        }

        tdi_buffer += (this_write_bit_length >> 3);
        tdo_buffer += (this_read_bit_length >> 3);
        if (ret != ST_OK) {
            syslog(LOG_ERR, "%s: ERROR, jtag_bic_shift_wrapper failed, slot%d",
                   __FUNCTION__, slot_id);
            break;
        }
  }

  return ret;
}

STATUS JTAG_set_active_chain(JTAG_Handler* state, scanChain chain) {
    if (state == NULL) {
        return ST_ERR;
    }
    if (pal_is_slot_server(state->fru)) {
        if (chain != SCAN_CHAIN_0) {
            syslog(LOG_ERR, "Invalid chain!.");
            return ST_ERR;
        }
    } else {
        if (chain > MAX_SCAN_CHAINS) {
        syslog(LOG_ERR, "Invalid chain!. %d", chain);
        return ST_ERR;
        }
    }

    state->active_chain = &state->chains[chain];
    return ST_OK;
}

static void asd_sig_handler(int sig)
{
    char sock_path[64];

    if (bic_asd_init(m_slot_id, 0xFF) < 0) {
        syslog(LOG_ERR, "1S_ASD_DEINIT failed, slot%d", m_slot_id);
    }

    sprintf(sock_path, "%s_%d", SOCK_PATH_ASD_BIC, m_slot_id);
    unlink(sock_path);
    sprintf(sock_path, "%s_%d", SOCK_PATH_JTAG_MSG, m_slot_id);
    unlink(sock_path);
}

static void *out_msg_thread(void *arg)
{
    int sock, msgsock, n, len, ret;
    int fru, size, offset = 0;
    size_t t;
    struct sockaddr_un server, client;
    unsigned char *req_buf;
    char sock_path[64] = {0};
    struct spi_message om;
    STATUS (*callback)(struct spi_message *);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "JTAG_MSG: socket() failed\n");
        exit(1);
    }

    fru = ((int *)arg)[0];
    server.sun_family = AF_UNIX;
    sprintf(sock_path, "%s_%d", SOCK_PATH_JTAG_MSG, fru);
    strcpy(server.sun_path, sock_path);
    unlink(server.sun_path);
    len = strlen(server.sun_path) + sizeof(server.sun_family);
    if (bind(sock, (struct sockaddr *)&server, len) == -1) {
        syslog(LOG_ERR, "JTAG_MSG: bind() failed, errno=%d", errno);
        exit(1);
    }

    if (listen (sock, 5) < 0) {
        syslog(LOG_ERR, "JTAG_MSG: listen() failed, errno=%d", errno);
        exit(1);
    }

    if ((om.buffer = (unsigned char *)malloc(4096)) == NULL) {
        syslog(LOG_ERR, "JTAG_MSG: failed to allocate om.buffer");
        exit(1);
    }

    if ((req_buf = (unsigned char *)malloc(4096)) == NULL) {
        syslog(LOG_ERR, "JTAG_MSG: failed to allocate req_buf");
        exit(1);
    }

    callback = (STATUS (*)(struct spi_message *))((int *)arg)[1];
    free(arg);

    while (1) {
        t = sizeof(client);
        if ((msgsock = accept(sock, (struct sockaddr *)&client, &t)) < 0) {
            ret = errno;
            syslog(LOG_WARNING, "JTAG_MSG: accept() failed with ret: %x, errno: %x\n", msgsock, ret);
            sleep(1);
            continue;
        }

        n = recv(msgsock, req_buf, 4096, 0);
        if (n <= 0) {
            syslog(LOG_WARNING, "JTAG_MSG: recv() failed with %d\n", n);
            close(msgsock);
            sleep(1);
            continue;
        }
        close(msgsock);

        switch (req_buf[0]) {
            case 0x02:  // only one package
            case 0x03:  // the first package
                offset = 0;
                memcpy(&om.header, &req_buf[1], sizeof(om.header));
                size = ((om.header.size_msb & 0x1F) << 8) | (om.header.size_lsb & 0xFF);
                if ((size < 0) || (size > 4000)) {
                    syslog(LOG_ERR, "JTAG_MSG: get message size failed %d, slot%d", size, fru);
                    continue;
                }

                if ((size = n - (1+sizeof(om.header))) < 0) {
                    syslog(LOG_ERR, "JTAG_MSG: invalid buffer size %d (n=%d, type=%u), slot%d", size, n, req_buf[0], fru);
                    continue;
                }
                memcpy(om.buffer, &req_buf[1+sizeof(om.header)], size);
                offset += size;
                break;
            case 0x04:  // the middle package
            case 0x05:  // the last package
                if (offset <= 0) {
                    syslog(LOG_ERR, "JTAG_MSG: offset shouldn't be 0, slot%d", fru);
                    continue;
                }

                if ((size = (n-1)) < 0) {
                    syslog(LOG_ERR, "JTAG_MSG: invalid buffer size %d (n=%d, type=%u), slot%d", size, n, req_buf[0], fru);
                    continue;
                }
                memcpy(&om.buffer[offset], &req_buf[1], size);
                offset += size;
                break;
        }

        if ((req_buf[0] == 0x02) || (req_buf[0] == 0x05)) {
            offset = 0;
            callback(&om);
        }
    }

    close(sock);
    pthread_exit(0);
}

STATUS JTAG_init_passthrough(JTAG_Handler *state, uint8_t jflow, STATUS (*callback)(struct spi_message *))
{
    static bool om_thread = false;
    static bool sig_init = false;
    static pthread_t om_tid;
    uint8_t slot_id;
    int *arg;
    struct sigaction sa;

    if (state == NULL) {
        return ST_ERR;
    }
    slot_id = state->fru;

    if (sig_init == false) {
      sa.sa_handler = asd_sig_handler;
      sa.sa_flags = 0;
      sigemptyset(&sa.sa_mask);

      sigaction(SIGHUP, &sa, NULL);
      sigaction(SIGINT, &sa, NULL);
      sigaction(SIGQUIT, &sa, NULL);
      sigaction(SIGSEGV, &sa, NULL);
      sigaction(SIGTERM, &sa, NULL);

      m_slot_id = slot_id;
      sig_init = true;
    }

    if ((jflow == JFLOW_BIC) && (om_thread == false)) {
        if ((arg = malloc(sizeof(int)*2)) == NULL) {
            syslog(LOG_ERR, "%s: malloc failed, fru=%d", __FUNCTION__, slot_id);
            return ST_ERR;
        }
        arg[0] = slot_id;
        arg[1] = (int)callback;

        pthread_create(&om_tid, NULL, out_msg_thread, arg);
        om_thread = true;
    }

    if (bic_asd_init(slot_id, jflow) < 0) {
        syslog(LOG_ERR, "1S_ASD_INIT failed, slot%d", slot_id);
        //return ST_ERR;
    }

    return ST_OK;
}

STATUS passthrough_jtag_message(JTAG_Handler *state, struct spi_message *s_message) {
    uint8_t tbuf[MAX_IPMB_RES_LEN] = {0x9c, 0x9c, 0x00}; // IANA ID
    uint8_t rbuf[8] = {0x00};
    uint8_t rlen = 0;
    uint16_t tlen = 3;
    uint8_t slot_id;
    int size;

    if (state == NULL) {
        return ST_ERR;
    }
    slot_id = state->fru;

    size = ((s_message->header.size_msb & 0x1F) << 8) | (s_message->header.size_lsb & 0xFF);
    if (size < 0) {
        syslog(LOG_ERR, "get message size failed %d, slot%d", size, slot_id);
        return ST_ERR;
    }

    // tbuf[0:2] = IANA ID
    // tbuf[3:n] = message
    memcpy(&tbuf[3], &s_message->header, sizeof(s_message->header));
    memcpy(&tbuf[3+sizeof(s_message->header)], s_message->buffer, size);
    tlen += sizeof(s_message->header) + size;

    if (bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_ASD_MSG_OUT, tbuf, tlen, rbuf, &rlen) < 0) {
        syslog(LOG_ERR, "passthrough_jtag_message failed, slot%d", slot_id);
        return ST_ERR;
    }

    return ST_OK;
}
