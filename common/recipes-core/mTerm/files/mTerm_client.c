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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <signal.h>
#include "tty_helper.h"
#include "mTerm_helper.h"

static sig_atomic_t sigexit = 0;

static int createClientSocket(const char *dev) {
  struct sockaddr_un remote;
  int sockfd;
  int len;

  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("mTerm_client: Socket create error");
    return -1;
  }
  if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0){
    perror("mTerm_client: Socket could not be made non-blocking");
    close(sockfd);
    return -1;
  }
  memset(&remote, 0, sizeof(remote));
  remote.sun_family = AF_UNIX;

  int ret;
  ret = snprintf(remote.sun_path, sizeof(remote.sun_path),
    "/var/run/mTerm_%s_socket", dev);
  if ((ret < 0) || (ret >= sizeof(remote.sun_path))) {
    perror("mTerm_client: Received dev name too long");
    close(sockfd);
    return -1;
  }

  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(sockfd, (struct sockaddr *)&remote, len) == -1) {
    perror("mTerm_client: Connect error");
    close(sockfd);
    return -1;
  }
  return sockfd;
}

/* TODO: Enhance
  - escSend() to be error tolorent
  - handle sending esc sequence to tty
*/
static int readFromStdin(int clientfd, int stdi) {
  static escMode mode = EOL;
  char c[MAX_BYTE];
  int nbytes;

  nbytes = read(stdi, &c, sizeof(c));
  if (nbytes < 0) {
    perror("mTerm_client: Client socket read error to mTerm_server");
    return 0;
  }
  if((mode == EOL) && (c[0] == ASCII_CTRL_L)) {
    mode = ESC;
    return 1;
  } else if (mode == ESC) {
    return (processEscMode(clientfd, c[0], &mode));
  } else if (mode == SEND) {
    if (escSend(clientfd, c[0] , &mode) < 0) {
      mode = EOL;
      perror("mTerm_client: Invalid input to read buffer");
    }
    return 1;
  }
  charSend(clientfd, &c[0], nbytes);
  return 1;
}

static int writeToStdout(int clientfd, int stdo) {
  char buf[SEND_SIZE];
  int nbytes;

  nbytes = read(clientfd, buf, sizeof(buf));
  if (nbytes < 0) {
    perror("mTerm_client: Client socket read error to stdout");
    return 0;
  }
  writeData(stdo, buf, nbytes, "stdout");
  return(nbytes);
}

static void exit_handler(int sig) {
  sigexit = sig;
  return;
}

static void connectClient(const char *dev) {
  fd_set master;
  fd_set read_fds;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  signal(SIGHUP, exit_handler);
  signal(SIGINT, exit_handler);
  signal(SIGQUIT, exit_handler);
  signal(SIGPIPE, exit_handler);
  signal(SIGTSTP, exit_handler);
  signal(SIGTERM, exit_handler);

  int stdi;
  struct ttyRaw* tty_in;
  stdi = STDIN_FILENO;
  if (!isatty(stdi)) {
    return;
  }
  tty_in = setTty(stdi, 0);

  int stdo;
  struct ttyRaw* tty_out;
  stdo = STDOUT_FILENO;
  if (!isatty(stdo)) {
    closeTty(tty_in);
    return;
  }
  tty_out = setTty(stdo, 0);

  int clientfd;
  clientfd = createClientSocket(dev);
  if (clientfd < 0) {
    closeTty(tty_out);
    closeTty(tty_in);
    return;
  }

  FD_SET(tty_in->fd,&master);
  FD_SET(clientfd, &master);

  int fdmax = 0;
  fdmax = (clientfd > tty_in->fd) ? clientfd : tty_in->fd;

  for(;;) {
    if (sigexit) {
      break;
    }
    read_fds = master;
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("mTerm_client: select error");
      break;
     }
    if (FD_ISSET(stdi, &read_fds)) {
      if (!readFromStdin(clientfd, tty_in->fd)) {
        break;
      }
    }
    if (FD_ISSET(clientfd, &read_fds)) {
      if (!writeToStdout(clientfd,tty_out->fd)) {
        break;
      }
    }
  }
  closeTty(tty_out);
  closeTty(tty_in);
  close(clientfd);
}

static void
print_usage() {
  printf("Usage example: /usr/local/bin/mTerm_client <fru> \n");
}

int main(int argc, char **argv)
{
   if (argc != 2) {
     print_usage();
     exit(1);
   }
   setFru(argv[1]);
   escHelp();
   connectClient(argv[1]);
   return sigexit;
}
