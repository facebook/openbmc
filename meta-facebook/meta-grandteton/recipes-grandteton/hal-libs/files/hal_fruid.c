/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include <openbmc/ipmi.h>
#include "hal_fruid.h"

#define SWB_BIC_EID            (0x0A)
#define SWB_BUS_ID             (3)
#define FRUID_READ_COUNT_MAX   (0x20)
#define FRUID_WRITE_COUNT_MAX  (0x20)
#define MAX_TXBUF_SIZE         (255)
#define MAX_RXBUF_SIZE         (255)

static int
get_pldm_fruid_info(uint8_t fru_id, ipmi_fruid_info_t *info) {
  int rc;
  size_t rxlen = 0;

  rc = pldm_norm_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                               NETFN_STORAGE_REQ, CMD_STORAGE_GET_FRUID_INFO,
                               &fru_id, 1,
                               (uint8_t*)info, &rxlen);

  return rc;
}

static int
_read_pldm_fruid(uint8_t fru_id, uint16_t offset, uint8_t count, uint8_t *rbuf, size_t *rlen) {
  int rc;
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t txlen = 0;

  txbuf[txlen++] = fru_id;
  txbuf[txlen++] = offset & 0xFF;
  txbuf[txlen++] = (offset >> 8) & 0xFF;
  txbuf[txlen++] = count;


  rc = pldm_norm_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                               NETFN_STORAGE_REQ, CMD_STORAGE_READ_FRUID_DATA,
                               txbuf, txlen,
                               rbuf, rlen);
  return rc;
}

static int
_write_pldm_fruid(uint8_t fru_id, uint16_t offset, uint8_t count, uint8_t *buf) {
  int rc;
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rxbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t txlen = 0;
  size_t  rxlen = 0;

  txbuf[txlen++] = fru_id;
  txbuf[txlen++] = offset & 0xFF;
  txbuf[txlen++] = (offset >> 8) & 0xFF;
  memcpy(txbuf + txlen, buf, count);
  txlen = count + txlen ;

  rc = pldm_norm_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                               NETFN_STORAGE_REQ, CMD_STORAGE_WRITE_FRUID_DATA,
                               txbuf, txlen,
                               rxbuf, &rxlen);
  if (rxbuf[0] != count)
    rc = -1;

  return rc;
}

int hal_read_pldm_fruid(uint8_t fru_id, const char *path, int fru_size) {
  int ret=-1;
  int fd;
  size_t nread, offset;
  size_t count;
  uint8_t rbuf[MAX_RXBUF_SIZE] = {0};
  size_t rlen = 0;
  ipmi_fruid_info_t info;

  // Remove the file if exists already
  unlink(path);

  // Open the file exclusively for write
  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open fails for path: %s", __func__, path);
    goto exit;
  }

  do {
    // Read the FRUID information
    ret = get_pldm_fruid_info(fru_id, &info);
    if (ret) {
      syslog(LOG_ERR, "%s: get_pldm_fruid_info returns %d", __func__, ret);
      break;
    }

    // Indicates the size of the FRUID
    nread = (info.size_msb << 8) | info.size_lsb;
    if (nread > fru_size) {
      nread = fru_size;
    }

    // Read chunks of FRUID binary data in a loop
    offset = 0;
    while (nread > 0) {
      count = (nread > FRUID_READ_COUNT_MAX) ? FRUID_READ_COUNT_MAX : nread;
      rlen = sizeof(rbuf);
      ret = _read_pldm_fruid(fru_id, offset, count, rbuf, &rlen);
      if (ret || (rlen < 1)) {
        break;
      }

      // Ignore the first byte as it indicates length of response
      rlen -= 1;
      if (write(fd, &rbuf[1], rlen) != rlen) {
        ret = -1;
        break;
      }

      // Update offset
      offset += rlen;
      nread -= rlen;
    }
    if (nread > 0) {
      ret = -1;
    }
  } while (0);

exit:
  if ( fd > 0 )
    close(fd);
  return ret;
}

int
hal_write_pldm_fruid(uint8_t fru_id, const char *path) {
  int fd, ret = -1, count;
  uint16_t offset;
  uint8_t buf[MAX_RXBUF_SIZE] = {0};

  fd = open(path, O_RDONLY, 0666);
  if (fd < 0) {
    syslog(LOG_ERR, "%s: open fails for path: %s", __func__, path);
    goto exit;
  }

  // Write chunks of FRUID binary data in a loop
  offset = 0;
  while (1) {
    // Read from file
    count = read(fd, buf, FRUID_WRITE_COUNT_MAX);
    if (count <= 0) {
      break;
    }

    // Write to the FRUID
    ret = _write_pldm_fruid(fru_id, offset, count, buf);
    if (ret) {
      break;
    }

    // Update counter
    offset += count;
  }

exit:
  if ( fd > 0 )
    close(fd);

  return ret;
}
