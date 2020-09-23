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
 * @file authenticate.c
 * @brief Functions supporting authentication of credentials from client,
 * tracking authentication attempts and locking out authentication when a
 * threshold is exceeded.
 */
#include "authenticate.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "asd_common.h"
#include "auth_none.h"
//#include "auth_pam.h"
#include "ext_network.h"
#include "logging.h"
#include "session.h"

static struct
{
    auth_hdlrs_t* p_hdlrs;
} sg_data = {0};

/** @brief Initialize authentication handler
 *  @param [in] e_type Handler type to use for authentication.
 *  @param [in] p_hdlr_data Pointer to handler specific data (not used)
 */
STATUS auth_init(auth_hdlr_type_t e_type, void* p_hdlr_data)
{
    STATUS st_ret = ST_OK;

    switch (e_type)
    {
        case AUTH_HDLR_NONE:
            sg_data.p_hdlrs = &authnone_hdlrs;
            break;
        //case AUTH_HDLR_PAM:
        //    sg_data.p_hdlrs = &authpam_hdlrs;
        //    break;
        default:
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Invalid authentication handler %d!",
                    e_type);
            st_ret = ST_ERR;
            break;
    }

    if (!sg_data.p_hdlrs || !sg_data.p_hdlrs->init)
        st_ret = ST_ERR;

    if (st_ret == ST_OK)
    {
        st_ret = sg_data.p_hdlrs->init(p_hdlr_data);
    }
    return st_ret;
}

/** @brief Read and validate client header and password.
 *
 *  Called when client has not been authenticated each time data is available
 *  on the external socket.
 *
 *  @param [in] p_extconn pointer
 *  @return AUTHRET_OK if successful, otherwise another auth_ret_t value.
 */
STATUS auth_client_handshake(Session* session, ExtNet* net_state,
                             extnet_conn_t* p_extconn)
{
    STATUS st_ret = ST_ERR;

    if (!sg_data.p_hdlrs || !sg_data.p_hdlrs->client_handshake)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s Not initialized!", __FUNCTION__);
    }
    else
    {
        st_ret =
            sg_data.p_hdlrs->client_handshake(session, net_state, p_extconn);
    }
    return st_ret;
}
