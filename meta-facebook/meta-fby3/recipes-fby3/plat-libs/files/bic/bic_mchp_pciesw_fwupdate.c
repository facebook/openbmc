/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "bic_mchp_pciesw_fwupdate.h"
#include <openbmc/kv.h>

static int
_get_pcie_sw_update_status(uint8_t slot_id, uint8_t *status) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, 0x01};  // IANA ID + data
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;
  int retries = 3;

  do {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_PCIE_SWITCH_STATUS, tbuf, 4, rbuf, &rlen, REXP_BIC_INTF);
    if ( ret != BIC_STATUS_SUCCESS ) {
      sleep(1);
      syslog(LOG_ERR,"_get_pcie_sw_update_status: slot: %d, retrying..\n", slot_id);
    } else break;
  } while ( ret < 0 && retries-- );

  //Ignore IANA ID
  memcpy(status, &rbuf[3], 2);

  return ret;
}

static int
_poll_pcie_sw_update_status(uint8_t slot_id) {
#define PCIE_SW_MAX_RETRY 50

  enum {
    FW_PCIE_SWITCH_STAT_IDLE = 0,
    FW_PCIE_SWITCH_STAT_INPROGRESS,
    FW_PCIE_SWITCH_STAT_DONE,
    FW_PCIE_SWITCH_STAT_ERROR = 0xFF,
  };

  enum {
    FW_PCIE_SWITCH_DLSTAT_READY = 0,
    FW_PCIE_SWITCH_DLSTAT_INPROGRESS,
    FW_PCIE_SWITCH_DLSTAT_HEADER_INCORRECT,
    FW_PCIE_SWITCH_DLSTAT_OFFSET_INCORRECT,
    FW_PCIE_SWITCH_DLSTAT_CRC_INCORRECT,
    FW_PCIE_SWITCH_DLSTAT_LENGTH_INCORRECT,
    FW_PCIE_SWITCH_DLSTAT_HARDWARE_ERR,
    FW_PCIE_SWITCH_DLSTAT_COMPLETES,
    FW_PCIE_SWITCH_DLSTAT_SUCESS_FIRM_ACT,
    FW_PCIE_SWITCH_DLSTAT_SUCESS_DATA_ACT,
    FW_PCIE_SWITCH_DLSTAT_DOWNLOAD_TIMEOUT = 0xE,
  };

  uint8_t status[2] = {0};
  int j = 0;
  int ret = BIC_STATUS_FAILURE;

  for ( j = 0; j < PCIE_SW_MAX_RETRY; j++ ) {
    // get status
    ret = _get_pcie_sw_update_status(slot_id, status);
    if ( ret < 0 ) {
      break;
    }

    // check status
    if ( status[0] == FW_PCIE_SWITCH_DLSTAT_INPROGRESS || status[0] == FW_PCIE_SWITCH_DLSTAT_READY ) {

      if ( status[1] == FW_PCIE_SWITCH_STAT_INPROGRESS || status[1] == FW_PCIE_SWITCH_STAT_IDLE ) {} // wait for it ready
      else if ( status[1] == FW_PCIE_SWITCH_STAT_DONE ) { break; }  // it's ready
      else { ret = BIC_STATUS_FAILURE; break; } // it's error status, set ret to BIC_STATUS_FAILURE

    } else if ( status[0] == FW_PCIE_SWITCH_DLSTAT_COMPLETES || status[0] == FW_PCIE_SWITCH_DLSTAT_SUCESS_FIRM_ACT || \
                status[0] == FW_PCIE_SWITCH_DLSTAT_SUCESS_DATA_ACT ) {

      if ( status[1] == FW_PCIE_SWITCH_STAT_INPROGRESS ) {} // wait for it ready
      else if ( status[1] == FW_PCIE_SWITCH_STAT_IDLE || status[1] == FW_PCIE_SWITCH_STAT_DONE ) { break; } // it's ready
      else { ret = BIC_STATUS_FAILURE; break; } // it's error status, set ret to BIC_STATUS_FAILURE

    } else { ret = BIC_STATUS_FAILURE; break; } // it's error status, set ret to BIC_STATUS_FAILURE

    // retry with delay
    msleep(50);
  }

  if ( ret < 0 ) {
    printf("Something went wrong. Rsp: sts[0]=0x%02X, sts[1]=0x%02X, ret=%d\n", status[0], status[1], ret);
  } else if ( j == PCIE_SW_MAX_RETRY ) {
    printf("PESW fw update was terminated since retry was done\n");
    ret = BIC_STATUS_FAILURE;
  }

  return ret;
}

#define MAX_FW_PCIE_SWITCH_BLOCK_SIZE 1008
#define PCIE_FW_IDX 0x05
int update_bic_mchp_pcie_fw(uint8_t slot_id, uint8_t comp, char *image, uint8_t intf, uint8_t force) {
#define LAST_FW_PCIE_SWITCH_TRUNK_SIZE (MAX_FW_PCIE_SWITCH_BLOCK_SIZE%224)
  int fd = 0;
  int ret = 0;
  int file_size = 0;

  //open fd and get size
  fd = open_and_get_size(image, &file_size);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d", __func__, image, fd);
    return -1;
  }

  comp = PCIE_FW_IDX;
  printf("file size = %d bytes, slot = %d, comp = 0x%x\n", file_size, slot_id, comp);
  const uint8_t bytes_per_read = 224;
  uint8_t per_read = 0;
  uint8_t buf[256] = {0};
  uint16_t buf_size = sizeof(buf);
  uint16_t read_bytes = 0;
  uint32_t offset = 0;
  uint32_t block_offset = 0;
  uint32_t last_offset = 0;
  uint32_t dsize = 0;
  int last_data_pcie_sw_flag = 0; // We already sent 1008 byte
  dsize = file_size / 20;

  if ( lseek(fd, 0, SEEK_SET) != 0 ) {
    syslog(LOG_WARNING, "%s() Cannot reinit the fd to the beginning. errstr=%s", __func__, strerror(errno));
    goto error_exit;
  }

  int i = 1;
  uint8_t temp_comp = 0;
  while (1) {
    memset(buf, 0, buf_size);

    // 1008 / 224 = 4,
    if (i%5 == 0) {
      per_read = ((offset + LAST_FW_PCIE_SWITCH_TRUNK_SIZE) <= file_size)?LAST_FW_PCIE_SWITCH_TRUNK_SIZE:(file_size - offset);
      last_data_pcie_sw_flag = 1;
    } else {
      last_data_pcie_sw_flag = ((offset + bytes_per_read) < file_size) ? 0 : 1;
      if ( last_data_pcie_sw_flag == 1 ) {
        per_read = file_size - offset; //we already sent all data
      } else {
        per_read = bytes_per_read;
      }
    }
    i++;
    read_bytes = read(fd, buf, per_read);
    if ( read_bytes <= 0 ) {
      //no more bytes can be read
      break;
    }

    if ( last_data_pcie_sw_flag == 1) {
      temp_comp = (comp | 0x80); //0x80 doesn't mean the last data in the image instead of meaning data block
    } else {
      temp_comp = (comp);
    }

    ret = send_image_data_via_bic(slot_id, temp_comp, REXP_BIC_INTF, block_offset, read_bytes, file_size, buf);
    if (ret != BIC_STATUS_SUCCESS)
      break;

    offset += read_bytes;
    if ((last_offset + dsize) <= offset) {
      printf("updated 2OU PESW: %d %%\n", (offset/dsize)*5);
      fflush(stdout);
      _set_fw_update_ongoing(slot_id, 1800);
      last_offset += dsize;
    }

    // wait for PCIe
    if ( (temp_comp & 0x80) == 0x80 ) {
      block_offset = offset; //update block offset
      if ( (ret = _poll_pcie_sw_update_status(slot_id)) < 0 ) break;
      if ( offset == file_size ) break;
    }
  }

error_exit:
  if ( fd > 0 ) close(fd);
  return ret;
}

/*******************************************************************************************************/
#include "bic_bios_fwupdate.h"

static int
_send_bic_usb_packet(usb_dev* udev, bic_usb_ext_packet *pkt) {
  const int transferlen = pkt->length + USB_PKT_EXT_HDR_SIZE;
  int transferred = 0;
  int retries = NUM_ATTEMPTS;
  int ret = 0;

  while(true) {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, (uint8_t*)pkt, transferlen, &transferred, 3000);
    if( (ret != 0) || (transferlen != transferred) ) {
      printf("Error in transferring data! err = %d and transferred = %d(expected data length 64)\n",ret ,transferred);
      printf("Retry since  %s\n", libusb_error_name(ret));
      retries--;
      if (!retries) {
        return BIC_STATUS_FAILURE;
      }
      msleep(100);
    } else break;
  }
  return BIC_STATUS_SUCCESS;
}

int
bic_update_pesw_fw_usb(uint8_t slot_id, uint8_t comp, char *image_file, usb_dev* udev) {
  int ret = BIC_STATUS_FAILURE;
  int fd = 0;
  int file_size = 0;
  uint8_t *buf = NULL;

  fd = open_and_get_size(image_file, &file_size);
  if ( fd < 0 ) {
    printf("%s() cannot open the file: %s, fd=%d", __func__, image_file, fd);
    goto error_exit;
  }

  // allocate memory
  buf = malloc(USB_PKT_EXT_HDR_SIZE + MAX_FW_PCIE_SWITCH_BLOCK_SIZE);
  if ( buf == NULL ) {
    printf("%s() failed to allocate memory\n", __func__);
    goto error_exit;
  }

  uint8_t *file_buf = buf + USB_PKT_EXT_HDR_SIZE;
  size_t write_offset = 0;
  size_t last_offset = 0;
  size_t dsize = file_size / 20;

  ssize_t read_bytes = 0;
  while (1) {
    // read MAX_FW_PCIE_SWITCH_BLOCK_SIZE
    read_bytes = read(fd, file_buf, MAX_FW_PCIE_SWITCH_BLOCK_SIZE);
    if ( read_bytes < 0 ) {
      printf("%s() read error: %d\n", __func__, errno);
      break;
    }

    // pad to 1008 if needed
    for (size_t i = read_bytes; i < MAX_FW_PCIE_SWITCH_BLOCK_SIZE; i++) {
      file_buf[i] = '\xff';
    }

    // create the packet
    bic_usb_ext_packet *pkt = (bic_usb_ext_packet *) buf;
    pkt->dummy = PCIE_FW_IDX;    // component
    pkt->offset = write_offset;  // img offset
    pkt->length = read_bytes;    // read bytes
    pkt->image_size = file_size; // imag size

    int rc = _send_bic_usb_packet(udev, pkt); // send data
    if ( rc < 0 ) {
      printf("%s() failed to write %d bytes @ %d: %d\n", __func__, read_bytes, write_offset, rc);
      break;
    }

    if ( (ret = _poll_pcie_sw_update_status(slot_id)) < 0 ) break;

    // update offset
    write_offset += read_bytes;
    if ((last_offset + dsize) <= write_offset ) {
      printf("updated 2OU PESW: %d %%\n", (write_offset/dsize)*5);
      fflush(stdout);
      _set_fw_update_ongoing(slot_id, 1800);
      last_offset += dsize;
    }

    if ( write_offset == file_size ) break;
  }

error_exit:
  if ( fd > 0 ) close(fd);
  if ( buf != NULL ) free(buf);
  return ret;
}

int update_bic_mchp_pcie_fw_usb(uint8_t slot_id, uint8_t comp, char *image, uint8_t intf, uint8_t force) {
  struct timeval start, end;
  int ret = BIC_STATUS_SUCCESS;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  udev->ci = 1;
  udev->epaddr = 0x1;

  // init usb device
  if ( (ret = bic_init_usb_dev(slot_id, udev, EXP2_TI_PRODUCT_ID, EXP2_TI_VENDOR_ID)) < 0 ) goto error_exit;

  printf("Input: %s, USB timeout: 3000ms\n", image);
  gettimeofday(&start, NULL);

  // sendfile
  if ( (ret = bic_update_pesw_fw_usb(slot_id, comp, image, udev)) < 0 ) goto error_exit;

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  ret = BIC_STATUS_SUCCESS;

error_exit:
  // close
  bic_close_usb_dev(udev);
  return ret;
}
