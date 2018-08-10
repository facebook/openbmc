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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <openbmc/ipc.h>

#define MAX_IPMI_RES_LEN 300

/*
 * Function to handle IPMI messages
 */
void
lib_ipmi_handle(unsigned char *request, unsigned char req_len,
            unsigned char *response, unsigned short *res_len) {

  size_t resp_len = MAX_IPMI_RES_LEN;

  if (ipc_send_req(SOCK_PATH_IPMI, request, (size_t)req_len, response, &resp_len, TIMEOUT_IPMI + 1) == 0) {
    *res_len = (unsigned short)resp_len;
  }
}
