/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#include "ipmi.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "/tmp/ipmi_socket"
#define MAX_IPMI_RES_LEN 100

/*
 * Function to handle IPMI messages
 */
void
ipmi_handle(unsigned char *request, unsigned char req_len,
            unsigned char *response, unsigned char *res_len) {

  int s, t, len;
  struct sockaddr_un remote;

  // TODO: Need to update to reuse the socket instead of creating new
  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    syslog(LOG_ALERT, "ipmi_handle: socket() failed\n");
    return;
  }

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_PATH);
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);

  if (connect(s, (struct sockaddr *)&remote, len) == -1) {
    syslog(LOG_ALERT, "ipmi_handle: connect() failed\n");
    return;
  }

  if (send(s, request, req_len, 0) == -1) {
    syslog(LOG_ALERT, "ipmi_handle: send() failed\n");
    return;
  }

  if ((t=recv(s, response, MAX_IPMI_RES_LEN, 0)) > 0) {
    *res_len = t;
  } else {
    if (t < 0) {
      syslog(LOG_ALERT, "ipmi_handle: recv() failed\n");
    } else {
      printf("Server closed connection");
    }

    return;
  }

  close(s);

  return;
}
