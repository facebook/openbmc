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

#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

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

static const ASD_LogStream stream = ASD_LogStream_JTAG;
static const ASD_LogOption option = ASD_LogOption_None;
static const char* JtagStatesString[] = {
    "TLR",   "RTI",   "SelDR", "CapDR", "ShfDR", "Ex1DR", "PauDR", "Ex2DR",
    "UpdDR", "SelIR", "CapIR", "ShfIR", "Ex1IR", "PauIR", "Ex2IR", "UpdIR"};

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

static int m_slot_id = -1;
static void *m_state = NULL;

static void asd_sig_handler(int sig)
{
    char sock_path[64];

    if (bic_asd_init(m_slot_id, 0xFF) < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "1S_ASD_DEINIT failed, slot%d", m_slot_id);
    }

    sprintf(sock_path, "%s_%d", SOCK_PATH_ASD_BIC, m_slot_id);
    unlink(sock_path);
    sprintf(sock_path, "%s_%d", SOCK_PATH_JTAG_MSG, m_slot_id);
    unlink(sock_path);
}

static void *out_msg_thread(void *arg)
{
    int sock, msgsock, recv_size, len, ret;
    int fru, size, offset = 0;
    size_t t;
    struct sockaddr_un server, client;
    uint8_t req_buf[MAX_PACKET_SIZE] = {0};
    uint8_t res_buf[MAX_PACKET_SIZE] = {0};
    char sock_path[64] = {0};
    struct asd_message *msg;
    STATUS (*callback)(void* state, unsigned char* buffer, size_t length);

    // create the socket to recevie msgs from BIC
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "JTAG_MSG: socket() failed\n");
        exit(1);
    }

    // get the fru id
    fru = ((int *)arg)[0];
    printf("fru%d out_msg_thread started...\n", fru);
    server.sun_family = AF_UNIX;
    sprintf(sock_path, "%s_%d", SOCK_PATH_JTAG_MSG, fru);
    strcpy(server.sun_path, sock_path);
    unlink(server.sun_path);
    len = strlen(server.sun_path) + sizeof(server.sun_family);
    if (bind(sock, (struct sockaddr *)&server, len) == -1) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "JTAG_MSG: bind() failed, errno=%d", errno);
        exit(1);
    }

    if (listen(sock, 5) < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "JTAG_MSG: listen() failed, errno=%d", errno);
        exit(1);
    }

    callback = (STATUS (*)(void* state, unsigned char* buffer, size_t length))((int *)arg)[1];

    // release it
    free(arg);

    // start
    while (1) {
        t = sizeof(client);
        if ((msgsock = accept(sock, (struct sockaddr *)&client, &t)) < 0) {
            ret = errno;
            ASD_log(ASD_LogLevel_Error,stream, option,
                    "JTAG_MSG: accept() failed with ret: %x, errno: %x\n", msgsock, ret);
            sleep(1);
            continue;
        }

        // receive msgs
        recv_size = recv(msgsock, req_buf, MAX_PACKET_SIZE, 0);
        if (recv_size <= 0) {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "JTAG_MSG: recv() failed with %d\n", recv_size);
            close(msgsock);
            sleep(1);
            continue;
        }
        close(msgsock);

        printf("recv: %d, size: %d, data ", req_buf[0], recv_size);
        for (int i = 0; i< recv_size; i++) {
            printf("%02X ", req_buf[i]);
        }
        printf("\n");

        switch (req_buf[0]) {
            case 0x02:  // only one package
            case 0x03:  // the first package
                offset = 0;
                msg = (struct asd_message *)&req_buf[1];

                // copy header
                if (memcpy_s(&res_buf, MAX_PACKET_SIZE,
                    &req_buf[1], HEADER_SIZE)) {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                            "memcpy_s: message header to send_buffer copy failed.");
                }

                // debug
                for (int i = 0; i < HEADER_SIZE; i++) {
                    printf("res_buf[%d]=%02X\n", i, res_buf[i]);
                }

                // get data size
                size = ((msg->header.size_msb & 0x1F) << 8) | (msg->header.size_lsb & 0xFF);
                if (size > MAX_PACKET_SIZE) {
                    ASD_log(ASD_LogLevel_Error, stream, option,
                            "JTAG_MSG: msg size is unexpected! size=%d, slot%d", size, fru);
                            continue;
                }
                printf("size=%d\n", size);

                // get the size of segmant data
                size = recv_size - HEADER_SIZE - 1;

                // copy data
                if (size > 0 &&
                    memcpy_s(&res_buf[HEADER_SIZE], MAX_PACKET_SIZE - HEADER_SIZE,
                    msg->buffer, (size_t)size)) {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                            "memcpy_s: message buffer to send buffer offset copy failed.");
                }

                // record the size/offset because we may have the other packets
                offset = recv_size - 1;

                // debug
                printf("[%d]print offset: ", offset);
                for (int i = 0; i < offset; i++) {
                    printf("%02X ", res_buf[i]);
                }
                printf("\n");
                break;
            case 0x04:  // the middle package
            case 0x05:  // the last package
                // check offset. it should be greater than 0 since the first packet is sent.
                if (offset <= 0) {
                    ASD_log(ASD_LogLevel_Error, stream, option,
                            "JTAG_MSG: offset shouldn't be 0, slot%d", fru);
                    continue;
                }

                // get the actual data size
                // the first byte is used to identify packets, recv_size - 1 = data_size
                if ((size = (recv_size - 1)) < 0) {
                    ASD_log(ASD_LogLevel_Error, stream, option,
                            "JTAG_MSG: invalid buffer size %d (n=%d, type=%u), slot%d", size, recv_size, req_buf[0], fru);
                    continue;
                }

                memcpy(&res_buf[offset], &req_buf[1], size);
                offset += size;
                break;
        }
        if ((req_buf[0] == 0x02) || (req_buf[0] == 0x05)) {
            // send response
            printf("Callback: ");
            for (int i = 0; i < offset; i++) {
                printf("%02X ", res_buf[i]);
            }
            printf("\n");
            callback(m_state, res_buf, offset);
            offset = 0;
        }
    }

    close(sock);
    pthread_exit(0);
}

STATUS init_passthrough_path(void *state, uint8_t fru,
                             STATUS (*send_back_to_client)(void* state, unsigned char* buffer, size_t length))
{
    static bool om_thread = false;
    static bool sig_init = false;
    static pthread_t om_tid;
    uint8_t slot_id = fru;
    int *arg;
    struct sigaction sa;

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

    m_state = state;

    if (om_thread == false) {
        if ((arg = malloc(sizeof(int)*2)) == NULL) {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "%s: malloc failed, fru=%d", __func__, slot_id);
            return ST_ERR;
        }
        arg[0] = slot_id;
        arg[1] = (int)send_back_to_client;

        pthread_create(&om_tid, NULL, out_msg_thread, arg);
        om_thread = true;
    }
    return ST_OK;
}

static
STATUS jtag_bic_ipmb_wrapper(uint8_t slot_id, uint8_t netfn, uint8_t cmd,
                             uint8_t *txbuf, uint8_t txlen,
                             uint8_t *rxbuf, uint8_t *rxlen)
{
    STATUS ret = ST_ERR;

    if (bic_ipmb_wrapper(slot_id, netfn, cmd, txbuf, txlen, rxbuf, rxlen)) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ERROR, bic_ipmb_wrapper failed, slot%d\n", slot_id);
        ret = ST_ERR;
    } else ret = ST_OK;

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
            printf("ASD_SP02: slot=%d, ERROR: invalid read write length. read=%d, write=%d, last_transaction=%d\n",
                   slot_id, this_read_bit_length, this_write_bit_length,
                   last_transaction);
            return ST_ERR;
        }

        transfer_bit_length -= MAX(this_write_bit_length, this_read_bit_length);
        if (transfer_bit_length) {
            printf("ASD_SP01: slot=%d, multi loop transfer %d",
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
            ASD_log(ASD_LogLevel_Info, stream, option,
                    "ERROR, jtag_bic_shift_wrapper failed, slot%d", slot_id);
            break;
        }
  }

  return ret;
}

static
STATUS generateTMSbits(enum jtag_states src, enum jtag_states dst, uint8_t *length, uint8_t *tmsbits)
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
STATUS JTAG_clock_cycle(uint8_t slot_id, int number_of_cycles)
{
    uint8_t tbuf[5] = {0x00}; // IANA ID
    uint8_t rbuf[4] = {0x00};
    uint8_t rlen = 0;
    uint8_t tlen = 5;

    // Fill the IANA ID
    memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);

    if (number_of_cycles > 256) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ASD: delay cycle = %d(> 256). slot%d", number_of_cycles, slot_id);
        number_of_cycles = 255;
    } else if (number_of_cycles == 256) {
        number_of_cycles = 255;
    }

    // tbuf[0:2] = IANA ID
    // tbuf[3]   = tms bit length
    // tbuf[4]   = tmsbits
    tbuf[3] = number_of_cycles;
    tbuf[4] = 0x0;
    if (jtag_bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_TAP_STATE,
                              tbuf, tlen, rbuf, &rlen) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "wait cycle failed, slot%d", slot_id);
        return ST_ERR;
    }

    return ST_OK;
}

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

STATUS JTAG_initialize(JTAG_Handler* state, bool sw_mode)
{
    if (state == NULL)
        return ST_ERR;

    state->sw_mode = (state->force_jtag_hw) ? false : sw_mode;
    ASD_log(ASD_LogLevel_Info, stream, option, "JTAG mode set to '%s'.",
            state->sw_mode ? "software" : "hardware");

    if (bic_asd_init(state->fru, state->msg_flow) < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "1S_ASD_INIT failed, slot%d", state->fru);
        // TODO: current BIC has not yet implement this command,
        // (the command is to enable BIC send GPIO interrupt message)
        // so don't break here for now
        //return ST_ERR;
    }

    if (JTAG_set_tap_state(state, jtag_tlr) != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to reset tap state.");
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

    if (bic_asd_init(state->fru, 0xFF) < 0) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "1S_ASD_DEINIT failed, slot%d", state->fru);
    }
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

    uint8_t tbuf[5] = {0x00}; // IANA ID
    uint8_t rbuf[5] = {0x00};
    uint8_t rlen = 0;
    uint8_t tlen = 5;
    uint8_t slot_id = state->fru;
    STATUS ret = ST_ERR;

    // Fill the IANA ID
    memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);

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
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to find path from state%d to state%d",
                    state->active_chain->tap_state, tap_state);
        }
    }

    // add delay count for 2 special cases
    if ((tap_state == jtag_rti) || (tap_state == jtag_pau_dr)) {
        tbuf[3] += 5;
    }

    ret = jtag_bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SET_TAP_STATE,
                                tbuf, tlen, rbuf, &rlen);
    if (ret != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to run CMD_OEM_1S_SET_TAP_STATE");
        return ST_ERR;
    }

    state->active_chain->tap_state = tap_state;

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
    struct scan_xfer scan_xfer;
    scan_xfer.mode = state->sw_mode ? SW_MODE : HW_MODE;
    scan_xfer.tap_state = current_tap_state;
    scan_xfer.length = number_of_bits;
    scan_xfer.tdi_bytes = input_bytes;
    scan_xfer.tdi = input;
    scan_xfer.tdo_bytes = output_bytes;
    scan_xfer.tdo = output;
    scan_xfer.end_tap_state = end_tap_state;

    if (jtag_bic_read_write_scan(state, &scan_xfer) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ERROR, jtag_bic_read_write_scan failed");
        return ST_ERR;
    }

    // go to end_tap_state as requested
    if (JTAG_set_tap_state(state, end_tap_state) != ST_OK) {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "ERROR, failed to go state %d,", end_tap_state);
        return ST_ERR;
    }

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

    if (JTAG_clock_cycle(state->fru, number_of_cycles) != ST_OK) {
        return ST_ERR;
    }

    return ST_OK;
}

STATUS JTAG_set_jtag_tck(JTAG_Handler* state, unsigned int tck)
{
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
