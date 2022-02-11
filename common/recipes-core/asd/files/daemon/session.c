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
 * @file session.c
 * @brief Functions supporting tracking of remote sessions
 *
 * The current model is that only one session is authenticated at a time.
 * But additional unauthenticated connections are allowed to meet security
 * goals:
 * 1. Don't allow an unauthenticated connection to hold a session open
 *    preventing access for other users.
 * 2  Don't give an adversary more information than necessary about sessions in
 *    progress. Accept credentials and validate even if a authenticated session
 *    is in progress.
 * 3. Track unsuccessful authentication attempts and lockout authentication if
 *    threshold is exceeded. Keep timing of accepting credentials while locked
 *    out consistent with non-locked state.
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "logging.h"
#include "ext_network.h"
#include "session.h"

/** Structure to track session info */
typedef struct {
    int id;
    extnet_conn_t extconn; // External Connection
    time_t t_auth_tout;    // Time stamp when authentication attempt times out
    bool b_authenticated;  // True if session is authenticated.
    uint32_t u32_data;     // Data associated with the session
} session_t;

/** Global data struct */
static struct {
    bool b_initialized;
    session_t sessions[MAX_SESSIONS];
    int n_authenticated_id; // only one session may be authenticated.
} sg_data = {0};

/** @brief Initialize session information
 *
 *  Initialize session data. Called once during intialization sequence.
 */
void session_init(void)
{
    int i;
    for (i=0; i < MAX_SESSIONS; i++) {
        session_t *p_sess = &sg_data.sessions[i];
        p_sess->id = i;
        extnet_init_client(&p_sess->extconn);
        p_sess->t_auth_tout = 0;
        p_sess->b_authenticated = false;
        p_sess->u32_data = 0;
    }
    sg_data.n_authenticated_id = -1;
    sg_data.b_initialized = true;
}

/** @brief Find session
 *
 *  Given a session file descriptor, return a pointer to the session struct
 *
 *  @param [in] sockfd File descriptor of socket.
 *
 *  @return NULL if not found, otherwise, a pointer to the session structure.
 */
static session_t *session_find_private(int sockfd)
{
    session_t *p_ret = NULL;
    int i;
    if (sg_data.b_initialized) {
        for (i=0; i < MAX_SESSIONS; i++) {
            session_t *p_sess = &sg_data.sessions[i];
            if (p_sess->extconn.sockfd == sockfd) {
                // looking for specific session, found it.
                p_ret = p_sess;
                break;
            }
        }
    }
    return p_ret;

}

/** @brief Get connection handle from fd
 *
 *  Lookup connection handle from file descriptor.
 *
 *  @param [in] fd File descriptor of accepted socket.
 *  @return Returns NULL if not found, pointer to connection if found.
 */
extnet_conn_t *session_lookup_conn(int fd)
{
    extnet_conn_t *p_ret = NULL;
    session_t *p_sess = session_find_private(fd);

    if (p_sess && p_sess->extconn.sockfd >=0) {
        p_ret = &p_sess->extconn;
    }
    return p_ret;
}

/** @brief Open a new session
 *
 *  Initialize session data. Called once during intialization sequence.
 *
 *  @param [in] p_extconn Connection handle
 *  @return Returns ST_ERR if no available session slots. ST_OK if
 *  successful.
 */
STATUS session_open(extnet_conn_t *p_extconn, uint32_t u32_data)
{
    STATUS st_ret = ST_ERR;
    session_t *p_sess;

    if (p_extconn) {
        p_sess = session_find_private(-1);

        if (NULL == p_sess) {
            ASD_log(LogType_Error, "No available sessions!");
        } else {
            memcpy(&p_sess->extconn, p_extconn, sizeof(extnet_conn_t));
            p_sess->t_auth_tout = SESSION_AUTH_EXPIRE_TIMEOUT + time(0);
            p_sess->b_authenticated = false;
            p_sess->u32_data = u32_data;
            ASD_log(LogType_Debug, "opened session %d fd %d", p_sess->id, p_extconn->sockfd);
            st_ret = ST_OK;
        }
    }
    return st_ret;
}

/** @brief Close a session using an pointer to the session data
 *
 *  @param [in] p_sess Pointer to session structure.
 *  @return ST_ERR if p_sess is NULL or session is not open
 *          ST_OK if successful.
 */
static STATUS session_close_private(session_t *p_sess)
{
    int st_ret = ST_ERR;

    if (!sg_data.b_initialized) {
        ASD_log(LogType_Error, "%s Not initialized!", __FUNCTION__);
    } else if (!p_sess) {
        ASD_log(LogType_Error, "Null session!");
    } else {
        ASD_log(LogType_Debug, "closing session %d fd %d",
                p_sess->id, p_sess->extconn.sockfd);
        if (extnet_is_client_closed(&p_sess->extconn)) {
            ASD_log(LogType_Error, "session %d already closed!", p_sess->id);
        } else {
            extnet_close_client(&p_sess->extconn);
        }
        if (p_sess->id == sg_data.n_authenticated_id) {
            sg_data.n_authenticated_id = -1;
        }
        p_sess->t_auth_tout = 0;
        p_sess->b_authenticated = false;
        p_sess->u32_data = 0;
        st_ret = ST_OK;
    }
    return st_ret;
}

/** @brief Close a new session
 *
 *  Given a session file descriptor, close the associated session.
 *
 *  @param [in] p_extconn File descriptor of socket.
 *  @return Returns ST_ERR if session does not exist for associated
 *          file descriptor. ST_OK if successful.
 */
STATUS session_close(extnet_conn_t *p_extconn)
{
    STATUS st_ret = ST_ERR;
    session_t *p_sess;

    if (p_extconn) {
        p_sess = session_find_private(p_extconn->sockfd);

        if (NULL == p_sess) {
            ASD_log(LogType_Error, "Invalid session");
        } else {
            st_ret = session_close_private(p_sess);
            st_ret = ST_OK;
        }
    }
    return st_ret;
}

/** @brief Close all sessions
 *
 *  Close all open sessions.
 */
void session_close_all(void)
{
    int i;

    if (sg_data.b_initialized) {
        for (i=0; i < MAX_SESSIONS; i++) {
            session_t *p_sess = &sg_data.sessions[i];
            if (!extnet_is_client_closed(&p_sess->extconn)) {
                session_close_private(p_sess);
            }
        }
    }
    return;
}

/** @brief Close expired sessions which have not authenticated
 *
 *  Close all open sessions open longer than the maximum time which have not
 *  authenticated.
 */
void session_close_expired_unauth(void)
{
    int i;

    if (sg_data.b_initialized) {
        for (i=0; i < MAX_SESSIONS; i++) {
            session_t *p_sess = &sg_data.sessions[i];
            if (!extnet_is_client_closed(&p_sess->extconn) &&
                    !p_sess->b_authenticated &&
                    p_sess->t_auth_tout <= time(0)) {
                ASD_log(LogType_Debug, "Unauthenticated Session %d time out",
                        p_sess->id);
                session_close_private(p_sess);
            }
        }
    }
    return;
}

/** @brief Add data to session
 *
 *  Given a session file descriptor, set data pointer
 *
 *  @param [in] p_extconn External connection associated with session
 *  @param [in] u32_data data to associate with session
 *  @return Returns ST_ERR if session does not exist for associated
 *          file descriptor. ST_OK if successful.
 */
STATUS session_set_data(extnet_conn_t *p_extconn, uint32_t u32_data)
{
    STATUS st_ret = ST_ERR;
    session_t *p_sess;

    if (p_extconn) {
        p_sess = session_find_private(p_extconn->sockfd);

        if (!sg_data.b_initialized || NULL == p_sess) {
            ASD_log(LogType_Error, "Invalid connection");
        } else {
            p_sess->u32_data = u32_data;
            st_ret = ST_OK;
        }
    }
    return st_ret;
}

/** @brief get data from session
 *
 *  Given a session file descriptor, get the data pointer
 *
 *  @param [in] p_extconn connection associated with session
 *  @param [out] u32_data data associated with session
 *  @return Returns ST_ERR if session does not exist for associated
 *          file descriptor. ST_OK if successful.
 */
STATUS session_get_data(extnet_conn_t *p_extconn, uint32_t *pu32_data)
{
    STATUS st_ret = ST_ERR;
    session_t *p_sess;

    if (p_extconn && pu32_data) {
        p_sess = session_find_private(p_extconn->sockfd);

        if (NULL == p_sess) {
            ASD_log(LogType_Error, "Invalid connection");
        } else {
            *pu32_data = p_sess->u32_data;
            st_ret = ST_OK;
        }
    }
    return st_ret;
}

/** @brief Indicate if session is authenticated
 *
 *  Given a session file descriptor, indicate if authenticated.
 *
 *  @param [in] p_extconn connection associated with session
 *  @return Returns ST_OK if authenticated. ST_ERR otherwise.
 */
STATUS session_already_authenticated(extnet_conn_t *p_extconn)
{
    STATUS st_ret = ST_ERR;
    session_t *p_sess;

    if (p_extconn && !extnet_is_client_closed(p_extconn)) {
        p_sess = session_find_private(p_extconn->sockfd);

        if (NULL == p_sess) {
            ASD_log(LogType_Error, "Invalid session");
        } else {
            if (p_sess->b_authenticated) {
                st_ret = ST_OK;
            }
        }
    }
    return st_ret;
}

/** @brief Indicate if session is authenticated
 *
 *  Given a session file descriptor, indicate if authenticated.
 *
 *  @param [in] p_extconn connection associated with session
 *  @return Returns ST_OK if authenticated.
 *                  ST_ERR if invalid session
 */
STATUS session_auth_complete(extnet_conn_t *p_extconn)
{
    STATUS st_ret = ST_ERR;

    if (sg_data.b_initialized && p_extconn) {
        if (sg_data.n_authenticated_id >= 0) {
            ASD_log(LogType_Error, "Cannot set complete authentication, session %d is already authenticated",
                    sg_data.n_authenticated_id);
        } else {
            session_t *p_sess = session_find_private(p_extconn->sockfd);

            if (NULL == p_sess) {
                ASD_log(LogType_Error, "Invalid session");
            } else {
                sg_data.n_authenticated_id = p_sess->id;
                p_sess->b_authenticated = true;
                st_ret = ST_OK;
            }
        }
    }
    return st_ret;
}

/** @brief Get connection for authenticated client
 *
 *  @param [in] p_authd_conn client handle
 *
 *  @return ST_OK if successful.
 *          ST_ERR if session does not exist for associated file
 *          descriptor.
 */
STATUS session_get_authenticated_conn(extnet_conn_t *p_authd_conn)
{
    STATUS st_ret = ST_ERR;
    int n_idx = sg_data.n_authenticated_id;

    if (sg_data.b_initialized && n_idx >= 0) {
        if (p_authd_conn) {
            memcpy(p_authd_conn,
                &sg_data.sessions[n_idx].extconn, sizeof(extnet_conn_t));
        }
        st_ret = ST_OK;
    }
    return st_ret;
}

/** @brief Get file descriptor set for all open sessions
 *
 *  Assumes that p_fdset points to valid fdset structure that has been zeroed.
 *  The p_fdset may include set fds from non-session file descriptors.
 *
 *  @param [in/out] fdset File descriptor set used to return session file
 *  descriptors.
 *  @param [in/out] pn_maxfd Max value of file descriptors in set.
 *
 *  @return Returns ST_ERR if invalid pointer passed to fdset.
 *          ST_OK if successful.
 */
STATUS session_getfds(session_fdarr_t na_fds, int *pn_fds, int *pn_timeout_ms)
{
    STATUS st_ret = ST_ERR;

    if (sg_data.b_initialized && na_fds && pn_fds && pn_timeout_ms) {
        int i;
        for (i=0; i < MAX_SESSIONS; i++) {
            session_t *p_sess = &sg_data.sessions[i];
            if (!extnet_is_client_closed(&p_sess->extconn)) {
                if (!p_sess->b_authenticated) {
                    int n_msremain = 1000 * (p_sess->t_auth_tout - time(0L));
                    if (n_msremain < 0) {
                        n_msremain = 0;
                    }
                    if (*pn_timeout_ms < 0 || n_msremain < *pn_timeout_ms) {
                        *pn_timeout_ms = n_msremain;
                        ASD_log(LogType_Debug, "Session %d time remaining: %d",
                                p_sess->id, n_msremain);
                    }
                }
                na_fds[*pn_fds] = p_sess->extconn.sockfd;
                (*pn_fds)++;
            }
        }
        st_ret = ST_OK;
    }
    return st_ret;
}
