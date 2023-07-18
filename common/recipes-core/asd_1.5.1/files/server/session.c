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
#include "session.h"

#include <netinet/in.h>
#include <safe_mem_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "asd_common.h"
#include "ext_network.h"
#include "logging.h"

/** @brief Initialize session information
 *
 *  Initialize session data. Called once during initialization sequence.
 *
 *  @return ST_ERR if session failed to initialize,
 *          ST_OK if successful.
 */
Session* session_init(ExtNet* extnet)
{
    STATUS result = ST_OK;
    if (extnet == NULL)
    {
        return NULL;
    }
    Session* state = (Session*)malloc(sizeof(Session));
    if (state == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        session_t* p_sess = &state->sessions[i];
        p_sess->id = i;
        result = extnet_init_client(extnet, &p_sess->extconn);
        if (result != ST_OK)
            break;
        p_sess->t_auth_tout = 0;
        p_sess->b_authenticated = false;
        p_sess->b_data_pending = false;
    }
    if (result == ST_OK)
    {
        state->n_authenticated_id = NO_SESSION_AUTHENTICATED;
        state->b_initialized = true;
        state->extnet = extnet;
    }
    else
    {
        free(state);
        state = NULL;
    }
    return state;
}

/** @brief Find session
 *
 *  Given a session file descriptor, return a pointer to the session struct
 *
 *  @param [in] sockfd File descriptor of socket.
 *
 *  @return NULL if not found, otherwise, a pointer to the session structure.
 */
static session_t* session_find_private(Session* state, int sockfd)
{
    session_t* p_ret = NULL;
    if (state && state->b_initialized)
    {
        for (int i = 0; i < MAX_SESSIONS; i++)
        {
            session_t* p_sess = &state->sessions[i];
            if (p_sess->extconn.sockfd == sockfd)
            {
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
extnet_conn_t* session_lookup_conn(Session* state, int fd)
{
    extnet_conn_t* p_ret = NULL;
    if (state)
    {
        session_t* p_sess = session_find_private(state, fd);

        if (p_sess && p_sess->extconn.sockfd >= 0)
        {
            p_ret = &p_sess->extconn;
        }
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
STATUS session_open(Session* state, extnet_conn_t* p_extconn)
{
    STATUS st_ret = ST_ERR;
    session_t* p_sess;

    if (state && p_extconn)
    {
        p_sess = session_find_private(state, UNUSED_SOCKET_FD);

        if (NULL == p_sess)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "No available sessions!");
        }
        else
        {
            if (memcpy_s(&p_sess->extconn, sizeof(p_sess->extconn), p_extconn,
                         sizeof(p_sess->extconn)))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                        ASD_LogOption_None,
                        "memcpy_s: p_extconn to p_sess copy failed.");
            }
            p_sess->t_auth_tout = SESSION_AUTH_EXPIRE_TIMEOUT + time(0);
            p_sess->b_authenticated = false;
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                    ASD_LogOption_None, "opened session %d fd %d", p_sess->id,
                    p_extconn->sockfd);
#endif
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
static STATUS session_close_private(Session* state, session_t* p_sess)
{
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network, ASD_LogOption_None,
            "closing session %d fd %d", p_sess->id, p_sess->extconn.sockfd);
#endif
    if (extnet_is_client_closed(state->extnet, &p_sess->extconn))
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "session %d already closed!", p_sess->id);
    }
    else
    {
        extnet_close_client(state->extnet, &p_sess->extconn);
    }
    if (p_sess->id == state->n_authenticated_id)
    {
        state->n_authenticated_id = NO_SESSION_AUTHENTICATED;
    }
    p_sess->t_auth_tout = 0;
    p_sess->b_authenticated = false;
    return ST_OK;
}

/** @brief Close a new session
 *
 *  Given a session file descriptor, close the associated session.
 *
 *  @param [in] p_extconn File descriptor of socket.
 *  @return Returns ST_ERR if session does not exist for associated
 *          file descriptor. ST_OK if successful.
 */
STATUS session_close(Session* state, extnet_conn_t* p_extconn)
{
    STATUS st_ret = ST_ERR;
    session_t* p_sess;

    if (state && p_extconn && p_extconn->sockfd != UNUSED_SOCKET_FD)
    {
        p_sess = session_find_private(state, p_extconn->sockfd);

        if (NULL == p_sess)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Invalid session");
        }
        else
        {
            st_ret = session_close_private(state, p_sess);
        }
    }
    return st_ret;
}

/** @brief Close all sessions
 *
 *  Close all open sessions.
 */
void session_close_all(Session* state)
{
    if (state && state->b_initialized)
    {
        for (int i = 0; i < MAX_SESSIONS; i++)
        {
            session_t* p_sess = &state->sessions[i];
            if (p_sess &&
                !extnet_is_client_closed(state->extnet, &p_sess->extconn))
            {
                session_close_private(state, p_sess);
            }
        }
    }
}

/** @brief Close expired sessions which have not authenticated
 *
 *  Close all open sessions open longer than the maximum time which have not
 *  authenticated.
 */
void session_close_expired_unauth(Session* state)
{
    if (state && state->b_initialized)
    {
        for (int i = 0; i < MAX_SESSIONS; i++)
        {
            session_t* p_sess = &state->sessions[i];
            if (p_sess &&
                !extnet_is_client_closed(state->extnet, &p_sess->extconn) &&
                !p_sess->b_authenticated && p_sess->t_auth_tout <= time(0))
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                        ASD_LogOption_None,
                        "Unauthenticated Session %d time out", p_sess->id);
#endif
                session_close_private(state, p_sess);
            }
        }
    }
}

/** @brief Indicate if session is authenticated
 *
 *  Given a session file descriptor, indicate if authenticated.
 *
 *  @param [in] p_extconn connection associated with session
 *  @return Returns ST_OK if authenticated. ST_ERR otherwise.
 */
STATUS session_already_authenticated(Session* state, extnet_conn_t* p_extconn)
{
    STATUS st_ret = ST_ERR;
    session_t* p_sess;

    if (state && state->b_initialized && p_extconn &&
        !extnet_is_client_closed(state->extnet, p_extconn))
    {
        p_sess = session_find_private(state, p_extconn->sockfd);

        if (NULL == p_sess)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Invalid session");
        }
        else
        {
            if (p_sess->b_authenticated)
            {
                st_ret = ST_OK;
            }
        }
    }
    return st_ret;
}

/** @brief Authenticate given session
 *
 *  Given a session file descriptor, authenticate it.
 *
 *  @param [in] p_extconn connection associated with session
 *  @return Returns ST_OK if authenticated.
 *                  ST_ERR if invalid session
 */
STATUS session_auth_complete(Session* state, extnet_conn_t* p_extconn)
{
    STATUS st_ret = ST_ERR;

    if (state && state->b_initialized && p_extconn)
    {
        if (state->n_authenticated_id >= 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "Cannot set complete authentication, "
                    "session %d is already authenticated",
                    state->n_authenticated_id);
        }
        else
        {
            session_t* p_sess = session_find_private(state, p_extconn->sockfd);

            if (NULL == p_sess)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                        ASD_LogOption_None, "Invalid session");
            }
            else
            {
                state->n_authenticated_id = p_sess->id;
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
STATUS session_get_authenticated_conn(Session* state,
                                      extnet_conn_t* p_authd_conn)
{
    STATUS st_ret = ST_ERR;
    if (state && state->b_initialized && state->n_authenticated_id >= 0)
    {
        if (p_authd_conn)
        {
            if (memcpy_s(p_authd_conn, sizeof(extnet_conn_t),
                         &state->sessions[state->n_authenticated_id].extconn,
                         sizeof(extnet_conn_t)))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                        ASD_LogOption_None, "memcpy_s: n_aunthenticated_id to \
						p_authd_conn copy failed.");
                st_ret = ST_ERR;
            }
            else
            {
                st_ret = ST_OK;
            }
        }
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
STATUS session_getfds(Session* state, session_fdarr_t* na_fds, int* pn_fds,
                      int* pn_timeout_ms)
{
    STATUS st_ret = ST_ERR;
    bool b_data_pending = false;
    bool b_set_zero_timeout = false;

    if (state && state->b_initialized && na_fds && pn_fds && pn_timeout_ms)
    {
        *pn_fds = 0;
        for (int i = 0; i < MAX_SESSIONS; i++)
        {
            session_t* p_sess = &state->sessions[i];
            if (!extnet_is_client_closed(state->extnet, &p_sess->extconn))
            {
                if (!p_sess->b_authenticated)
                {
                    int n_msremain =
                        (int)(1000 * (p_sess->t_auth_tout - time(0L)));
                    if (n_msremain < 0)
                    {
                        n_msremain = 0;
                    }
                    if (*pn_timeout_ms < 0 || n_msremain < *pn_timeout_ms)
                    {
                        *pn_timeout_ms = n_msremain;
#ifdef ENABLE_DEBUG_LOGGING
                        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                                ASD_LogOption_None,
                                "Session %d time remaining: %d", p_sess->id,
                                n_msremain);
#endif
                    }
                }
                session_get_data_pending(state, &p_sess->extconn,
                                         &b_data_pending);
                if (b_data_pending)
                    b_set_zero_timeout = true;
                (*na_fds)[(*pn_fds)] = p_sess->extconn.sockfd;
                (*pn_fds)++;
            }
        }
        if (b_set_zero_timeout)
        {
            *pn_timeout_ms = 0;
        }
        st_ret = ST_OK;
    }
    return st_ret;
}

/** @brief Indicate whether data is pending for a session
 *
 *  Given a session file descriptor, set data pending flag
 *
 *  @param [in] p_extconn External connection associated with session
 *  @param [in] bool flag indicating more data is pending for the connection
 *  @return Returns ST_ERR if session does not exist for associated
 *          file descriptor. ST_OK if successful.
 */
STATUS session_set_data_pending(Session* state, extnet_conn_t* p_extconn,
                                bool b_data_pending)
{
    STATUS st_ret = ST_ERR;
    session_t* p_sess;

    if (state && state->b_initialized && p_extconn)
    {
        p_sess = session_find_private(state, p_extconn->sockfd);

        if (!state->b_initialized || NULL == p_sess)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Invalid connection");
        }
        else
        {
            p_sess->b_data_pending = b_data_pending;
            st_ret = ST_OK;
        }
    }
    return st_ret;
}

/** @brief get flag indicating data is pending for a session
 *
 *  Given a session file descriptor, get the data pending flag
 *
 *  @param [in] p_extconn connection associated with session
 *  @param [out] bool flag indicating more data is pending for the connection
 *  @return Returns ST_ERR if session does not exist for associated
 *          file descriptor. ST_OK if successful.
 */
STATUS session_get_data_pending(Session* state, extnet_conn_t* p_extconn,
                                bool* b_data_pending)
{
    STATUS st_ret = ST_ERR;
    session_t* p_sess;

    if (state && state->b_initialized && p_extconn && b_data_pending)
    {
        p_sess = session_find_private(state, p_extconn->sockfd);

        if (NULL == p_sess)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Invalid connection");
        }
        else
        {
            *b_data_pending = p_sess->b_data_pending;
            st_ret = ST_OK;
        }
    }
    return st_ret;
}
