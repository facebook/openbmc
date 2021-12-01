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

/**
 * @file auth_none.c
 * @brief Handler functions to support no authentication
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "authenticate.h"
#include "ext_network.h"

STATUS authnone_init(void* p_hdlr_data);
STATUS auth_none(Session* session, ExtNet* net_state, extnet_conn_t* p_extconn);

auth_hdlrs_t authnone_hdlrs = {authnone_init, auth_none};

/** @brief Initialize authentication handler
 *  @param [in] p_hdlr_data Pointer to handler specific data (not used)
 */
STATUS authnone_init(void* p_hdlr_data)
{
    (void)p_hdlr_data;
    return ST_OK;
}

/** @brief Read and validate client header and password.
 *
 *  Called when client has not been authenticated each time data is available
 *  on the external socket.
 *
 *  @param [in] p_extconn pointer
 *  @return ST_OK if successful, otherwise ST_ERR.
 */
STATUS auth_none(Session* session, ExtNet* net_state, extnet_conn_t* p_extconn)
{
    (void)session;
    (void)net_state;
    (void)p_extconn;
    return ST_OK;
}
