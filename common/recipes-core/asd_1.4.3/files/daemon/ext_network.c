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

#include "ext_network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "asd_common.h"
#include "ext_tcp.h"
//#include "ext_tls.h"
#include "logging.h"

/** @brief Initialize External network interface
 *
 *  Called to initialize External Network Interface
 */
ExtNet* extnet_init(extnet_hdlr_type_t e_type, void* p_hdlr_data,
                    int n_max_sessions)
{
    STATUS st_ret = ST_OK;

    if (!p_hdlr_data)
    {
        return NULL;
    }

    ExtNet* state = (ExtNet*)malloc(sizeof(ExtNet));
    if (state == NULL)
    {
        return NULL;
    }

    state->n_max_sessions = n_max_sessions;
    switch (e_type)
    {
        case EXTNET_HDLR_NON_ENCRYPT:
            state->p_hdlrs = &tcp_hdlrs;
            break;
        //case EXTNET_HDLR_TLS:
        //    state->p_hdlrs = &tls_hdlrs;
        //    break;
        default:
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Invalid external network handler %d!",
                    e_type);
            st_ret = ST_ERR;
            break;
    }

    if (st_ret == ST_OK)
    {
        st_ret = state->p_hdlrs->init(p_hdlr_data);
    }

    if (st_ret == ST_OK)
    {
        // ignore the SIGPIPE and handle the error directly in code
        signal(SIGPIPE, SIG_IGN);
    }
    else
    {
        free(state);
        state = NULL;
    }

    return state;
}

/** @brief Open the external TCP socket for listening
 *
 *  Called to open the main listening port for the external network connections
 *
 *  @param [in] cp_bind_if String containing the network interface name on
 *         which the program will listen for incoming connections. If NULL,
 *         will listen on all intefaces.
 *  @param [in] u16_port TCP port number used to accept incoming connections.
 *  @param [out] pfd_sock file descriptor for the new socket.
 *  @return ST_OK if successful.
 */
STATUS extnet_open_external_socket(ExtNet* state, const char* cp_bind_if,
                                   uint16_t u16_port, int* pfd_sock)
{
    struct sockaddr_in6 addr;
    int n_reuse = 1;
    int n_delay = 1;
    STATUS result = ST_OK;

    if (!state || !pfd_sock)
    {
        result = ST_ERR;
    }

    if (result == ST_OK)
    {
        // create socket
        *pfd_sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (*pfd_sock < 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "Error creating socket on %s:%u. errno: %d",
                    (cp_bind_if ? cp_bind_if : "*"), u16_port, errno);
            result = ST_ERR;
        }
    }

    if (result == ST_OK)
    {
        if (setsockopt(*pfd_sock, IPPROTO_TCP, TCP_NODELAY, &n_delay,
                       sizeof(n_delay)) < 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "setsockopt(TCP_NODELAY) failed errno: %d", errno);
            result = ST_ERR;
        }
    }

    if (result == ST_OK)
    {
        // if client didn't disconnect clean, don't require wait to
        // rebind.
        if (setsockopt(*pfd_sock, SOL_SOCKET, SO_REUSEADDR, &n_reuse,
                       sizeof(n_reuse)) < 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "setsockopt(SO_REUSEADDR) failed errno: %d", errno);
            result = ST_ERR;
        }
    }

    if (result == ST_OK)
    {
        // Bind to specific interface if configured.
        if (cp_bind_if)
        {
            if (setsockopt(*pfd_sock, SOL_SOCKET, SO_BINDTODEVICE, cp_bind_if,
                           (socklen_t)strnlen(cp_bind_if, IFNAMSIZ)) < 0)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                        ASD_LogOption_None,
                        "setsockopt(SO_BINDTODEVICE '%s') failed; errno: %d",
                        cp_bind_if, errno);
                result = ST_ERR;
            }
        }
    }

    if (result == ST_OK)
    {
        // bind the socket
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(u16_port);
        addr.sin6_addr = in6addr_any;
        if (bind(*pfd_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "Error binding to socket on %s:%u. errno: %d",
                    (cp_bind_if ? cp_bind_if : "*"), u16_port, errno);
            close(*pfd_sock);
            *pfd_sock = -1;
            result = ST_ERR;
        }
    }

    if (result == ST_OK)
    {
        // Start listening
        if (listen(*pfd_sock, state->n_max_sessions) < 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "Error listen on external socket errno: %d", errno);
            close(*pfd_sock);
            *pfd_sock = -1;
            result = ST_ERR;
        }
    }

    return result;
}

/** @brief Accepts the socket connection and handles client key
 *
 *  Called each time a new connection is accepted on the listening socket.
 *
 *  @param [in] ext_listen_sockfd File descriptor for listening socket.
 *  @param [in,out] pconn Pointer to the connection structure.
 *  @return ST_OK if successful.
 */
STATUS extnet_accept_connection(ExtNet* state, int ext_listen_sockfd,
                                extnet_conn_t* pconn)
{
    struct sockaddr_in6 addr;
    size_t len = sizeof(addr);
    STATUS st_ret = ST_ERR;

    if (!state || !pconn || !state->p_hdlrs || !state->p_hdlrs->on_accept)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
        return ST_ERR;
    }

    pconn->sockfd =
        accept(ext_listen_sockfd, (struct sockaddr*)&addr, (socklen_t*)&len);
    if (pconn->sockfd < 0)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "accept() errno: %d", errno);
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network, ASD_LogOption_None,
                "Accepted client fd %d", pconn->sockfd);
#endif
        st_ret = state->p_hdlrs->on_accept(state, pconn);
        if (st_ret != ST_OK)
            extnet_close_client(state, pconn);
    }

    return st_ret;
}

/** @brief Initialize client connection
 *
 *  Called to initialize external client connection
 *
 *  @param [in,out] pconn Pointer to the connection pointer.
 *  @return RET_OK if successful, ST_ERR otherwise.
 */
STATUS extnet_init_client(ExtNet* state, extnet_conn_t* pconn)
{
    STATUS st_ret = ST_ERR;

    if (!state || !pconn || !state->p_hdlrs || !state->p_hdlrs->init_client)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
    }
    else
    {
        pconn->sockfd = UNUSED_SOCKET_FD;
        st_ret = state->p_hdlrs->init_client(pconn);
    }
    return st_ret;
}

/** @brief Close client connection and free pointer.
 *
 *  Called to close external client connection
 *
 *  @param [in,out] pconn Pointer to the connection pointer.
 *  @return RET_OK if successful, ST_ERR otherwise.
 */
STATUS extnet_close_client(ExtNet* state, extnet_conn_t* pconn)
{
    STATUS st_ret = ST_ERR;

    if (!state || !pconn || !state->p_hdlrs || !state->p_hdlrs->on_close_client)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network, ASD_LogOption_None,
                "Closing client fd %d", pconn->sockfd);
#endif

        if (pconn->sockfd < 0)
        {
            st_ret = ST_ERR;
        }
        else
        {
            st_ret = state->p_hdlrs->on_close_client(pconn);
            if (close(pconn->sockfd) < 0)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                        ASD_LogOption_None, "Failed to close client fd %d",
                        pconn->sockfd);
                st_ret = ST_ERR;
            }
            pconn->sockfd = UNUSED_SOCKET_FD;
            pconn->p_hdlr_data = NULL;
        }
    }

    return st_ret;
}

/** @brief Close client connection and free pointer.
 *
 *  Called to close external client connection
 *
 *  @param [in,out] pconn Pointer to the connection pointer.
 *  @return RET_OK if successful, ST_ERR otherwise.
 */
bool extnet_is_client_closed(ExtNet* state, extnet_conn_t* pconn)
{
    if (!state || !pconn)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
        return false;
    }

    return (pconn->sockfd < 0 ? true : false);
}

/** @brief Read data from external network connection
 *
 *  Called each time data is available on the external socket.
 *
 *  @param [in] pconn Connetion pointer
 *  @param [out] pv_buf Buffer where data will be stored.
 *  @param [in] sz_len sizeof pv_buf
 *  @return number of bytes received.
 */
int extnet_recv(ExtNet* state, extnet_conn_t* pconn, void* pv_buf,
                size_t sz_len, bool* b_data_pending)
{
    int n_ret = -1;

    if (state && state->p_hdlrs && state->p_hdlrs->recv && pconn && pv_buf &&
        b_data_pending)
    {
        n_ret = state->p_hdlrs->recv(pconn, pv_buf, sz_len, b_data_pending);
    }
    return n_ret;
}

/** @brief Write data to external network connection
 *
 *  Called each time data is available on the external socket.
 *
 *  @param [in] pconn Connetion pointer
 *  @param [out] pv_buf Buffer where data will be stored.
 *  @param [in] sz_len sizeof pv_buf
 *  @return number of bytes received.
 */
int extnet_send(ExtNet* state, extnet_conn_t* pconn, void* pv_buf,
                size_t sz_len)
{
    int n_ret = -1;

    if (state && state->p_hdlrs && state->p_hdlrs->send && pconn && pv_buf)
    {
        n_ret = state->p_hdlrs->send(pconn, pv_buf, sz_len);
    }
    return n_ret;
}
