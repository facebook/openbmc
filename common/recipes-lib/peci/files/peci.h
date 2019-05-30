/*
 *
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
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

#ifndef __PECI_H__
#define __PECI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define PECI_DEVICE "/dev/peci-0"
#define PECI_BUFFER_SIZE 32

enum peci_cmd {
  PECI_CMD_XFER = 0,
  PECI_CMD_MAX
};

struct peci_xfer_msg {
  uint8_t addr;
  uint8_t tx_len;
  uint8_t rx_len;
  uint8_t tx_buf[PECI_BUFFER_SIZE];
  uint8_t rx_buf[PECI_BUFFER_SIZE];
} __attribute__((__packed__));

// IOCTL
#define PECI_IOC_BASE  0xb7
#define PECI_IOC_XFER _IOWR(PECI_IOC_BASE, PECI_CMD_XFER, struct peci_xfer_msg)

int peci_cmd_xfer(struct peci_xfer_msg *msg);
int peci_cmd_xfer_fd(int fd, struct peci_xfer_msg *msg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __PECI_H__ */
