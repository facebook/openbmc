/*
Copyright (c) 2019, Intel Corporation
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

#include "jtag_handler.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
// clang-format off
#include <safe_mem_lib.h>
// clang-format on

#include "logging.h"

static const ASD_LogStream stream = ASD_LogStream_JTAG;
static const ASD_LogOption option = ASD_LogOption_None;
static const char* JtagStatesString[] = {
    "TLR",   "RTI",   "SelDR", "CapDR", "ShfDR", "Ex1DR", "PauDR", "Ex2DR",
    "UpdDR", "SelIR", "CapIR", "ShfIR", "Ex1IR", "PauIR", "Ex2IR", "UpdIR"};

#ifdef IPMB_JTAG_HNDLR
#include <signal.h>
#include <openbmc/ipmi.h>

#include "target_handler.h"

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#define MAX_TRANSFER_BITS  0x400
#define IANA_SIZE          3

struct scan_xfer
{
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

static uint8_t m_fruid = 0;

STATUS jtag_ipmb_wrapper(uint8_t fru, uint8_t netfn, uint8_t cmd,
                         uint8_t *txbuf, size_t txlen,
                         uint8_t *rxbuf, size_t *rxlen);

static STATUS jtag_ipmb_asd_init(uint8_t fru, uint8_t cmd)
{
    uint8_t tbuf[8] = {0x15, 0xA0, 0x00};  // IANA
    uint8_t rbuf[8] = {0x00};
    size_t rlen = sizeof(rbuf);

    tbuf[3] = cmd;
    if (jtag_ipmb_wrapper(fru, NETFN_OEM_1S_REQ, CMD_OEM_1S_ASD_INIT,
                          tbuf, 4, rbuf, &rlen) != ST_OK) {
        return ST_ERR;
    }

    return ST_OK;
}

static STATUS jtag_ipmb_shift_wrapper(uint8_t fru, uint32_t write_bit_length,
                                      uint8_t *write_data, uint32_t read_bit_length,
                                      uint8_t *read_data, uint32_t last_transaction)
{
    uint8_t tbuf[256] = {0x15, 0xA0, 0x00};  // IANA
    uint8_t rbuf[256] = {0x00};
    uint8_t write_len_bytes = ((write_bit_length+7) >> 3);
    size_t rlen = sizeof(rbuf);
    size_t tlen = 0;

    // tbuf[0:2]   = IANA ID
    // tbuf[3]     = write bit length (LSB)
    // tbuf[4]     = write bit length (MSB)
    // tbuf[5:n-1] = write data
    // tbuf[n]     = read bit length (LSB)
    // tbuf[n+1]   = read bit length (MSB)
    // tbuf[n+2]   = last transactions
    tbuf[3] = write_bit_length & 0xFF;
    tbuf[4] = (write_bit_length >> 8) & 0xFF;
    if (write_data) {
        memcpy(&tbuf[5], write_data, write_len_bytes);
    }
    tbuf[5 + write_len_bytes] = read_bit_length & 0xFF;
    tbuf[6 + write_len_bytes] = (read_bit_length >> 8) & 0xFF;
    tbuf[7 + write_len_bytes] = last_transaction;

    tlen = write_len_bytes + 8;
    if (jtag_ipmb_wrapper(fru, NETFN_OEM_1S_REQ, CMD_OEM_1S_JTAG_SHIFT,
                          tbuf, tlen, rbuf, &rlen) != ST_OK) {
        return ST_ERR;
    }
    if (read_bit_length && read_data && (rlen > IANA_SIZE)) {
        memcpy(read_data, &rbuf[IANA_SIZE], (rlen - IANA_SIZE));
    }

    return ST_OK;
}

static STATUS jtag_ipmb_read_write_scan(JTAG_Handler *state, struct scan_xfer *scan_xfer)
{
    if (state == NULL || scan_xfer == NULL) {
        return ST_ERR;
    }

    int write_bit_length    = (scan_xfer->tdi_bytes << 3);
    int read_bit_length     = (scan_xfer->tdo_bytes << 3);
    int transfer_bit_length = scan_xfer->length;
    int last_transaction    = 0;
    uint8_t *tdi_buffer, *tdo_buffer;
    uint8_t fru = state->fru;

    if (write_bit_length < transfer_bit_length &&
        read_bit_length < transfer_bit_length) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "illegal input, read(%d)/write(%d) length < transfer length(%d)",
                read_bit_length, write_bit_length, transfer_bit_length);
        return ST_ERR;
    }

    write_bit_length = MIN(write_bit_length, transfer_bit_length);
    read_bit_length  = MIN(read_bit_length, transfer_bit_length);
    tdi_buffer = scan_xfer->tdi;
    tdo_buffer = scan_xfer->tdo;
    while (transfer_bit_length) {
        int this_write_bit_length = MIN(write_bit_length, MAX_TRANSFER_BITS);
        int this_read_bit_length  = MIN(read_bit_length, MAX_TRANSFER_BITS);

        transfer_bit_length -= MAX(this_write_bit_length, this_read_bit_length);
        if (transfer_bit_length) {
            ASD_log(ASD_LogLevel_Info, stream, option,
                    "fru=%u, multi loop transfer %d", fru, transfer_bit_length);
        }

        write_bit_length -= this_write_bit_length;
        read_bit_length  -= this_read_bit_length;

        last_transaction = (transfer_bit_length <= 0) &&
                           (scan_xfer->end_tap_state != jtag_shf_dr) &&
                           (scan_xfer->end_tap_state != jtag_shf_ir);
        if (jtag_ipmb_shift_wrapper(fru, this_write_bit_length, tdi_buffer,
                                    this_read_bit_length, tdo_buffer,
                                    last_transaction) != ST_OK) {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "jtag_ipmb_shift_wrapper failed, fru%u", fru);
            return ST_ERR;
        }

        if (last_transaction) {
            if (state->active_chain->tap_state == jtag_shf_dr) {
                state->active_chain->tap_state = jtag_ex1_dr;
            } else {
                state->active_chain->tap_state = jtag_ex1_ir;
            }
        }
        tdi_buffer += (this_write_bit_length >> 3);
        tdo_buffer += (this_read_bit_length >> 3);
    }

    return ST_OK;
}

static STATUS generateTMSbits(enum jtag_states src, enum jtag_states dst,
                              uint8_t *length, uint8_t *tmsbits)
{
    if (length == NULL || tmsbits == NULL) {
        return ST_ERR;
    }

    // ensure that src and dst tap states are within 0 to 15.
    if ((src >= sizeof(_tmsCycleLookup[0])/sizeof(_tmsCycleLookup[0][0])) ||  // Column
        (dst >= sizeof(_tmsCycleLookup)/sizeof(_tmsCycleLookup[0]))) {        // Row
        return ST_ERR;
    }

    *length  = _tmsCycleLookup[src][dst].count;
    *tmsbits = _tmsCycleLookup[src][dst].tmsbits;

    return ST_OK;
}

static STATUS JTAG_clock_cycle(uint8_t fru, int number_of_cycles)
{
    uint8_t tbuf[8] = {0x15, 0xA0, 0x00};  // IANA
    uint8_t rbuf[8] = {0x00};
    size_t rlen = sizeof(rbuf);

    if (number_of_cycles > 255) {
        number_of_cycles = 255;
    }

    // tbuf[0:2] = IANA ID
    // tbuf[3]   = tms bit length
    // tbuf[4]   = tmsbits
    tbuf[3] = number_of_cycles;
    tbuf[4] = 0x0;
    if (jtag_ipmb_wrapper(fru, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_TAP_STATE,
                          tbuf, 5, rbuf, &rlen) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "wait cycle failed, fru%u", fru);
        return ST_ERR;
    }

    return ST_OK;
}

static void asd_sig_handler(int sig)
{
    char path[32];

    if (jtag_ipmb_asd_init(m_fruid, 0xFF) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "1S_ASD_DEINIT failed, fru%u", m_fruid);
    }

    snprintf(path, sizeof(path), SOCK_PATH_GPIO_EVT, m_fruid);
    unlink(path);
}
#endif

#ifdef JTAG_LEGACY_DRIVER
STATUS JTAG_clock_cycle(int handle, unsigned char tms, unsigned char tdi);
#endif
STATUS perform_shift(JTAG_Handler* state, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     enum jtag_states current_tap_state,
                     enum jtag_states end_tap_state);

void initialize_jtag_chains(JTAG_Handler* state)
{
    for (int i = 0; i < MAX_SCAN_CHAINS; i++)
    {
        state->chains[i].shift_padding.drPre = 0;
        state->chains[i].shift_padding.drPost = 0;
        state->chains[i].shift_padding.irPre = 0;
        state->chains[i].shift_padding.irPost = 0;
        state->chains[i].tap_state = jtag_tlr;
        state->chains[i].scan_state = JTAGScanState_Done;
    }
}

JTAG_Handler* JTAGHandler()
{
    JTAG_Handler* state = (JTAG_Handler*)malloc(sizeof(JTAG_Handler));
    if (state == NULL)
    {
        return NULL;
    }

    state->active_chain = &state->chains[SCAN_CHAIN_0];
    initialize_jtag_chains(state);
    state->sw_mode = true;
    memset_s(state->padDataOne, sizeof(state->padDataOne), ~0,
             sizeof(state->padDataOne));
    explicit_bzero(state->padDataZero, sizeof(state->padDataZero));
    state->JTAG_driver_handle = -1;

    for (unsigned int i = 0; i < MAX_WAIT_CYCLES; i++)
    {
        state->bitbang_data[i].tms = 0;
        state->bitbang_data[i].tdi = 0;
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
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl AST_JTAG_BITBANG failed");
        return ST_ERR;
    }
    return ST_OK;
}
#endif

STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
#ifdef IPMB_JTAG_HNDLR
    static bool sig_init = false;
    struct sigaction sa;
#else
#ifndef JTAG_LEGACY_DRIVER
    struct jtag_mode jtag_mode;
#endif
#endif

    if (state == NULL)
        return ST_ERR;

    state->sw_mode = (state->force_jtag_hw) ? false : sw_mode;
    ASD_log(ASD_LogLevel_Info, stream, option, "JTAG mode set to '%s'.",
            state->sw_mode ? "software" : "hardware");

#ifdef IPMB_JTAG_HNDLR
    if (jtag_ipmb_asd_init(state->fru, state->msg_flow) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "1S_ASD_INIT failed, fru%u", state->fru);
        return ST_ERR;
    }

    if (sig_init == false) {
        m_fruid = state->fru;
        sa.sa_handler = asd_sig_handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGHUP, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGQUIT, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        sig_init = true;
    }
#else
#ifdef JTAG_LEGACY_DRIVER
    state->JTAG_driver_handle = open("/dev/jtag", O_RDWR);
#else
    state->JTAG_driver_handle = open("/dev/jtag0", O_RDWR);
#endif
    if (state->JTAG_driver_handle == -1)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Can't open /dev/jtag, please install driver");
        return ST_ERR;
    }

#ifndef JTAG_LEGACY_DRIVER
    jtag_mode.feature = JTAG_XFER_MODE;
    jtag_mode.mode = sw_mode ? JTAG_XFER_SW_MODE : JTAG_XFER_HW_MODE;
    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCMODE, &jtag_mode))
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed JTAG_SIOCMODE to set xfer mode");
        close(state->JTAG_driver_handle);
        state->JTAG_driver_handle = -1;
        return ST_ERR;
    }
#endif
#endif

    if (JTAG_set_tap_state(state, jtag_tlr) != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to reset tap state.");
#ifndef IPMB_JTAG_HNDLR
        close(state->JTAG_driver_handle);
#endif
        state->JTAG_driver_handle = -1;
        return ST_ERR;
    }

    initialize_jtag_chains(state);
    return ST_OK;
}

STATUS JTAG_deinitialize(JTAG_Handler* state)
{
    if (state == NULL)
        return ST_ERR;

#ifdef IPMB_JTAG_HNDLR
    if (jtag_ipmb_asd_init(state->fru, 0xFF) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "1S_ASD_DEINIT failed, fru%u", state->fru);
    }
#else
    close(state->JTAG_driver_handle);
#endif
    state->JTAG_driver_handle = -1;

    return ST_OK;
}

STATUS JTAG_set_padding(JTAG_Handler* state, const JTAGPaddingTypes padding,
                        const unsigned int value)
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
        ASD_log(ASD_LogLevel_Error, stream, option, "Unknown padding value: %d",
                value);
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
STATUS JTAG_set_tap_state(JTAG_Handler* state, enum jtag_states tap_state)
{
    if (state == NULL)
        return ST_ERR;
#ifdef IPMB_JTAG_HNDLR
    uint8_t fru = state->fru;
    uint8_t tbuf[8] = {0x15, 0xA0, 0x00};  // IANA
    uint8_t rbuf[8] = {0x00};
    size_t rlen = sizeof(rbuf);

    // Jtag state is tap_state already.
    if (tap_state != jtag_tlr && state->active_chain->tap_state == tap_state) {
        return ST_OK;
    }

    if (tap_state == jtag_tlr) {
        tbuf[3] = 8;
        tbuf[4] = 0xff;
    } else {
        // look up the TMS sequence to go from current state to tap_state
        if (generateTMSbits(state->active_chain->tap_state,
                            tap_state, &tbuf[3], &tbuf[4]) != ST_OK) {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to find path from state%d to state%d",
                    state->active_chain->tap_state, tap_state);
        }
    }

    // add delay count for 2 special cases
    if ((tap_state == jtag_rti) || (tap_state == jtag_pau_dr)) {
        tbuf[3] += 5;
    }

    if (jtag_ipmb_wrapper(fru, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_TAP_STATE,
                          tbuf, 5, rbuf, &rlen) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to run CMD_OEM_1S_SET_TAP_STATE");
        return ST_ERR;
    }

    state->active_chain->tap_state = tap_state;
#else
#ifdef JTAG_LEGACY_DRIVER
    struct tap_state_param params;
    params.mode = state->sw_mode ? SW_MODE : HW_MODE;
    params.from_state = state->active_chain->tap_state;
    params.to_state = tap_state;
#else
    struct jtag_tap_state tap_state_t;
    tap_state_t.from = state->active_chain->tap_state;
    tap_state_t.endstate = tap_state;
    tap_state_t.reset =
        (tap_state == jtag_tlr) ? JTAG_FORCE_RESET : JTAG_NO_RESET;
    tap_state_t.tck = 1;
#endif

#ifdef JTAG_LEGACY_DRIVER
    if (ioctl(state->JTAG_driver_handle, AST_JTAG_SET_TAPSTATE, &params)
#else
    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCSTATE, &tap_state_t)
#endif
        < 0)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl AST_JTAG_SET_TAPSTATE failed");
        return ST_ERR;
    }

    state->active_chain->tap_state = tap_state;

    if ((tap_state == jtag_rti) || (tap_state == jtag_pau_dr))
        if (JTAG_wait_cycles(state, 5) != ST_OK)
            return ST_ERR;
#endif

    ASD_log(ASD_LogLevel_Info, stream, option, "Goto state: %s (%d)",
            tap_state >=
                    (sizeof(JtagStatesString) / sizeof(JtagStatesString[0]))
                ? "Unknown"
                : JtagStatesString[tap_state],
            tap_state);
    return ST_OK;
}

//
// Retrieve the current the TAP state
//
STATUS JTAG_get_tap_state(JTAG_Handler* state, enum jtag_states* tap_state)
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
                  enum jtag_states end_tap_state)
{
    if (state == NULL)
        return ST_ERR;

#if !(defined IPMB_JTAG_HNDLR || defined JTAG_LEGACY_DRIVER)
    if (!state->sw_mode && !state->force_jtag_hw)
        return JTAG_shift_hw(state, number_of_bits, input_bytes, input,
                             output_bytes, output, end_tap_state);
#endif

    unsigned int preFix = 0;
    unsigned int postFix = 0;
    unsigned char* padData = NULL;
    enum jtag_states current_state;
    JTAG_get_tap_state(state, &current_state);

    if (current_state == jtag_shf_ir)
    {
        preFix = state->active_chain->shift_padding.irPre;
        postFix = state->active_chain->shift_padding.irPost;
        padData = state->padDataOne;
    }
    else if (current_state == jtag_shf_dr)
    {
        preFix = state->active_chain->shift_padding.drPre;
        postFix = state->active_chain->shift_padding.drPost;
        padData = state->padDataZero;
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Shift called but the tap is not in a ShiftIR/DR tap state");
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

#if !(defined IPMB_JTAG_HNDLR || defined JTAG_LEGACY_DRIVER)
STATUS JTAG_shift_hw(JTAG_Handler* state, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     enum jtag_states end_tap_state)
{
    struct jtag_xfer xfer;
    unsigned char tdio[MAX_DATA_SIZE];
    enum jtag_states current_state;
    union pad_config padding;

    if (state == NULL)
        return ST_ERR;

    JTAG_get_tap_state(state, &current_state);

    padding.int_value = 0;

    if (current_state == jtag_shf_ir)
    {
        padding.pre_pad_number = state->active_chain->shift_padding.irPre;
        padding.post_pad_number = state->active_chain->shift_padding.irPost;
        padding.pad_data = 1;
    }
    else if (current_state == jtag_shf_dr)
    {
        padding.pre_pad_number = state->active_chain->shift_padding.drPre;
        padding.post_pad_number = state->active_chain->shift_padding.drPost;
        padding.pad_data = 0;
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Shift called but the tap is not in a Shift IR/DR tap state");
        return ST_ERR;
    }

    xfer.from = current_state;
    xfer.endstate = end_tap_state;
    xfer.type = (current_state == jtag_shf_ir) ? JTAG_SIR_XFER : JTAG_SDR_XFER;
    xfer.length = number_of_bits;
    xfer.direction = JTAG_READ_WRITE_XFER;

    if (state->active_chain->scan_state == JTAGScanState_Done)
    {
        state->active_chain->scan_state = JTAGScanState_Run;
    }
    else
    {
        padding.pre_pad_number = 0;
    }

    if (current_state == end_tap_state)
    {
        padding.post_pad_number = 0;
    }
    else
    {
        state->active_chain->scan_state = JTAGScanState_Done;
    }

    xfer.padding = padding.int_value;

    if (output != NULL)
    {
        if (memcpy_s(output, output_bytes, input,
                     DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE)))
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "memcpy_s: input to output copy buffer failed.");
        }
        xfer.tdio = (__u64)output;
    }
    else
    {
        if (memcpy_s(tdio, DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE), input,
                     DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE)))
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "memcpy_s: input to tdio buffer copy failed.");
        }
        xfer.tdio = (__u64)tdio;
    }
    if (ioctl(state->JTAG_driver_handle, JTAG_IOCXFER, &xfer) < 0)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl JTAG_IOCXFER failed");
        return ST_ERR;
    }

    state->active_chain->tap_state = end_tap_state;

#ifdef ENABLE_DEBUG_LOGGING
    if (padding.pre_pad_number)
        ASD_log(ASD_LogLevel_Debug, stream, option, "PrePadConfig = 0x%x",
                xfer.padding);
    if (input != NULL)
        ASD_log_shift(ASD_LogLevel_Debug, stream, option, number_of_bits,
                      input_bytes, input,
                      (current_state == jtag_shf_dr) ? "Shift DR TDI"
                                                     : "Shift IR TDI");
    if (output != NULL)
        ASD_log_shift(ASD_LogLevel_Debug, stream, option, number_of_bits,
                      output_bytes, output,
                      (current_state == jtag_shf_dr) ? "Shift DR TDO"
                                                     : "Shift IR TDO");
    if (padding.post_pad_number)
        ASD_log(ASD_LogLevel_Debug, stream, option, "PostPadConfig = 0x%x",
                xfer.padding);
#endif
    return ST_OK;
}
#endif

//
//  Optionally write and read the requested number of
//  bits and go to the requested target state
//
STATUS perform_shift(JTAG_Handler* state, unsigned int number_of_bits,
                     unsigned int input_bytes, unsigned char* input,
                     unsigned int output_bytes, unsigned char* output,
                     enum jtag_states current_tap_state,
                     enum jtag_states end_tap_state)
{
#if defined IPMB_JTAG_HNDLR || defined JTAG_LEGACY_DRIVER
    struct scan_xfer scan_xfer;
    scan_xfer.mode = state->sw_mode ? SW_MODE : HW_MODE;
    scan_xfer.tap_state = current_tap_state;
    scan_xfer.length = number_of_bits;
    scan_xfer.tdi_bytes = input_bytes;
    scan_xfer.tdi = input;
    scan_xfer.tdo_bytes = output_bytes;
    scan_xfer.tdo = output;
    scan_xfer.end_tap_state = end_tap_state;

#ifdef IPMB_JTAG_HNDLR
    if (jtag_ipmb_read_write_scan(state, &scan_xfer) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ERROR, jtag_ipmb_read_write_scan failed");
        return ST_ERR;
    }

    // go to end_tap_state as requested
    if (JTAG_set_tap_state(state, end_tap_state) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ERROR, failed to go state %d,", end_tap_state);
        return ST_ERR;
    }
#else
    if (ioctl(state->JTAG_driver_handle, AST_JTAG_READWRITESCAN, &scan_xfer) <
        0)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl AST_JTAG_READWRITESCAN failed.");
        return ST_ERR;
    }
#endif
#else
    struct jtag_xfer xfer;
    unsigned char tdio[MAX_DATA_SIZE];

    xfer.from = current_tap_state;
    xfer.endstate = end_tap_state;
    xfer.type =
        (current_tap_state == jtag_shf_ir) ? JTAG_SIR_XFER : JTAG_SDR_XFER;
    xfer.length = number_of_bits;
    xfer.direction = JTAG_READ_WRITE_XFER;

    if (output != NULL)
    {
        if (memcpy_s(output, output_bytes, input,
                     DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE)))
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "memcpy_s: input to output copy buffer failed.");
        }
        xfer.tdio = (__u64)output;
    }
    else
    {
        if (memcpy_s(tdio, DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE), input,
                     DIV_ROUND_UP(number_of_bits, BITS_PER_BYTE)))
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "memcpy_s: input to tdio buffer copy failed.");
        }
        xfer.tdio = (__u64)tdio;
    }
    if (ioctl(state->JTAG_driver_handle, JTAG_IOCXFER, &xfer) < 0)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl JTAG_IOCXFER failed");
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
    if (state == NULL)
        return ST_ERR;

    // Execute wait cycles in SW and HW mode
    ASD_log(ASD_LogLevel_Debug, stream, option, "Wait %d cycles",
            number_of_cycles);

#ifdef IPMB_JTAG_HNDLR
    if (JTAG_clock_cycle(state->fru, number_of_cycles) != ST_OK) {
        return ST_ERR;
    }
#elif defined JTAG_LEGACY_DRIVER
    if (state->sw_mode)
    {
        for (unsigned int i = 0; i < number_of_cycles; i++)
        {
            if (JTAG_clock_cycle(state->JTAG_driver_handle, 0, 0) != ST_OK)
                return ST_ERR;
        }
    }
#else
    struct bitbang_packet bitbang = {NULL, 0};

    if (state == NULL)
        return ST_ERR;

    if (number_of_cycles > MAX_WAIT_CYCLES)
        return ST_ERR;

    // Execute wait cycles in SW and HW mode
    ASD_log(ASD_LogLevel_Debug, stream, option, "Wait %d cycles",
            number_of_cycles);

    bitbang.data = state->bitbang_data;
    bitbang.length = number_of_cycles;

    if (ioctl(state->JTAG_driver_handle, JTAG_IOCBITBANG, &bitbang) < 0)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl JTAG_IOCBITBANG failed");
        return ST_ERR;
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
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl AST_JTAG_SET_TCK failed");
        return ST_ERR;
    }
#else
#ifndef IPMB_JTAG_HNDLR
    unsigned int frq = APB_FREQ / tck;

    if (ioctl(state->JTAG_driver_handle, JTAG_SIOCFREQ, &frq) < 0)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ioctl JTAG_SIOCFREQ failed");
        return ST_ERR;
    }
#endif
#endif
    return ST_OK;
}

STATUS JTAG_set_active_chain(JTAG_Handler* state, scanChain chain)
{
    if (state == NULL)
        return ST_ERR;

    if (chain < 0 || chain >= MAX_SCAN_CHAINS)
    {
        ASD_log(ASD_LogLevel_Error, stream, option, "Invalid scan chain.");
        return ST_ERR;
    }

    state->active_chain = &state->chains[chain];

    return ST_OK;
}
