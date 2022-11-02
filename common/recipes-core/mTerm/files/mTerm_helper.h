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

#define ASCII_DELETE  0177
#define ESC_CHAR_HELP '?'
//#define ASCII_ESC 27 // ctrl + [
#define ASCII_CTRL_L 12 // ctrl + L
//#define ASCII_CTRL_X 24 // ctrl + X
#define ASCII_COLON 58 // :
#define ASCII_CARAT 94 // ^
#define ASCII_CR 015
#define BUF_SIZE 10
#define PATH_SIZE 64
/* SEND_SIZE definition:
 * Serial packet of 8N1 it 10bit.
 * The current baudrate of mTerm when using sol.sh is
 * 38400 bps(which COMe console is using now). So we
 * can get 3840=38400(baudrate)/10(bits). But we think it's
 * better that add a buffer, so we use 5120.
 * And if someone wants to change this, it should follow
 * baudrate/n_bits_of_per_byte_transfer_via_uart (depends
 * on uart config about the stop bits/parity bits and so on).
 */
#define SEND_SIZE 5120
#define FILE_SIZE_BYTES 300000
#define FILE_SIZE_MAX_BYTES 10000000
#define MAX_BYTE 5120

typedef enum escMode {
  EOL,
  ESC,
  CTRL_X,
  SEND
} escMode;

typedef struct bufStore {
  int  buf_fd;
  int  maxSizeBytes;
  char file[PATH_SIZE];
  char backupfile[PATH_SIZE];
  char needTimestamp;
  unsigned long lineNumber;
} bufStore;

typedef struct TlvHeader {
  uint16_t type;
  uint16_t length;
}TlvHeader;

void setFru();
//esc mode processing
void escHelp();
int processEscMode(int clientfd, char c, escMode* mode);
int escSend(int clientfd, char c, escMode* mode);
void escClose(int clientfd);
void charSend(int clientfd, char* c, int length);
// buffer processing
bufStore* createBuffer(const char *dev, int fsize);
void closeBuffer(bufStore* buf);
long int bufferGetLines(char* fname, int clientfd, int n, long int curr);
void writeToBuffer(bufStore *buf, char* data, int len);
// tx
int sendTlv(int fd, uint16_t type, void* value, uint16_t valLen);
int escSendBreak(int clientfd, char *c);
