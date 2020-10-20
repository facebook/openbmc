/*
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
#include <syslog.h>
#include <pthread.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>



// Thread for update the uart_select
static void *
debug_card_handler() {
#define  DELAY_READ 500
  int ret = 0;
  uint8_t card_prsnt = 0x00;
  uint8_t uart_select = 0x00;
  uint8_t prev_uart_select = 0xff;
  char str[8] = "\n";

  while (1) {
    // we start updating uart position sts when card is present
    if ( pal_is_debug_card_prsnt(&card_prsnt) < 0 || card_prsnt == 0 ) {
      msleep(DELAY_READ); //sleep
      continue;
    }

    // get the uart position
    ret = pal_get_uart_select_from_cpld(&uart_select);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() Failed to get debug_card_uart_select\n", __func__);
      msleep(DELAY_READ); //sleep
      continue;
    }

    // check if we need to update kv
    if ( uart_select != prev_uart_select ) {
      //detect the value has been changed
      sprintf(str, "%u", uart_select);
      ret = kv_set("debug_card_uart_select", str, 0, 0);
      if (ret < 0)  {
        syslog(LOG_WARNING, "%s() Failed to set debug_card_uart_select\n", __func__);
      } else {
        //update prev_uart_select
        prev_uart_select = uart_select;
      }
    }
    msleep(DELAY_READ);
  }

  return 0;
}


int
main (int argc, char * const argv[]) {
  pthread_t tid_debug_card;
  int rc;
  int pid_file;

  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if(rc) {
    if(EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if (pthread_create(&tid_debug_card, NULL, debug_card_handler, NULL) < 0) {
    syslog(LOG_WARNING, "pthread_create for debug card error\n");
    exit(1);
  }

  pthread_join(tid_debug_card, NULL);

  return 0;
}
