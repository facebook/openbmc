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
#include "ipmb.h"

/*
 * Function to handle IPMB messages
 */
void
lib_ipmb_handle(unsigned char bus_id,
            unsigned char *request, unsigned char req_len,
            unsigned char *response, unsigned char *res_len) {

  int s, t, len;
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

  if ((t=recv(s, response, MAX_IPMB_RES_LEN, 0)) > 0) {
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
