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

/** @file ext_tcp.h
 * Handlers for unencrypted TCP connections.
 */

#ifndef __EXT_TCP_H
#define __EXT_TCP_H

#include <stdbool.h>

#include "asd_common.h"
#include "ext_network.h"

extern extnet_hdlrs_t tcp_hdlrs;

extern STATUS exttcp_init(void* p_hdlr_data);
extern void exttcp_cleanup(void);
extern STATUS exttcp_on_accept(void* net_state, extnet_conn_t* pconn);
extern STATUS exttcp_init_client(extnet_conn_t* pconn);
extern STATUS exttcp_on_close_client(extnet_conn_t* pconn);
extern int exttcp_recv(extnet_conn_t* pconn, void* pv_buf, size_t sz_len,
                       bool* b_data_pending);
extern int exttcp_send(extnet_conn_t* pconn, void* pv_buf, size_t sz_len);

#endif //__EXT_TCP_H
