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
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include "mTerm_helper.h"
#include "tty_helper.h"

static char g_fru[10];

void setFru(char *dev) {
    strncpy(g_fru, dev, sizeof(g_fru));
}

void escHelp(void) {
  printf("\r\n------------------TERMINAL MULTIPLEXER---------------------\r\n");
  printf("  CTRL-l ?   : Display help message.\r\n");
  printf("  CTRL-l x : Terminate the connection.\r\n");
  printf("  /var/log/mTerm_%s.log : Log location\r\n", g_fru);
  printf("  CTRL-l + b : Send Break\r\n");
  /*TODO: Log file read from tool*/
  //printf("  CTRL-L :N - For reading last N lines from end of buffer.\r\n");
  printf("\r\n-----------------------------------------------------------\r\n");
  return;
}

void escClose(int clientfd) {
  sendTlv(clientfd, 'x', NULL , 0);
  printf("Connection closed.\r\n");
 return;
}

int processEscMode(int clientfd, char c, escMode* mode) {
  if (c == ESC_CHAR_HELP) {
    escHelp();
    *mode = EOL;
    return 1;
  }
  if (c == ASCII_COLON) {
    *mode = SEND;
    return 1;
  }
  if (c == 'x') {
    escClose(clientfd);
    *mode = EOL;
    return 0;
  }
  if (isalpha(c) && (c == 'b')) {
    printf("Warning: Send BREAK \r\n");
    escSendBreak(clientfd, &c);
  }
  *mode = EOL;
  return 1;
}

int sendTlv(int fd, uint16_t type, void* value, uint16_t valLen) {
  TlvHeader header;
  header.type = type;
  header.length = valLen;

  struct iovec vec[2];
  vec[0].iov_base = &header;
  vec[0].iov_len = sizeof(header);

  int rc;
  vec[1].iov_base = value;
  vec[1].iov_len = valLen;

  rc = writev(fd, vec, 2);

  // TODO: Check the return code, and asynchronously retry later if
  // we were only able to write part of the data.
  if ((rc < 0) || (rc < (vec[0].iov_len + vec[1].iov_len))) {
    perror("Write error");
    return -1;
  }
  return rc;
}

int escSendBreak(int clientfd, char *c) {
  sendTlv(clientfd, ASCII_CTRL_L, c, 1);
  return 1;
}

int escSend(int clientfd, char c, escMode* mode) {
  static char rbuf[BUF_SIZE];
  static int rbuf_len = 0;

  if (c == ASCII_CR) {
    sendTlv(clientfd, ASCII_CTRL_L, rbuf, rbuf_len);
    *mode = EOL;

    //reset buf and length
    rbuf_len = 0;
    memset(rbuf, 0, sizeof(rbuf));
   } else {
     if (!isdigit(c) || (rbuf_len > BUF_SIZE)) {
       rbuf_len = 0;
       memset(rbuf, 0, sizeof(rbuf));
       return -1;
     }
     rbuf[rbuf_len++] = c;
   }
  return 1;
}

void charSend(int clientfd, char* c, int length) {
  sendTlv(clientfd, ASCII_CARAT, c, length);
}

bufStore* createBuffer(const char *dev, int fsize) {
  bufStore* buf;

  buf = (bufStore*)malloc(sizeof(bufStore));
  if (buf == NULL) {
    perror("Malloc error");
    return NULL;
  }

  int ret;
  ret = snprintf(buf->file, sizeof(buf->file), "/var/log/mTerm_%s.log", dev);
  if ((ret < 0) || (ret >= sizeof(buf->file))) {
    perror("mTerm: Received dev name too long to create buffer file");
    free(buf);
    return NULL;
  }

  ret = snprintf(buf->backupfile, sizeof(buf->backupfile),
    "/var/log/mTerm_%s_backup.log", dev);
  if ((ret < 0) || (ret >= sizeof(buf->backupfile))) {
    perror("mTerm: Received dev name too long to create backup buffer file");
    free(buf);
    return NULL;
  }

  buf->buf_fd = open(buf->file, O_RDWR | O_APPEND | O_CREAT, 0666) ;
  buf->maxSizeBytes = fsize;
  buf->needTimestamp = 1;
  return buf;
}

void closeBuffer(bufStore* buf) {
  if (!buf) {
    return;
  }
  close(buf->buf_fd);
  free(buf);
}

/* Write human-readable timestamp with line number in the provided buffer */
void writeTimestampToBuffer(bufStore *buf) {

  time_t cur_time;
  size_t dateLen;
  char dateBuff[64];

  time(&cur_time);

  if (!ctime_r(&cur_time, dateBuff))
    strcpy(dateBuff, "unknown time ");

  dateLen = strlen(dateBuff);
  dateBuff[dateLen - 1] = ' ';
  snprintf(dateBuff + dateLen, sizeof(dateBuff) - dateLen, "%07lu ", buf->lineNumber++);
  writeData(buf->buf_fd, dateBuff, strlen(dateBuff), "buffer");
}

void writeToBuffer(bufStore *buf, char* data, int len) {
   bool rotate = false;
   struct stat file_stat;
   int rc = stat(buf->file, &file_stat), nbytes = len, cur_len;
   char *cur = data, *prev = data;

   if (rc != 0) {
     if (errno == ENOENT) {
       // Maybe someone externally removed our buffer file. Force file rotation.
       rotate = true;
     } else {
       // We couldn't figure out if the file needs to be rotated.
       // Don't rotate the file.  Continue and log the data anyway, though.
       syslog(LOG_WARNING, "Error determining existing buffer file size: "
              "errno=%d", errno);
     }
   } else {
     if (file_stat.st_size >= buf->maxSizeBytes) {
       rotate = true;
     }
   }

   // Rollover to a backup file when buffer hits filesize
   if (rotate) {
     close(buf->buf_fd);
     rename(buf->file, buf->backupfile);
     buf->buf_fd = open(buf->file, O_RDWR | O_APPEND | O_CREAT, 0666) ;
     if (buf->buf_fd < 0) {
       perror("Cannot open the mTerm buffer log file");
       exit(-1);
     }
   }

  /*
   * Treat data as byte array but try to seek out newline characters. When they are
   * found, add current timestamp and sequential line number.
   */
   while ((cur = memchr(cur, '\n', nbytes)) || nbytes) {
     if (buf->needTimestamp) {
       writeTimestampToBuffer(buf);
       buf->needTimestamp = 0;
     }
     /* there is no new line in this buffer, move on */
     if (!cur) {
       writeData(buf->buf_fd, prev, nbytes, "buffer");
       break;
     }

     cur_len = cur - prev + 1;
     nbytes -= cur_len;

     writeData(buf->buf_fd, prev, cur_len, "buffer");
     prev = ++cur;
     buf->needTimestamp = 1;
  }

}

long int bufferGetLines(char* fname, int clientfd, int nlines, long int curr) {
  FILE* fd;
  int count = 0;
  int cursize = 0;
  char ch;
  char buf[SEND_SIZE];

  if (nlines == 0) {
    return curr;
  }

  fd = fopen(fname, "r");
  if (fd == NULL) {
    perror("fopen");
    return -1;
  }

  if (curr) {
    fseek(fd, curr, SEEK_END);
  } else {
    fseek(fd, 0, SEEK_END);
    curr = ftell(fd);
  }

  while (curr) {
    fseek(fd, --curr, SEEK_SET);
    if (fgetc(fd) == '\n') {
      if (count++ == nlines) {
         break;
      }
    }
  }
  count = 0;
  if (!curr) {
    fseek(fd, -1, SEEK_CUR);
  }
  while (count < nlines) {
    ch = fgetc(fd);
    if(ch == '\n') {
      count++;
    }
    buf[cursize++] = ch;

    if(cursize == SEND_SIZE) {
      send(clientfd, buf , SEND_SIZE, 0);
      memset(buf, 0, sizeof(buf));
      cursize = 0;
    }
  }
  if (cursize) {
    send(clientfd, buf, cursize, 0);
  }
  fclose(fd);
  return curr;
}
