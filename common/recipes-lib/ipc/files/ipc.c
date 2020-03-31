/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "ipc.h"

#ifdef DEBUG
#undef DEBUG
#ifdef __TEST__
#define DEBUG(fmt, ...) fprintf(stderr, "INFO: " fmt, ## __VA_ARGS__)
#else
#define DEBUG(fmt, ...) syslog(LOG_INFO, fmt, ## __VA_ARGS__)
#endif
#else
#define DEBUG(fmt, ...)
#endif

#ifdef __TEST__
#define ERROR(fmt, ...) fprintf(stderr, "ERROR: " fmt, ## __VA_ARGS__)
#define CRITICAL(fmt, ...) fprintf(stderr, "CRITICAL: " fmt, ## __VA_ARGS__)
#else
#define ERROR(fmt, ...) syslog(LOG_ERR, fmt, ## __VA_ARGS__)
#define CRITICAL(fmt, ...) syslog(LOG_CRIT, fmt, ## __VA_ARGS__)
#endif

#define STACK_SIZE (1024 * 32)
#define MAX_RETRIES 5
#define CLIENT_TIMEOUT 16

#define WAIT_CLIENT_RETRIES 5
#define ACCEPT_RECOVER_RETRIES 5

#define SAVE_ERRNO_RUN(exp)  \
  do {                       \
    int saved_errno = errno; \
    exp;                     \
    errno = saved_errno;     \
  } while (0)

struct service_s {
  ipc_handle_req_t handle_req;
  client_t base_cli;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
  int             num_active;
  int             active_limit;
};

static void set_sock_timeout(int sock, int timeout)
{
  if (timeout >= 0) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) != 0) {
      DEBUG("%s failed to set timeout (%d)", __func__, timeout);
    }
  }
}

int ipc_send_req(const char *endpoint, uint8_t *req, size_t req_len,
                 uint8_t *resp, size_t *resp_len, int timeout)
{
  struct sockaddr_un remote;
  int len, retry = 0, sockfd;
  size_t max_resp;

  if (!req || !req_len || !resp || !resp_len || !*resp_len) {
    DEBUG("%s(%s) bad parameters passed", __func__, endpoint);
    errno = EINVAL;
    return -1;
  }


  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    DEBUG("%s(%s) failed to create socket (%s)", __func__, endpoint, strerror(errno));
    return -1;
  }

  set_sock_timeout(sockfd, timeout);

  remote.sun_family = AF_UNIX;
  sprintf(remote.sun_path, "/tmp/%s", endpoint);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);

  if (connect(sockfd, (struct sockaddr *)&remote, len) == -1) {
    DEBUG("%s(%s) failed to connect (%s)", __func__, endpoint, strerror(errno));
    goto error;
  }
  
  if (send(sockfd, req, req_len, MSG_NOSIGNAL) != req_len) {
    DEBUG("%s(%s) failed to send (%s)", __func__, endpoint, strerror(errno));
    goto error;
  }

  max_resp = *resp_len;
  while ((len = recv(sockfd, resp, max_resp, 0)) < 0) {
    if ((errno != EINTR && errno != EWOULDBLOCK) ||
        (retry++ >= MAX_RETRIES)) {
      DEBUG("%s(%s) failed to recv (%s)", __func__, endpoint, strerror(errno));
      goto error;
    }
    DEBUG("%s(%s) recv interrupted (%s)", __func__, endpoint, strerror(errno));
    usleep(20 * 1000);
  }
  *resp_len = len;

  close(sockfd);
  return 0;

error:
  SAVE_ERRNO_RUN(close(sockfd));
  return -1;
}

int ipc_recv_req(client_t *cli, uint8_t *req, size_t *req_len, int timeout)
{
  int r;
  int ret = -1;
  int max = (int)*req_len;
  if (!cli || !req || !req_len || !*req_len) {
    return -1;
  }

  set_sock_timeout(cli->fd, timeout);
  
  for (r = 0; r < MAX_RETRIES; r++) {
    int rx_len = recv(cli->fd, req, max, 0);
    if (rx_len >= 0) {
      ret = 0;
      *req_len = rx_len;
      break;
    }
    if (rx_len < 0 && errno != EINTR) {
      DEBUG("%s(%s) failed to recv (%s)", __func__, cli->endpoint, strerror(errno));
      break;
    }
    usleep(20 * 1000);
  }
  return ret;
}

static void cli_done(client_t *cli)
{
  service_t *svc = cli->svc;
  cli->svc = NULL;
  if (svc) {
    close(cli->fd);
    free(cli);
    pthread_mutex_lock(&svc->mutex);
    svc->num_active--;
    pthread_cond_signal(&svc->cond);
    pthread_mutex_unlock(&svc->mutex);
  }
}

int ipc_send_resp(client_t *cli, uint8_t *resp, size_t resp_len)
{
  int ret = 0;
  if (!cli || !resp || !resp_len) {
    return -1;
  }
  if (send(cli->fd, resp, resp_len, MSG_NOSIGNAL) < 0) {
    DEBUG("%s(%s) failed to recv (%s)", __func__, cli->endpoint, strerror(errno));
    ret = -1;
  } else {
    cli_done(cli);
  }
  return ret;
}

static void *conn_handler(void *param)
{
  client_t *cli = (client_t *)param;
  ipc_handle_req_t handle_req = cli->svc->handle_req;
  if(handle_req(cli)) {
    cli_done(cli);
  }
  pthread_exit(NULL);
  return NULL;
}

static client_t *get_client(service_t *svc)
{
  client_t *cli = NULL;
  struct timespec ts;
  struct timeval tp;
  int rc;

  gettimeofday(&tp, NULL);
  ts.tv_sec = tp.tv_sec + CLIENT_TIMEOUT + 1;
  ts.tv_nsec = 0;

  pthread_mutex_lock(&svc->mutex);
  while (svc->num_active >= svc->active_limit) {
    rc = pthread_cond_timedwait(&svc->cond, &svc->mutex, &ts);
    if (rc == ETIMEDOUT) {
      break;
    }
  }
  if (rc != ETIMEDOUT) {
    cli = calloc(1, sizeof(*cli));
    if (cli) {
      svc->num_active++;
      memcpy(cli, &svc->base_cli, sizeof(*cli));
    }
  }
  pthread_mutex_unlock(&svc->mutex);
  return cli;
}

static void *svc_thread(void *param)
{
  service_t *svc = (service_t *)param;
  client_t *base_cli = &svc->base_cli;
  struct sockaddr_un local, remote;
  int sock, conn;
  pthread_attr_t attr;
  int len;
  int cli_retries = WAIT_CLIENT_RETRIES;
  int acc_retries = ACCEPT_RECOVER_RETRIES;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize(&attr, STACK_SIZE);

  if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    DEBUG("%s(%s) failed to create socket (%s)", __func__, base_cli->endpoint, strerror(errno));
    goto bail;
  }

  local.sun_family = AF_UNIX;
  sprintf(local.sun_path, "/tmp/%s", base_cli->endpoint);
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(sock, (struct sockaddr *)&local, len) == -1) {
    DEBUG("%s(%s) failed to bind (%s)", __func__, base_cli->endpoint, strerror(errno));
    goto close_bail;
  }

  if (listen(sock, 5) == -1) {
    DEBUG("%s(%s) failed to listen (%s)", __func__, base_cli->endpoint, strerror(errno));
    goto close_bail;
  }

  while (1) {
    socklen_t t = sizeof(remote);
    pthread_t tid;
    client_t *cli = get_client(svc);
    if (!cli) {
      if (--cli_retries <= 0) {
        CRITICAL("%s(%s) outstanding clients %d exceeded limit %d",
          __func__, base_cli->endpoint, svc->num_active, svc->active_limit);
        break;
      }
      ERROR("%s(%s) outstanding clients %d exceeded limit %d retrying in 5 seconds",
        __func__, base_cli->endpoint, svc->num_active, svc->active_limit);
      sleep(5);
      continue;
    }
    cli_retries = WAIT_CLIENT_RETRIES;

    while (--acc_retries > 0 && (conn = accept(sock, (struct sockaddr *)&remote, &t)) < 0) {
      ERROR("%s(%s) failed to accept (%s) retrying in 5 seconds", __func__, base_cli->endpoint, strerror(errno));
      sleep(5);
      continue;
    }
    if (acc_retries <= 0 || conn < 0) {
      CRITICAL("%s(%s) failed to accept (%s) retrying in 5 seconds", __func__, base_cli->endpoint, strerror(errno));
      cli_done(cli);
      break;
    }
    acc_retries = ACCEPT_RECOVER_RETRIES;
    cli->fd = conn;
    if (pthread_create(&tid, &attr, conn_handler, (void *)cli)) {
      CRITICAL("%s(%s) failed to create thread (%s)", __func__, base_cli->endpoint, strerror(errno));
      cli_done(cli);
    }
  }
close_bail:
  close(sock);
bail:
  pthread_exit(NULL);
  return NULL;
}

int ipc_start_svc(const char *endpoint, ipc_handle_req_t handle_req, int max_active, void *svc_cookie, pthread_t *waiter)
{
  pthread_t tid;
  pthread_attr_t attr;
  int ret = 0;
  service_t *svc;

  if (strlen(endpoint) >= MAX_ENDPOINT_LEN - 1) {
    return -1;
  }

  svc = calloc(1, sizeof(*svc));
  if (!svc) {
    return -1;
  }

  strcpy(svc->base_cli.endpoint, endpoint);
  svc->base_cli.svc_cookie = svc_cookie;
  svc->base_cli.fd = -1;
  svc->handle_req = handle_req;
  pthread_mutex_init(&svc->mutex, NULL);
  pthread_cond_init(&svc->cond, NULL);
  svc->base_cli.svc = svc;
  svc->num_active = 0;
  svc->active_limit = max_active;

  pthread_attr_init(&attr);
  if (!waiter)
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  if (pthread_create(&tid, &attr, svc_thread, svc)) {
      DEBUG("%s(%s) failed to start thread (%s)", __func__, endpoint, strerror(errno));
      free(svc);
      ret = -1;
  }
  pthread_attr_destroy(&attr);

  if (waiter)
    *waiter = tid;

  return ret;
}

#ifdef __TEST__
#include <assert.h>
char *svc_cookie = "test_cookie";

int test_handle_req(client_t *cli)
{
  uint8_t req[32] = {0};
  uint8_t resp[32] = {1,2,3,4};
  size_t len = 32;
  static int req_num = 0;
  int reqno = req_num++;

  assert(strcmp(cli->endpoint, "test_svc") == 0);
  assert(cli->svc_cookie == svc_cookie);
  if (ipc_recv_req(cli, req, &len, 1) != 0) {
    assert(0);
    return -1;
  }
  sleep(10);
  assert(memcmp(req, resp, 4) == 0);

  if (ipc_send_resp(cli, resp, 4) != 0) {
    if (reqno != 0)
      assert(0);
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int rc;
  rc = ipc_start_svc("test_svc", test_handle_req, 1, svc_cookie, NULL);
  assert(rc == 0);
  sleep(1);
  uint8_t req[32] = {1,2,3,4};
  uint8_t resp[32] = {0};
  size_t resp_len = 32;
  rc = ipc_send_req("test_svc", req, 4, resp, &resp_len, 2);
  printf("rc = %d\n", rc);
  assert(rc != 0);
  printf("First request timed out!\n");
  rc = ipc_send_req("test_svc", req, 4, resp, &resp_len, 20);
  assert(rc == 0);
  assert(memcmp(req, resp, 4) == 0);
  printf("Second request succeeded!\n");
  return 0;
}
#endif
