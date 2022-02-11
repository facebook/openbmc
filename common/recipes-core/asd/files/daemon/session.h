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

/**
 * @file session.h
 * @brief Functions and definitions to track remote sessions
 */

#ifndef __SESSION_H
#define __SESSION_H

#include "ext_network.h"

/** Max number of sessions */
#define MAX_SESSIONS 5
#define SESSION_AUTH_EXPIRE_TIMEOUT 15

typedef int session_fdarr_t[MAX_SESSIONS];

extern void session_init(void);
extern extnet_conn_t *session_lookup_conn(int fd);
extern STATUS session_open(extnet_conn_t *p_extconn, uint32_t u32_data);
extern STATUS session_close(extnet_conn_t *p_extconn);
extern void session_close_all(void);
extern void session_close_expired_unauth(void);
extern STATUS session_set_data(extnet_conn_t *p_extconn, uint32_t u32_data);
extern STATUS session_get_data(extnet_conn_t *p_extconn, uint32_t *pu32_data);
extern STATUS session_already_authenticated(extnet_conn_t *p_extconn);
extern STATUS session_auth_complete(extnet_conn_t *p_extconn);
extern STATUS session_get_authenticated_conn(extnet_conn_t *p_authd_conn);
extern STATUS session_getfds(session_fdarr_t na_fds, int *pn_fds, int *pn_timeout);

#endif
