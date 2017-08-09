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

/** @file ext_network.c
 * This file contains the external network interface between the Probe Plugin
 * and JTAG.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include <syslog.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <errno.h>
#include <assert.h>

#include "asd/SoftwareJTAGHandler.h"
#include "logging.h"
#include "ext_network.h"
#include "ext_tcp.h"
#ifndef REFERENCE_CODE
/* Placeholder for inclusion of an implementation specific network security solution.
   Implement your network security solution and undefine the REFERENCE_CODE variable. */
#include "ext_tls.h"
#endif

static struct {
    extnet_hdlrs_t *p_hdlrs;
    int n_max_sessions;
} sg_data = {0};

/** @brief Initialize External network interface
 *
 *  Called to initialize External Network Interface
 */
STATUS extnet_init(extnet_hdlr_type_t e_type, void *p_hdlr_data, int n_max_sessions)
{
    STATUS st_ret = ST_ERR;

    switch(e_type) {
        case EXTNET_HDLR_NON_ENCRYPT:
            sg_data.p_hdlrs = &tcp_hdlrs;
            break;
#ifndef REFERENCE_CODE
        /* Placeholder for inclusion of an implementation specific network security solution.
           Implement your network security solution and undefine the REFERENCE_CODE variable. */
        case EXTNET_HDLR_TLS:
            sg_data.p_hdlrs = &tls_hdlrs;
            break;
#endif
        default:
            ASD_log(LogType_Error, "Invalid external network handler %d!", e_type);
            break;
    }
    if (sg_data.p_hdlrs && sg_data.p_hdlrs->init) {
        st_ret = sg_data.p_hdlrs->init(p_hdlr_data);
        sg_data.n_max_sessions = n_max_sessions;
        st_ret = ST_OK;
    }

    // ignore the SIGPIPE and handle the error directly in code
    signal(SIGPIPE, SIG_IGN);

    return st_ret;
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
STATUS extnet_open_external_socket(const char *cp_bind_if, uint16_t u16_port, int *pfd_sock)
{
    struct sockaddr_in6 addr;
    int n_reuse = 1;
    int n_delay = 1;

    // create socket
    *pfd_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (*pfd_sock < 0) {
        ASD_log(LogType_Error, "Error creating socket on %s:%u. errno: %d",
                (cp_bind_if ? cp_bind_if : "*"), u16_port, errno);
        return ST_ERR;
    }

    if (setsockopt(*pfd_sock, IPPROTO_TCP, TCP_NODELAY, &n_delay, sizeof(n_delay)) < 0) {
        ASD_log(LogType_Error, "setsockopt(TCP_NODELAY) failed errno: %d", errno);
        return ST_ERR;
    }

    // if client didn't disconnect clean, don't require wait to rebind.
    if (setsockopt(*pfd_sock, SOL_SOCKET, SO_REUSEADDR, &n_reuse, sizeof(n_reuse)) < 0) {
        ASD_log(LogType_Error, "setsockopt(SO_REUSEADDR) failed errno: %d", errno);
        return ST_ERR;
    }

    // Bind to specific interface if configured.
    if (cp_bind_if) {
        if (setsockopt(*pfd_sock, SOL_SOCKET, SO_BINDTODEVICE, cp_bind_if,
                    strnlen(cp_bind_if, IFNAMSIZ)) < 0) {
            ASD_log(LogType_Error,
                    "setsockopt(SO_BINDTODEVICE '%s') failed; errno: %d",
                    cp_bind_if, errno);
            return ST_ERR;
        }
    }

    // bind the socket
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(u16_port);
    addr.sin6_addr = in6addr_any;
    if (bind(*pfd_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ASD_log(LogType_Error, "Error binding to socket on %s:%u. errno: %d",
                (cp_bind_if ? cp_bind_if : "*"), u16_port, errno);
        close(*pfd_sock);
        *pfd_sock = -1;
        return ST_ERR;
    }

    // Start listening
    if (listen(*pfd_sock, sg_data.n_max_sessions) < 0) {
        ASD_log(LogType_Error, "Error listen on external socket errno: %d", errno);
        close(*pfd_sock);
        *pfd_sock = -1;
        return ST_ERR;
    }

    return ST_OK;
}


/** @brief Clean up handler data
 *
 *  Called to cleanup connection.
 */
void extnet_cleanup(void)
{
    if (sg_data.p_hdlrs) {
        return sg_data.p_hdlrs->cleanup();
    }
}

/** @brief Accepts the socket connection and handles client key
 *
 *  Called each time a new connection is accepted on the listening socket.
 *
 *  @param [in] ext_listen_sockfd File descriptor for listening socket.
 *  @param [in,out] pconn Pointer to the connection structure.
 *  @return ST_OK if successful.
 */
STATUS extnet_accept_connection(int ext_listen_sockfd, extnet_conn_t *pconn)
{
    struct sockaddr_in6 addr;
    uint len = sizeof(addr);
    STATUS st_ret = ST_ERR;

    if (!pconn || !sg_data.p_hdlrs || !sg_data.p_hdlrs->on_accept) {
        ASD_log(LogType_Error, "%s called with invalid pointer", __FUNCTION__);
        assert(0);
        return ST_ERR;
    }

    pconn->sockfd = accept(ext_listen_sockfd, (struct sockaddr*)&addr, &len);
    if (pconn->sockfd < 0) {
        ASD_log(LogType_Error, "accept() errno: %d", errno);
    } else {
        ASD_log(LogType_Debug, "Accepted client fd %d", pconn->sockfd);
        st_ret = sg_data.p_hdlrs->on_accept(pconn);
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
STATUS extnet_init_client(extnet_conn_t *pconn)
{
    STATUS st_ret = ST_ERR;

    if (!pconn || !sg_data.p_hdlrs || !sg_data.p_hdlrs->init_client) {
        ASD_log(LogType_Error, "%s called with invalid pointer", __FUNCTION__);
fflush(stdout);
        assert(0);
    } else {
        pconn->sockfd = -1;
        st_ret = sg_data.p_hdlrs->init_client(pconn);
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
STATUS extnet_close_client(extnet_conn_t *pconn)
{
    STATUS st_ret = ST_ERR;

    if (!pconn || !sg_data.p_hdlrs || !sg_data.p_hdlrs->on_close_client) {
        ASD_log(LogType_Error, "%s called with invalid pointer", __FUNCTION__);
        assert(0);
    } else {
        ASD_log(LogType_Debug, "Closing client fd %d", pconn->sockfd);

        if (pconn->sockfd < 0) {
            st_ret = ST_ERR;
        } else {
            st_ret = sg_data.p_hdlrs->on_close_client(pconn);
            close(pconn->sockfd);
            pconn->sockfd = -1;
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
bool extnet_is_client_closed(extnet_conn_t *pconn)
{
    if (!pconn) {
        ASD_log(LogType_Error, "%s called with invalid pointer", __FUNCTION__);
        assert(0);
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
int extnet_recv(extnet_conn_t *pconn, void *pv_buf, size_t sz_len)
{
    int n_ret = -1;

    if (sg_data.p_hdlrs && sg_data.p_hdlrs->recv) {
        n_ret = sg_data.p_hdlrs->recv(pconn, pv_buf, sz_len);
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
int extnet_send(extnet_conn_t *pconn, void *pv_buf, size_t sz_len)
{
    int n_ret = -1;

    if (sg_data.p_hdlrs && sg_data.p_hdlrs->send) {
        n_ret = sg_data.p_hdlrs->send(pconn, pv_buf, sz_len);
    }
    return n_ret;
}
