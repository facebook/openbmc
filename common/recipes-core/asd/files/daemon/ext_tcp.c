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

/** @file ext_tcp.c
 * This file contains the external network interface using vanilla TCP with no
 * encryption.
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
#include <sys/socket.h>
#include <linux/if.h>
#include <assert.h>
#include "asd/SoftwareJTAGHandler.h"
#include "logging.h"
#include "ext_network.h"
#include "ext_tcp.h"

extnet_hdlrs_t tcp_hdlrs = {
    exttcp_init,
    exttcp_on_accept,
    exttcp_on_close_client,
    exttcp_init_client,
    exttcp_recv,
    exttcp_send,
    exttcp_cleanup,
};

/** @brief Initialize TCP
 *
 *  Called to initialize External Network Interface
 */
STATUS exttcp_init(void * p_hdlr_data)
{
    return ST_OK;
}

/** @brief Cleans up TCP context and connections.
 *
 *  Called to cleanup connection.
 */
void exttcp_cleanup(void)
{
    return;
}


/** @brief Accepts the socket connection and handles client key
 *
 *  Called each time a new connection is accepted on the listening socket.
 *
 *  @param [in] ext_listen_sockfd File descriptor for listening socket.
 *  @param [in,out] pconn Pointer to the connection structure.
 *  @return ST_OK if successful.
 */
STATUS exttcp_on_accept(extnet_conn_t *pconn)
{
    return ST_OK;
}

/** @brief Initialize client connection
 *
 *  Called to initialize external client connection
 *
 *  @param [in,out] pconn Pointer to the connection pointer.
 *  @return RET_OK if successful.
 */
STATUS exttcp_init_client(extnet_conn_t *pconn)
{
    return ST_OK;
}

/** @brief Close client connection and free pointer.
 *
 *  Called to close external client connection
 *
 *  @param [in,out] pconn Pointer to the connection pointer.
 *  @return RET_OK if successful.
 */
STATUS exttcp_on_close_client(extnet_conn_t *pconn)
{
    return ST_OK;
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
int exttcp_recv(extnet_conn_t *pconn, void *pv_buf, size_t sz_len)
{
    int n_read = -1;

    if (!pconn) {
        ASD_log(LogType_Error, "%s called with invalid pointer", __FUNCTION__);
        assert(0);
    } else if (pconn->sockfd < 0) {
        ASD_log(LogType_Error, "%s called with invalid file descriptor %d",
                __FUNCTION__, pconn->sockfd);
    } else {
        n_read = recv(pconn->sockfd, pv_buf, sz_len, 0);
    }
    return n_read;
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
int exttcp_send(extnet_conn_t *pconn, void *pv_buf, size_t sz_len)
{
    int n_wr = -1;

    if (!pconn) {
        ASD_log(LogType_Error, "%s called with invalid pointer", __FUNCTION__);
        assert(0);
    } else if (pconn->sockfd < 0) {
        ASD_log(LogType_Error, "%s called with invalid file descriptor %d",
                __FUNCTION__, pconn->sockfd);
    } else {
        n_wr = send(pconn->sockfd, pv_buf, sz_len, 0);
    }
    return n_wr;
}
