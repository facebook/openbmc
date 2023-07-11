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
#include <ctype.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <errno.h>
#include <syslog.h>
#include <sys/uio.h>
#include "tty_helper.h"
#include "mTerm_helper.h"

#define NUM_CLIENTS 10

static size_t file_size = FILE_SIZE_BYTES;

static int createServerSocket(const char* dev) {
  int serverFd;
  struct sockaddr_un local;

  if ((serverFd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    syslog(LOG_ERR, "mTerm_server: Cannot create a new socket\n");
    return -1;
  }

  local.sun_family = AF_UNIX;

  int ret;
  ret = snprintf(local.sun_path, sizeof(local.sun_path),
    "/var/run/mTerm_%s_socket", dev);
  if ((ret < 0) || (ret >= (int)sizeof(local.sun_path))) {
    syslog(LOG_ERR, "mTerm_server: Received dev name too long");
    close(serverFd);
    return -1;
  }

  unlink(local.sun_path);

  int len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(serverFd, (struct sockaddr *)&local, len) == -1) {
    syslog(LOG_ERR, "mTerm_server: Cannot bind to socket\n");
    close(serverFd);
    return -1;
  }

  if (listen(serverFd, NUM_CLIENTS) == -1) {
    syslog(LOG_ERR, "mTerm_server: Cannot listen to clients\n");
    close(serverFd);
    return -1;
  }

  if (fcntl(serverFd, F_SETFL, O_NONBLOCK) < 0){
    syslog(LOG_ERR, "mTerm_server: Cannot set socket to non-blocking\n");
    close(serverFd);
    return -1;
  }
  return serverFd;
}

static int acceptClient(int serverFd) {
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;
  int fd;

  addrlen = sizeof remoteaddr;
  fd = accept(serverFd, (struct sockaddr *)&remoteaddr, &addrlen);
  if (fd == -1) {
    syslog(LOG_ERR, "mTerm_server: Server errror on accept()\n");
    return -1;
  }
  syslog(LOG_INFO, "mTerm_server: Client socket %d created\n", fd);
  return fd;
}

void closeClient(fd_set* master, int clientfd) {
  close(clientfd);
  FD_CLR(clientfd, master);
}

void sendBreak(int clientFd, int solFd, char *c) {
  syslog(LOG_INFO, "mTerm_server: Client socket %d send BREAK+%c\n", clientFd,*c);
  tcsendbreak(solFd, 1);
}

static void processClient(fd_set* master, int clientFd , int solFd,
                          bufStore *buf) {
  char data[SEND_SIZE];
  int nbytesHeader = 0, nbytesData = 0;
  TlvHeader header;
  struct iovec vecHeader, vecData;
  char* tbuf;

  /* TODO: server should be able to handle data for a tlv over multiple reads */
  /* Reading header from client FD */
  vecHeader.iov_base = &header;
  vecHeader.iov_len = sizeof(header);
  nbytesHeader = readv(clientFd, &vecHeader, 1);
  /* Reading client data to which header length info correspond */
  vecData.iov_base = &data;
  vecData.iov_len = header.length;
  nbytesData = readv(clientFd, &vecData, 1);

  if (nbytesHeader <= 0) {
    if (nbytesHeader == 0) {
      syslog(LOG_ERR, "mTerm_server: Client socket %d hung up\n", clientFd);
    } else {
      syslog(LOG_ERR, "mTerm_server: Error on read fd=%d\n", clientFd);
    }
    closeClient(master, clientFd);
  } else if ((size_t)nbytesHeader < sizeof(TlvHeader)) {
    // TODO: Potentially we should use a per-client buffer, for now close
    //  Client connection
    syslog(LOG_ERR, "mTerm_server: Error on read fd=%d socket_nbytes=%d\n", clientFd, nbytesHeader);
    closeClient(master, clientFd);
  } else if (header.length != nbytesData ) {
    syslog(LOG_ERR,"mTerm_server: data is not correct to receive nbytes=%d, length=%d",nbytesData, header.length);
    //syslog(LOG_ERR, "mTerm_server: Received %d bytes for fd=%d dropping message.\n",nbytes, clientFd);
  } else {
    switch (header.type) {
      case ASCII_CTRL_L:
        /* TODO: Server should store client pointers for last reference of
         buffer read per client, thus subsequent reads can be based on the
         last reference
        */
        tbuf = vecData.iov_base;
        if (isalpha(*tbuf)) {
          if (*tbuf == 'b') {
            sendBreak(clientFd, solFd, tbuf);
          } else {
            syslog(LOG_ERR, "mTerm_server: Received incorrect break char");
          }
        } else {
          bufferGetLines(buf->file, clientFd, atoi(vecData.iov_base), 0);
        }
        break;
      case 'x':
        syslog(LOG_INFO, "mTerm_server: Client socket %d closed\n", clientFd);
        closeClient(master, clientFd);
        break;
      case ASCII_CARAT:
        writeData(solFd, vecData.iov_base, nbytesData, "tty");
        break;
      default:
        syslog(LOG_ERR, "mTerm_server: Received unknown tlv\n");
        break;
     }
  }
}

/* TODO: We should support non-blocking writes to clients, and buffer data if
they don't read it fast enough. If we end up buffering too much data for any
particular client then we should disconnect that client.
*/
static int processSol(fd_set* master, int serverfd, int fdmax,
                      int solFd, bufStore *buf) {
  char data[SEND_SIZE];
  int nbytes;
  int currFd;

  nbytes = read(solFd, data, sizeof(data));
  if (nbytes > 0) {
    for (currFd = 0; currFd <= fdmax; currFd++) {
      if (FD_ISSET(currFd, master)) {
        if ((currFd != serverfd) && (currFd != solFd)) {
          if (send(currFd, data, nbytes, 0) == -1) {
            syslog(LOG_ERR, "mTerm_server: Error on send fd=%d\n", currFd);
            closeClient(master, currFd);
            syslog(LOG_ERR, "mTerm_server: Terminated client fd=%d\n", currFd);
          }
        }
      }
    }
    writeToBuffer(buf, data, nbytes);
  } else if (nbytes < 0) {
    syslog(LOG_ERR, "mTerm_server: Error on read fd=%d\n", solFd);
    return -1;
  }
  return 1;
}

static void connectServer(const char *stty, const char *dev) {
  int fdmax, newfd;

  fd_set master, read_fds;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  int serverfd;
  serverfd = createServerSocket(dev);
  if (serverfd < 0) {
    syslog(LOG_ERR, "mTerm_server: Failed to create server socket\n");
    return;
  }

  struct ttyRaw* tty_sol;
  tty_sol = setTty(openTty(stty), 1);
  if (!tty_sol) {
    syslog(LOG_ERR, "mTerm_server: Failed to set tty to raw mode\n");
    close(serverfd);
    return;
  }

  struct bufStore* buf;
  buf = createBuffer(dev, file_size);
  if (!buf || (buf->buf_fd < 0)) {
    syslog(LOG_ERR, "mTerm_server: Failed to create the log file\n");
    closeTty(tty_sol);
    close(serverfd);
    return;
  }

  FD_SET(serverfd, &master);
  FD_SET(tty_sol->fd,&master);
  fdmax = (serverfd > tty_sol->fd) ? serverfd : tty_sol->fd;

  for(;;) {
    read_fds = master;
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      syslog(LOG_ERR, "mTerm_server: Server socket: select error\n");
      break;
    }
    if (FD_ISSET(serverfd, &read_fds)) {
      newfd = acceptClient(serverfd);
      if (newfd < 0) {
        syslog(LOG_ERR, "mTerm_server: Error on accepting client\n");
      } else {
        FD_SET(newfd, &master);
        if (newfd > fdmax) {
          fdmax = newfd;
        }
      }
    }
    if (FD_ISSET(tty_sol->fd, &read_fds)) {
      if ( processSol(&master, serverfd, fdmax, tty_sol->fd, buf) < 0) {
        break;
      }
    }
    int i;
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if ((i == serverfd) || (i == tty_sol->fd)) {
          continue;
        } else {
          processClient(&master, i, tty_sol->fd, buf);
        }
      }
    }
  }
  closeTty(tty_sol);
  close(serverfd);
  closeBuffer(buf);
}

static void
print_usage() {
  printf("Usage:\t/usr/local/bin/mTerm_server <fru> /dev/ttyS*\n"
      "\t/usr/local/bin/mTerm_server <fru> /dev/ttyS* baudrate\n"
      "\t/usr/local/bin/mTerm_server <fru> /dev/ttyS* baudrate max-log-size\n\n"
      "\tDefault baudrate: 57600\n"
      "\tDefault max log size: 300 KB\n");
}

int main(int argc, char **argv) {
  if (argc < 3 || argc > 5) {
    print_usage();
    exit(1);
  }

  char *stty, *dev;
  dev = argv[1];
  stty = argv[2];
  baudrate = BAUDRATE;
  file_size = FILE_SIZE_BYTES;

  if (argc >= 4) {
    baudrate = strtol(argv[3], NULL, 10);
    if (errno)
      baudrate = BAUDRATE;
  }

  if (argc == 5) {
    file_size = strtol(argv[4], NULL, 10);
    if (errno || file_size < FILE_SIZE_BYTES || file_size > FILE_SIZE_MAX_BYTES) {
      printf("File size must be between %d and %d bytes\n", FILE_SIZE_BYTES, FILE_SIZE_MAX_BYTES);
      exit(-1);
    }
  }

  int ret;
  char file[PATH_SIZE];
  char *tty = strstr(stty, "/dev/");
  tty += strlen("/dev/");
  ret = snprintf(file, sizeof(file), "/var/lock/LCK..%s", tty);
  if ((ret <0) || ret >= (int)sizeof(file)) {
    perror("mTerm_server: dev name too long for tty lockfile");
    return -1;
  }

  if (access(file, F_OK) == 0) {
    perror("mTerm_server: dev serial is not available");
    return -1;
  }

  ret = snprintf(file, sizeof(file), "/var/lock/mTerm_%s", dev);
  if ((ret < 0) || (ret >= (int)sizeof(file))) {
    perror("mTerm_server: dev name too long for lockfile");
    return -1;
  }

  int lock_file;
  lock_file = open(file, O_CREAT | O_RDWR, 0666);
  if (lock_file < 0) {
    perror("mTerm_server: failed to acquire lock");
    exit(-1);
  }

  int rc;
  rc = flock(lock_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (errno == EWOULDBLOCK) {
      perror("Another mTerm instance is running...");
      exit(1);
    }
  } else {
    openlog("mTerm_log", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "mTerm: daemon started");

    connectServer(stty, dev);
  }
  if (lock_file >= 0) {
    unlink(file);
  }
 return 0;
}
