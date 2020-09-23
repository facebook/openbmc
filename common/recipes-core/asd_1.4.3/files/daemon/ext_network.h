/*
Copyright (c) 2019, Intel Corporation

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

/** @file ext_network.h
 * This file contains the external network interface between the Probe Plugin
 * and JTAG
 */

#ifndef __EXT_NETWORK_H_
#define __EXT_NETWORK_H_

#include <stdbool.h>
#include <stddef.h>

#include "asd_common.h"

#define UNUSED_SOCKET_FD (-1)

// External network connection data structure.
typedef struct
{
    int sockfd;        // File descriptor of socket
    void* p_hdlr_data; // handler private data pointer
} extnet_conn_t;

typedef enum
{
    EXTNET_HDLR_NON_ENCRYPT,
    EXTNET_HDLR_TLS
} extnet_hdlr_type_t;

// Function pointers for external network interface.
typedef struct
{
    STATUS (*init)(void* p_hdlr_data);
    STATUS (*on_accept)(void* net_state, extnet_conn_t* pconn);
    STATUS (*on_close_client)(extnet_conn_t* pconn);
    STATUS (*init_client)(extnet_conn_t* pconn);
    int (*recv)(extnet_conn_t* pconn, void* pv_buf, size_t sz_len,
                bool* b_data_pending);
    int (*send)(extnet_conn_t* pconn, void* pv_buf, size_t sz_len);
    void (*cleanup)(void);
} extnet_hdlrs_t;

typedef struct ExtNet
{
    extnet_hdlrs_t* p_hdlrs;
    int n_max_sessions;
} ExtNet;

ExtNet* extnet_init(extnet_hdlr_type_t eType, void* p_hdlr_data,
                    int n_max_sessions);
STATUS extnet_open_external_socket(ExtNet* state, const char* cp_bind_if,
                                   uint16_t u16_port, int* pfd_sock);
STATUS extnet_accept_connection(ExtNet* state, int ext_listen_sockfd,
                                extnet_conn_t* pconn);
STATUS extnet_close_client(ExtNet* state, extnet_conn_t* pconn);
STATUS extnet_init_client(ExtNet* state, extnet_conn_t* pconn);
bool extnet_is_client_closed(ExtNet* state, extnet_conn_t* pconn);
int extnet_recv(ExtNet* state, extnet_conn_t* pconn, void* pv_buf,
                size_t sz_len, bool* b_data_pending);
int extnet_send(ExtNet* state, extnet_conn_t* pconn, void* pv_buf,
                size_t sz_len);

#endif // __EXT_NETWORK_H_
