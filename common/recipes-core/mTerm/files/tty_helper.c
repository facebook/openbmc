/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#include "tty_helper.h"
#include <syslog.h>

void writeData(int file, char* buf, int len, char* fname) {
  int wlen = 0;
  char* tbuf = buf;
  while (len > 0) {
    wlen = write(file, tbuf, len);
    if (wlen >= 0) {
      len -= wlen;
      tbuf = tbuf + wlen;
    } else {
      syslog(LOG_ERR, "writeData: write() failed to file %s ", fname);
      return;
    }
  }
}

static int setRaw(int tty_fd, struct termios* old_ttytio,
                  struct termios* new_ttytio, int setBaudRate) {

  memset(new_ttytio, 0, sizeof(struct termios));
  memset(old_ttytio, 0, sizeof(struct termios));

  if (tcgetattr(tty_fd, old_ttytio) < 0 ) {
    perror("tcgetter error");
    return -1;
  }
  memcpy(new_ttytio, old_ttytio, sizeof(struct termios));

  cfmakeraw(new_ttytio);

  if (setBaudRate) {
    cfsetspeed(new_ttytio, BAUDRATE);
  }
  tcflush(tty_fd, TCIFLUSH);

  if (tcsetattr(tty_fd, TCSANOW, new_ttytio) < 0) {
    perror("tcsetter attribute error");
    return -1;
  }

  return 1;
}

int openTty(const char* tty) {
  int fd;

  if((fd = open(tty, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
    perror("Cannot create a new fd for tty");
    exit(1);
  }
  if (fcntl(fd, F_SETFL, O_RDWR) < 0) {
    perror("Cannot set tty fd");
  }
  return fd;
}

ttyRaw* setTty(int fd, int setBaudRate) {
  ttyRaw* new_tty;

  new_tty = (ttyRaw*) malloc(sizeof(ttyRaw));
  if (new_tty == NULL) {
    perror("Malloc error");
    return NULL;
  }
  new_tty->fd = fd;
  if (setRaw(new_tty->fd, &new_tty->old_tio,
    &new_tty->new_tio, setBaudRate) < 0) {
    return NULL;
  }
  return new_tty;
}

void closeTty(ttyRaw* tty_raw) {
  if (!tty_raw ) {
    return;
  }
  if (isatty(tty_raw->fd)) {
    tcflush(tty_raw->fd, TCIFLUSH);
    tcsetattr(tty_raw->fd, TCSANOW, &tty_raw->old_tio);
  }
  free(tty_raw);
}
