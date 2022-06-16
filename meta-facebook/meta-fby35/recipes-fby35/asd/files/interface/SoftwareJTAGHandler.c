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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>
#include "SoftwareJTAGHandler.h"


// Enable FBY35-specific debug messages
//#define FBY35_DEBUG

// ignore errors communicating with BIC
//#define FBY35_IGNORE_ERROR

typedef uint8_t __u8;
typedef uint32_t __u32;

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

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

struct scan_xfer {
    __u8 mode;
    __u32 tap_state;
    __u32 length;
    __u8* tdi;
    __u32 tdi_bytes;
    __u8* tdo;
    __u32 tdo_bytes;
    __u32 end_tap_state;
} __attribute__((__packed__));

typedef struct {
    uint8_t tmsbits;
    uint8_t count;
} TmsCycle;

// this is the complete set TMS cycles for going from any TAP state to any other TAP state, following a “shortest path” rule
static const TmsCycle _tmsCycleLookup[][16] = {
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


// debug helper functions
#ifdef FBY35_DEBUG
static const char *tap_states_name[] = {
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

static void print_jtag_state(const char *func, JTAG_Handler *state)
{
    if (state == NULL) {
        printf("Error: null passed to print_state\n");
    } else if (state->active_chain->tap_state > 15) {
        printf("Error! tap_state=%d(invalid!)\n", state->active_chain->tap_state);
    } else {
        printf("\n%s\n    -JTAG.tap_state=%d(%s), active_chain->shift_padding.drPre=%d, active_chain->shift_padding.drPost\
=%d, active_chain->shift_padding.irPre=%d, active_chain->shift_padding.irPost=%d scan_state=%d slot_id=%d\n", func,
               state->active_chain->tap_state,
               tap_states_name[state->active_chain->tap_state],
               state->active_chain->shift_padding.drPre,
               state->active_chain->shift_padding.drPost,
               state->active_chain->shift_padding.irPre,
               state->active_chain->shift_padding.irPost,
               state->active_chain->scan_state,
               state->fru);
    }
}
#endif

static
STATUS jtag_bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                             uint8_t *txbuf, uint8_t txlen,
                             uint8_t *rxbuf, uint8_t *rxlen)
{
    STATUS ret = ST_ERR;

#ifdef FBY35_DEBUG
    printf("      -BMC->BIC, slot=%u, netfn=0x%02x, cmd=0x%02x, txbuf=0x", slot_id, netfn, cmd);
    // IANA
    printf("%02x%02x%02x ", txbuf[0], txbuf[1], txbuf[2]);

    // DATA
    for (int i=3; i<txlen; ++i) {
        printf("%02x ", txbuf[i]);
    }
    printf(", txlen=%d\n", txlen);
#endif
    if (bic_ipmb_wrapper(slot_id, netfn, cmd, txbuf, txlen, rxbuf, rxlen)) {
        syslog(LOG_ERR, "ERROR, bic_ipmb_wrapper failed, slot%u", slot_id);
        ret = ST_ERR;
    } else ret = ST_OK;

#ifdef FBY35_DEBUG
    printf("        returned data: rxlen=%d, rxbuf=0x%02x\n", *rxlen, rxbuf[0]);
#ifdef FBY35_IGNORE_ERROR
    if (ret) {
        printf("        ERROR: bic_ipmb_wrapper returns %d, ignore error and continue for now\n", ret);
        ret = ST_OK;
    }
#endif
#endif
    return ret;
}

static
STATUS jtag_bic_shift_wrapper(uint8_t slot_id, uint32_t write_bit_length,
                              uint8_t* write_data, uint32_t read_bit_length,
                              uint8_t* read_data, uint32_t last_transaction)
{
    uint8_t tbuf[256] = {0x00};
    uint8_t rbuf[256] = {0x00};
    uint8_t rlen = 0;
    uint8_t tlen = 0;
    STATUS ret = ST_ERR;

    // Fill the IANA ID
    memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);

    // round up to next byte boundary
    uint8_t  write_len_bytes = ((write_bit_length+7) >> 3);

#ifdef FBY35_DEBUG
    printf("        -%s\n", __FUNCTION__);
    printf("              - write_bit_length=%u (0x%02x)\n", write_bit_length, write_bit_length);
    printf("              - read_bit_length=%u (0x%02x)\n", read_bit_length, read_bit_length);
    printf("              - last_transaction=%u\n\n", last_transaction);
#endif
    /*
    tbuf[0:2] = IANA ID
    tbuf[3]   = write bit length, (LSB)
    tbuf[4]   = write bit length, (MSB)
    tbuf[5:n-1] = write data
    tbuf[n]   = read bit length (LSB)
    tbuf[n+1] = read bit length (MSB)
    tbuf[n+2] = last transactions
    */
    tbuf[3] = write_bit_length & 0xFF;
    tbuf[4] = (write_bit_length >> 8) & 0xFF;
    memcpy(&(tbuf[5]), write_data, write_len_bytes);
    tbuf[5 + write_len_bytes] = read_bit_length & 0xFF;
    tbuf[6 + write_len_bytes] = (read_bit_length >> 8) & 0xFF;
    tbuf[7 + write_len_bytes] = last_transaction;

    tlen    = write_len_bytes + 8;    /*    write payload
                                      + 3 bytes IANA ID
                                      + 2 bytes WR length
                                      + 2 bytes RD length
                                      + 1 byte last_transaction
                                      */
    ret = jtag_bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_JTAG_SHIFT,
                                tbuf, tlen, rbuf, &rlen);

#ifdef FBY35_DEBUG
    int print_len = MIN(10, (read_bit_length>>3));
    printf("        -%s\n", __FUNCTION__);
    printf("              - ret=%d, rlen=%d\n", ret, rlen);
    printf("              - first %d bytes of return data\n", print_len);
    printf("          ");
    for (int i=0; i<print_len; ++i) {
        printf("0x%02x ", rbuf[i]);
    }
    printf("\n\n");
#endif
    if (ret == ST_OK) {
        // Ignore IANA ID
        memcpy(read_data, &rbuf[3], rlen-3);
    }
    return ret;
}

static
STATUS jtag_bic_read_write_scan(JTAG_Handler* state, struct scan_xfer *scan_xfer)
{
#define MAX_TRANSFER_BITS  0x400

    int write_bit_length    = (scan_xfer->tdi_bytes)<<3;
    int read_bit_length     = (scan_xfer->tdo_bytes)<<3;
    int transfer_bit_length = scan_xfer->length;
    int last_transaction    = 0;
    STATUS ret = ST_OK;
    uint8_t *tdi_buffer, *tdo_buffer;
    uint8_t slot_id = state->fru;

#ifdef FBY35_DEBUG
    printf("        -%s initial value:\n", __FUNCTION__);
    printf("              - transfer_bit_length=%d\n", transfer_bit_length);
    printf("              - write_bit_length=%d\n", write_bit_length);
    printf("              - read_bit_length =%d\n\n", read_bit_length);
#endif
    if (write_bit_length < transfer_bit_length &&
        read_bit_length < transfer_bit_length) {
        printf("%s: ERROR: illegal input, read(%d)/write(%d) length < transfer length(%d)\n",
               __FUNCTION__, read_bit_length, write_bit_length, transfer_bit_length);
        return ST_ERR;
    }

    write_bit_length = MIN(write_bit_length, transfer_bit_length);
    read_bit_length  = MIN(read_bit_length, transfer_bit_length);
    tdi_buffer = scan_xfer->tdi;
    tdo_buffer = scan_xfer->tdo;
    while (transfer_bit_length) {
        int this_write_bit_length = MIN(write_bit_length, MAX_TRANSFER_BITS);
        int this_read_bit_length  = MIN(read_bit_length, MAX_TRANSFER_BITS);

        // check if we entered illegal state
        if (this_write_bit_length < 0 ||
            this_read_bit_length < 0 ||
            last_transaction == 1 ||
            (this_write_bit_length == 0 && this_read_bit_length == 0)) {
            printf("ASD_SP02: slot=%u, ERROR: invalid read write length. read=%d, write=%d, last_transaction=%d\n",
                   slot_id, this_read_bit_length, this_write_bit_length,
                   last_transaction);
            return ST_ERR;
        }

        transfer_bit_length -= MAX(this_write_bit_length, this_read_bit_length);
        if (transfer_bit_length) {
            printf("ASD_SP01: slot=%u, multi loop transfer %d",
                   slot_id, transfer_bit_length);
        }

        write_bit_length -= this_write_bit_length;
        read_bit_length  -= this_read_bit_length;

        last_transaction = (transfer_bit_length <= 0) &&
                           (scan_xfer->end_tap_state != jtag_shf_dr) &&
                           (scan_xfer->end_tap_state != jtag_shf_ir);
        ret = jtag_bic_shift_wrapper(slot_id, this_write_bit_length, tdi_buffer,
                                     this_read_bit_length, tdo_buffer,
                                     last_transaction);

        if (last_transaction) {
            state->active_chain->tap_state = (state->active_chain->tap_state == jtag_shf_dr) ? jtag_ex1_dr : jtag_ex1_ir;
        }

        tdi_buffer += (this_write_bit_length >> 3);
        tdo_buffer += (this_read_bit_length >> 3);
        if (ret != ST_OK) {
            syslog(LOG_ERR, "%s: ERROR, jtag_bic_shift_wrapper failed, slot%u",
                   __FUNCTION__, slot_id);
            break;
        }
  }

  return ret;
}

static
STATUS generateTMSbits(JtagStates src, JtagStates dst, uint8_t *length, uint8_t *tmsbits)
{
    // ensure that src and dst tap states are within 0 to 15.
    if ((src >= sizeof(_tmsCycleLookup[0])/sizeof(_tmsCycleLookup[0][0])) ||  // Column
        (dst >= sizeof(_tmsCycleLookup)/sizeof(_tmsCycleLookup[0]))) {        // Row
        return ST_ERR;
    }

    *length  = _tmsCycleLookup[src][dst].count;
    *tmsbits = _tmsCycleLookup[src][dst].tmsbits;

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

    return state;
}

STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
    scanChain chain;

#ifdef FBY35_DEBUG
    print_jtag_state(__FUNCTION__, state);
#endif
    if (state == NULL)
        return ST_ERR;

    chain = (state->fru & 0x80) ? SCAN_CHAIN_1 : SCAN_CHAIN_0;
    state->fru &= 0x7F;

    if (bic_asd_init(state->fru, JFLOW_BMC) < 0) {
        syslog(LOG_ERR, "1S_ASD_INIT failed, slot%d", state->fru);
        //return ST_ERR;
    }

    if (bic_set_gpio(state->fru, BMC_JTAG_SEL_R, 1)) {
        syslog(LOG_ERR, "Setting BMC_JTAG_SEL_R failed, slot%d", state->fru);
    }

    if (bic_set_gpio(state->fru, FM_JTAG_TCK_MUX_SEL_R, chain)) {
        syslog(LOG_ERR, "Setting FM_JTAG_TCK_MUX_SEL_R failed, slot%d",
               state->fru);
    }

    if (JTAG_set_tap_state(state, jtag_tlr) != ST_OK) {
        syslog(LOG_ERR, "Failed to reset tap state. slot%d", state->fru);
        //close(state->JTAG_driver_handle);
        state->JTAG_driver_handle = -1;
        return ST_ERR;
    }

    initialize_jtag_chains(state);
    return ST_OK;
}

STATUS JTAG_deinitialize(JTAG_Handler* state)
{
#ifdef FBY35_DEBUG
    print_jtag_state(__FUNCTION__, state);
#endif
    if (state == NULL)
        return ST_ERR;

    if (bic_asd_init(state->fru, 0xFF) < 0) {
        syslog(LOG_ERR, "1S_ASD_DEINIT failed, slot%d", state->fru);
    }
    //close(state->JTAG_driver_handle);
    state->JTAG_driver_handle = -1;

    return ST_OK;
}

//
// Request the TAP to go to the target state
//
STATUS JTAG_set_tap_state(JTAG_Handler* state, JtagStates tap_state)
{
    if (state == NULL)
        return ST_ERR;

    uint8_t tbuf[5] = {0x00}; // IANA ID
    uint8_t rbuf[5] = {0x00};
    uint8_t rlen = 0;
    uint8_t tlen = 5;
    uint8_t slot_id = state->fru;
    STATUS ret = ST_ERR;

    // Fill the IANA ID
    memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);

#ifdef FBY35_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -dst_state=%d(", tap_state);
    if (tap_state <= 15) {
        printf("%s)\n", tap_states_name[tap_state]);
    } else {
        printf("invalid!!)\n");
    }
#endif
    // Jtag state is tap_state already.
    if (tap_state != jtag_tlr && state->active_chain->tap_state == tap_state) {
        return ST_OK;
    }

    if (tap_state == jtag_tlr) {
        tbuf[3] = 8;
        tbuf[4] = 0xff;
    } else {
        // look up the TMS sequence to go from current state to tap_state
        ret = generateTMSbits(state->active_chain->tap_state,
                              tap_state, &(tbuf[3]), &(tbuf[4]));
        if (ret != ST_OK) {
            syslog(LOG_ERR, "ERROR: __%s__ Failed to find path from state%d to state%d",
                   __FUNCTION__, state->active_chain->tap_state, tap_state);
        }
    }

    // add delay count for 2 special cases
    if ((tap_state == jtag_rti) || (tap_state == jtag_pau_dr)) {
        tbuf[3] += 5;
    }

    ret = jtag_bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_TAP_STATE,
                                tbuf, tlen, rbuf, &rlen);
    if (ret != ST_OK) {
        syslog(LOG_ERR, "ERROR: %s Failed to run CMD_OEM_1S_SET_TAP_STATE. slot%d",
               __FUNCTION__, state->fru);
        return ST_ERR;
    }

    state->active_chain->tap_state = tap_state;

#ifdef FBY35_DEBUG
    syslog(LOG_DEBUG, "TapState: %d, slot%d", state->active_chain->tap_state,
           state->fru);
#endif
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
#ifdef FBY35_DEBUG
    int print_len = MIN(10, input_bytes);

    printf("\n    -%s -number_of_bits=%u", __FUNCTION__, number_of_bits);
    printf("    -input_bytes=%u", input_bytes);
    printf("    -output_bytes=%u\n", output_bytes);
    printf("                -first %d bytes of data: ", print_len);
    for (int i=0; i<print_len; ++i) {
      printf("0x%02x ", input[i]);
    }
    printf("\n\n");
#endif
    struct scan_xfer scan_xfer;
    //scan_xfer.mode = state->sw_mode ? SW_MODE : HW_MODE;
    scan_xfer.tap_state = current_tap_state;
    scan_xfer.length = number_of_bits;
    scan_xfer.tdi_bytes = input_bytes;
    scan_xfer.tdi = input;
    scan_xfer.tdo_bytes = output_bytes;
    scan_xfer.tdo = output;
    scan_xfer.end_tap_state = end_tap_state;

    if (jtag_bic_read_write_scan(state, &scan_xfer) != ST_OK) {
        syslog(LOG_ERR, "%s: ERROR, jtag_bic_read_write_scan failed, slot%d",
               __FUNCTION__, state->fru);
        return ST_ERR;
    }

    // go to end_tap_state as requested
    if (JTAG_set_tap_state(state, end_tap_state) != ST_OK) {
        syslog(LOG_ERR, "%s: ERROR, failed to go state %d, slot%d", __FUNCTION__,
               end_tap_state, state->fru);
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
#ifdef FBY35_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -number_of_bits=%u\n", number_of_bits);
    printf("    -input_bytes=%u\n", input_bytes);
    printf("    -output_bytes=%u\n", output_bytes);
    printf("    -end_tap_state=%d(%s)\n", end_tap_state,
                 tap_states_name[end_tap_state]);
#endif
    if (state == NULL)
        return ST_ERR;

    unsigned int preFix = 0;
    unsigned int postFix = 0;
    unsigned char* padData = NULL;
    JtagStates current_state;
    JTAG_get_tap_state(state, &current_state);

    if (current_state == jtag_shf_ir) {
        preFix = state->active_chain->shift_padding.irPre;
        postFix = state->active_chain->shift_padding.irPost;
        padData = state->padDataOne;
    } else if (current_state == jtag_shf_dr) {
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
#ifdef FBY3_DEBUG
    print_jtag_state(__FUNCTION__, state);
    printf("    -current state = %d\n", state->active_chain->tap_state);
#endif
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
