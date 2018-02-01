/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include "ipmb.h"

static pthread_key_t rxkey, txkey;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void
destructor(void *buf)
{
  if (buf)
    free(buf);
}

static void
make_key()
{
  (void) pthread_key_create(&rxkey, destructor);
  (void) pthread_key_create(&txkey, destructor);
}

/*
 *  Return thread specific ipmb rx buffer
 */
ipmb_res_t*
ipmb_rxb()
{
  ipmb_res_t *buf = NULL;

  pthread_once(&key_once, make_key);
  if ((buf = pthread_getspecific(rxkey)) == NULL) {
    buf = malloc(MAX_IPMB_RES_LEN);
    pthread_setspecific(rxkey, buf);
  }

  return (ipmb_res_t*)buf;
}

/*
 *  Return thread specific ipmb tx buffer
 */
ipmb_req_t*
ipmb_txb()
{
  void *buf = NULL;

  pthread_once(&key_once, make_key);
  if ((buf = pthread_getspecific(txkey)) == NULL) {
    buf = malloc(MAX_IPMB_RES_LEN);
    pthread_setspecific(txkey, buf);
  }

  return (ipmb_req_t*)buf;
}

/*
 * Function to handle IPMB messages
 */
void
lib_ipmb_handle(unsigned char bus_id,
            unsigned char *request, unsigned short req_len,
            unsigned char *response, unsigned char *res_len) {

  int s, t, len;
  int r, retries = 5, delay = 20;
  struct sockaddr_un remote;
  char sock_path[64] = {0};
  struct timeval tv;

  sprintf(sock_path, "%s_%d", SOCK_PATH_IPMB, bus_id);

  // TODO: Need to update to reuse the socket instead of creating new
  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG
    syslog(LOG_WARNING, "lib_ipmb_handle: socket() failed\n");
#endif
    return;
  }

  // setup timeout for receving on socket
  tv.tv_sec = TIMEOUT_IPMB + 1;
  tv.tv_usec = 0;

  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, sock_path);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);

  if (connect(s, (struct sockaddr *)&remote, len) == -1) {
#ifdef DEBUG
    syslog(LOG_WARNING, "ipmb_handle: connect() failed\n");
#endif
    goto clean_exit;
  }

  if (send(s, request, req_len, 0) == -1) {
#ifdef DEBUG
    syslog(LOG_WARNING, "ipmb_handle: send() failed\n");
#endif
    goto clean_exit;
  }

  for ( r=0; r<retries; r++) {
    t = recv(s, response, MAX_IPMB_RES_LEN, 0);
    if ( t >= 0 || (t < 0 && errno != EINTR))
      break;
    //Errno==EINTR, retry,
    //Delay 20 ms
    usleep(delay * 1000);
  }

  if (t > 0) {
    *res_len = t;
  } else {
    if (t < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "lib_ipmb_handle: recv() failed\n");
#endif
    } else {
#ifdef DEBUG
      syslog(LOG_DEBUG, "Server closed connection\n");
#endif
    }
  }

clean_exit:
  close(s);

  return;
}

int
ipmb_send_buf (unsigned char bus_id, unsigned char tlen)
{
  unsigned char rlen = 0;

  lib_ipmb_handle(bus_id,
    (unsigned char *)ipmb_txb(), tlen,
    (unsigned char *)ipmb_rxb(), &rlen);

  if (rlen >= MIN_IPMB_RES_LEN)
    return (int)rlen;
  else
    return -1;
}

int
ipmb_send_internal (unsigned char bus_id,
  unsigned char addr,
  unsigned char netfn,
  unsigned char cmd, ...)
{
  unsigned char *tbuf;
  unsigned char *rbuf;
  unsigned char tlen = 0;
  unsigned char rlen = 0;
  int data_len;
  ipmb_req_t *req;
  int data;
  va_list dl; // data list

  // Get thread specific buffers
  tbuf = (unsigned char *)ipmb_txb();
  rbuf = (unsigned char *)ipmb_rxb();
  if (rbuf == NULL || tbuf == NULL)
    return -1;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = addr;
  req->netfn_lun = netfn;
  req->cmd = cmd;

  tlen = 0;
  va_start(dl, cmd);
  while ((data = va_arg(dl, int)) != OBMC_API_IPMB_CMD_END) {
    if (tlen == (MAX_IPMB_RES_LEN - MIN_IPMB_REQ_LEN)) {
      va_end(dl);
      return -1;
    }
    req->data[tlen++] = (uint8_t)data;
  }
  va_end(dl);
  tlen += MIN_IPMB_REQ_LEN;

  rlen = 0;
  lib_ipmb_handle(bus_id, tbuf, tlen, rbuf, &rlen);

  if (rlen >= MIN_IPMB_RES_LEN) {
    data_len = rlen - MIN_IPMB_RES_LEN;
  } else {
    data_len = -1;
  }

  return data_len;
}
