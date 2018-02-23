/*
 * fpc-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <openbmc/pal.h>

static void
print_usage_help(void) {
  printf("Usage: fpc-util [ bmc, switch ] --uart\n");
  printf("       fpc-util switch --reset\n");
  printf("       fpc-util --identify [on, off]\n");
}

int
main(int argc, char **argv) {

  uint8_t pos;
  uint8_t curr;
  int ret;


  if (argc != 3) {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--uart")) {
    if (!strcmp(argv[1], "bmc")) {
      pos = UART_POS_BMC;
    } else if (!strcmp(argv[1] , "switch")) {
      pos = UART_POS_PCIE_SW;
    } else {
      goto err_exit;
    }

    // Check for the current uart channel position
    ret = pal_get_uart_chan(&curr);
    if (ret)
      goto err_exit;

    if (curr == pos) {
       printf("UART channel already connected to %s console\n",
          (pos == UART_POS_BMC)? "BMC": "SWITCH");

    } else {
      printf("Moving UART channel to %s console\n",
          (pos == UART_POS_BMC)? "BMC": "SWITCH");
      // Set the UART channel to requested position
      ret = pal_set_uart_chan(pos);
      if (ret) {
        printf("Operation failed.\n");
        return -1;
      }
    }
    return 0;

  // Perform PCIe Switch Reset
  } else if ((!strcmp(argv[1] , "switch")) && (!strcmp(argv[2], "--reset"))) {
    ret = pal_reset_pcie_switch();
    if (ret) {
      printf("Operation failed.\n");
      return -1;
    }

  // Set the system_identify key to on or off value
  } else if ((!strcmp(argv[1] , "--identify")) &&
      (!strcmp(argv[2], "on") || !strcmp(argv[2], "off"))) {
    return pal_set_key_value("system_identify", argv[2]);
  } else {
    goto err_exit;
  }

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
