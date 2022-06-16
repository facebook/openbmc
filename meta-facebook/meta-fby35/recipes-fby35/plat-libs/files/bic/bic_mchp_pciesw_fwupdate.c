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
#include <syslog.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include "bic_mchp_pciesw_fwupdate.h"
#include "bic_power.h"
#include "bic_ipmi.h"
#include "bic_xfer.h"

enum {
  PESW_BOOTLOADER_RCVRY = 0x00,
  PESW_PARTMAP          = 0x01,
  PESW_BOOTLOADER2      = 0x02,
  PESW_CFG              = 0x03,
  PESW_MAIN             = 0x04,
  PESW_IMG_CNT          = 0x05,
  PESW_INIT_BL          = 0x06,
  PESW_TYPE_OFFSET      = 0x1C,
};

char pesw_image_path[5][PATH_MAX+1] = {{"\0"}, {"\0"}, {"\0"}, {"\0"}, {"\0"}};
static int _update_mchp(uint8_t slot_id, uint8_t type, uint8_t intf, bool is_usb, bool is_rcvry);

static int
_check_image(char *image, uint8_t *type) {
  int ret = BIC_STATUS_FAILURE;
  int file_size = 0;
  int fd = open_and_get_size(image, &file_size);

  // check file size
  if ( file_size < 1024 ) {
    printf("%s() the file size is abnormal. img: %s, szie:%d\n", __func__, image, file_size);
    goto error_exit;
  }

  // read its tyoe
  lseek(fd, PESW_TYPE_OFFSET, SEEK_SET);
  uint8_t temp = 0;
  if ( read(fd, &temp, 1) != 1 ) {
    printf("%s() Couldn't read its type!\n", __func__);
    goto error_exit;
  }

  // set flag
  switch(temp) {
    case (PESW_PARTMAP-1):
      printf("WARN: When the partmap image is applied, all images in PCIe switch will be invalidated!!\n");
      temp = PESW_PARTMAP;
    case PESW_BOOTLOADER2:
      if ( strstr(image, "rcvry") != NULL ) temp = PESW_BOOTLOADER_RCVRY;
    case PESW_CFG:
    case PESW_MAIN:
      *type |= 0x1 << temp;
      break;
    default:
      goto error_exit;
  }

  // store path
  strcpy(pesw_image_path[temp], image);
  ret = BIC_STATUS_SUCCESS;
error_exit:
  if ( fd > 0 ) close(fd);

  return ret;
}

static bool
_is_dir(char *image) {
  struct stat st;
  return stat(image, &st) == 0 && S_ISDIR(st.st_mode);
}

static int
_check_dirs(char *image, uint8_t *type) {
  int ret = BIC_STATUS_FAILURE;
  bool is_valid_folder = false;
  // open and check it
  DIR *dir = opendir(image);
  if ( dir == NULL ) {
    printf("Failed to open current directory\n");
    goto error_exit;
  }

  // go through the any entry except for '.' or '..'
  struct dirent *entry;
  while ( (entry = readdir(dir)) != NULL ) {
    if ( entry->d_name[0] == '.' ) continue;

    // create the abs path
    char temp_path[PATH_MAX+1] = "\0";
    char abs_path[PATH_MAX+1] = "\0";
    strcat(temp_path, image);
    strcat(temp_path, "/");
    strcat(temp_path, entry->d_name);

    // if the pointer points to NULL, something went wrong
    char *is_file_good = realpath(temp_path, abs_path);
    if ( is_file_good == NULL ) {
      printf("Err: No such file - %s\n", abs_path);
      goto error_exit;
    }

    if ( _check_image(abs_path, type) == BIC_STATUS_SUCCESS ) {
      printf("Find: %s\n", entry->d_name);
      is_valid_folder = true;
    }
  }

  ret = (is_valid_folder == true)?BIC_STATUS_SUCCESS:BIC_STATUS_FAILURE;
error_exit:
  if ( dir != NULL ) closedir(dir);
  return ret;
}

static int
_is_valid_files(char *image, uint8_t *type) {
  int ret = BIC_STATUS_FAILURE;

  // set type according to files
  if ( _is_dir(image) == true ) {
    if ( _check_dirs(image, type) < 0 ) goto error_exit;
  } else {
    if ( _check_image(image, type) < 0 ) goto error_exit;
  }

  ret = BIC_STATUS_SUCCESS;
error_exit:
  return ret;
}

static int
_switch_pesw_to_recovery(uint8_t slot_id, uint8_t intf, bool is_low) {
  int ret = BIC_STATUS_FAILURE;
  uint8_t tbuf[6] = {0x00};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = IANA_ID_SIZE;
  uint8_t rlen = 0;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
  tbuf[tlen++] = 1;
  tbuf[tlen++] = 19;
  tbuf[tlen++] = (is_low == true)?0:1;

  printf("Pulling %s FM_BIC_PESW_RECOVERY_0...\n", (is_low == true)?"donw":"high");
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    printf("Failed to pull %s FM_BIC_PESW_RECOVERY_0\n", (is_low == true)?"donw":"high");
  }
  return ret;
}

static int
_switch_i2c_mux_to_pesw(uint8_t slot_id, uint8_t intf) {
  uint8_t tbuf[5] = {0x13, 0xE2, 0x00, 0x00, 0x02};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  return bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
}

static int
_check_pesw_status(uint8_t slot_id, uint8_t intf, uint8_t sel_sts, bool run_rcvry, char *expected_sts) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t tbuf[4] = {0x0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);

  switch(sel_sts) {
    case PESW_BOOTLOADER_RCVRY:
      tbuf[3] = (run_rcvry == false)?0x4:0x5;
      break;
    case PESW_PARTMAP:
      tbuf[3] = 0x12;
      break;
    case PESW_BOOTLOADER2:
      tbuf[3] = 0x11;
      break;
    case PESW_CFG:
      tbuf[3] = 0x15;
      break;
    case PESW_MAIN:
      tbuf[3] = 0x17;
      break;
    case PESW_INIT_BL:
      tbuf[3] = 0x00;
      break;
  }

  printf("Send: ");
  for (int i = 0; i < tlen; i++) {
    printf("%02X ", tbuf[i]);
  }
  printf("\n");

  sleep(1);
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, 0x38, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    printf("%s() Failed to check status: %02X\n", __func__, sel_sts);
    goto error_exit;
  }

  printf("Recv: ");
  for (int i = 0; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
  }
  printf("\n");

#if 0
  switch(sel_sts) {
    case PESW_BOOTLOADER_RCVRY:
      break;
    case PESW_PARTMAP:
      break;
    case PESW_BOOTLOADER2:
      break;
    case PESW_CFG:
      break;
    case PESW_MAIN:
      break;
  }
#endif
error_exit:
  return ret;
}

static int
_toggle_pesw(uint8_t slot_id, uint8_t intf, uint8_t type, bool is_rcvry) {
  uint8_t tbuf[7] = {0x00};
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0;
  uint8_t tlen = IANA_ID_SIZE;

  // Fill the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
  tbuf[tlen++] = (is_rcvry == true)?0x06:0x03;
  tbuf[tlen++] = 0x00;
  tbuf[tlen++] = 0x00;
  tbuf[tlen++] = 0x00;

  if ( (type >> PESW_MAIN) & 0x1 ) tbuf[4] = 0x1;
  if ( (type >> PESW_CFG)  & 0x1 ) tbuf[5] = 0x1;
  if ( (type >> PESW_BOOTLOADER2) & 0x1 ) tbuf[6] = 0x1;

  printf("Send the toggle command ");
  for (int i = 0; i < 7; i++) printf("%02X ", tbuf[i]);
  printf("\n");
  return bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, 0x38, tbuf, tlen, rbuf, &rlen, intf);
}

static int
_get_pcie_sw_update_status(uint8_t slot_id, uint8_t *status) {
  uint8_t tbuf[4] = {0x00};  // IANA ID + data
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;
  int retries = 3;

  // Fill the IANA ID + data
  memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
  tbuf[IANA_ID_SIZE] = 0x01;

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
  int ret = BIC_STATUS_FAILURE;
  int file_size = 0;

  //open fd and get size
  fd = open_and_get_size(image, &file_size);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s() cannot open the file: %s, fd=%d", __func__, image, fd);
    goto error_exit;
  }

  comp = PCIE_FW_IDX;
  printf("file size = %d bytes, slot = %d, comp = 0x%x\n", file_size, slot_id, comp);
  const uint8_t bytes_per_read = 224;
  uint8_t per_read = 0;
  uint8_t buf[256] = {0};
  uint16_t buf_size = sizeof(buf);
  uint32_t offset = 0;
  uint32_t block_offset = 0;
  uint32_t last_offset = 0;
  uint32_t dsize = 0;
  ssize_t read_bytes = 0;
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
      printf("Error in transferring data! err = %d and transferred = %d(expected data length %d)\n",ret ,transferred, transferlen);
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
bic_update_pesw_fw_usb(uint8_t slot_id, char *image_file, usb_dev* udev, char *comp_name) {
  int ret = BIC_STATUS_FAILURE;
  int fd = -1;
  int file_size = 0;
  uint8_t *buf = NULL;

  fd = open_and_get_size(image_file, &file_size);
  if ( fd < 0 ) {
    printf("%s() cannot open the file: %s, fd=%d", __func__, image_file, fd);
    return ret;
  }

  // allocate memory
  buf = malloc(USB_PKT_EXT_HDR_SIZE + MAX_FW_PCIE_SWITCH_BLOCK_SIZE);
  if ( buf == NULL ) {
    printf("%s() failed to allocate memory\n", __func__);
    close(fd);
    return ret;
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
      printf("%s() failed to write %zd bytes @ %zu: %d\n", __func__, read_bytes, write_offset, rc);
      break;
    }

    if ( (ret = _poll_pcie_sw_update_status(slot_id)) < 0 ) break;

    // update offset
    write_offset += read_bytes;
    if ((last_offset + dsize) <= write_offset ) {
      printf("updated 2OU PESW %s: %d %%\n", comp_name, (write_offset/dsize)*5);
      fflush(stdout);
      _set_fw_update_ongoing(slot_id, 1800);
      last_offset += dsize;
    }

    if ( write_offset == file_size ) break;
  }

  close(fd);
  free(buf);

  return ret;
}

static int
_quit_pesw_update(uint8_t slot_id, uint8_t type, uint8_t intf, bool is_rcvry) {
  int ret = _toggle_pesw(slot_id, intf, type, is_rcvry);
  if ( is_rcvry == false ) return ret;

  printf("Checking the new image status...\n");

  // check status
  for (int i = PESW_BOOTLOADER2; i < PESW_IMG_CNT; i++ ) {
    ret = _check_pesw_status(slot_id, intf, i, false, NULL);
    if ( ret < 0 ) {
      printf("%s() Failed to check status: %02X\n", __func__, i);
      goto error_exit;
    }
  }

  // check BL
  printf("Initializing the bootloader...\n");
  ret = _check_pesw_status(slot_id, intf, PESW_INIT_BL, false, NULL);
  if ( ret < 0 ) {
    printf("%s() Failed to initialize bootloader\n", __func__);
  }

  // recover it
  ret = _switch_pesw_to_recovery(slot_id, intf, false);
  if ( ret < 0 ) {
    goto error_exit;
  }

  printf("Doing power cycle to trigger the new images...\n");
  ret = bic_server_power_cycle(slot_id);
  if ( ret < 0 ) {
    printf("Failed to do power cycle\n");
    goto error_exit;
  }

  // update bl2
  printf("Updating the bootloader...\n");
  ret = _update_mchp(slot_id, (0x1 << PESW_BOOTLOADER2), intf, true, false);
  if ( ret < 0 ) {
    printf("Failed to update it\n");
    goto error_exit;
  }

  printf("Doing power cycle to trigger the bootloader...\n");
  ret = bic_server_power_cycle(slot_id);
  if ( ret < 0 ) {
    printf("Failed to do power cycle\n");
    goto error_exit;
  }
error_exit:
  return ret;
}

static int
_enter_pesw_rcvry_mode(uint8_t slot_id, uint8_t intf, bool is_rcvry) {
  int ret = BIC_STATUS_FAILURE;
  uint8_t tbuf[6] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;

  if ( is_rcvry == false ) return BIC_STATUS_SUCCESS;

  printf("Starting running PESW recovery\n");

  ret = _switch_pesw_to_recovery(slot_id, intf, true);
  if ( ret < 0 ) {
    goto error_exit;
  }

  printf("Doing power cycle to trigger TWI recovery mode...\n");
  ret = bic_server_power_cycle(slot_id);
  if ( ret < 0 ) {
    printf("Failed to do power cycle\n");
    goto error_exit;
  }

  printf("Waiting for BIC...\n");
  sleep(6);

  printf("Stopping monitoring SSD...\n");
  ret = bic_enable_ssd_sensor_monitor(slot_id, false, intf);
  if ( ret < 0 ) {
    printf("Failed to stop monitoring SSD\n");
    goto error_exit;
  }

  printf("Switching I2C mux to PESW...\n");
  ret = _switch_i2c_mux_to_pesw(slot_id, intf);
  if ( ret < 0 ) {
    printf("Failed to switch i2c mux\n");
    goto error_exit;
  }

  printf("Checking PESW status...\n");
  memcpy(tbuf, (uint8_t *)&IANA_ID, IANA_ID_SIZE);
  tbuf[3] = 0x09;
  tlen = 4;
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, 0x60, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    printf("Failed to get the device list\n");
    goto error_exit;
  }

  // if B0h is not appeared, we can't proceed to do the rest.
  ret = BIC_STATUS_FAILURE;
  for (int i = 3; i < rlen; i++ ) {
    if ( rbuf[i] == 0xb0 ) {
      ret = BIC_STATUS_SUCCESS;
      break;
    }
  }

  // check bootloader phase
  ret = (ret < 0)?BIC_STATUS_FAILURE: _check_pesw_status(slot_id, intf, PESW_BOOTLOADER_RCVRY, false, NULL);
  printf("PESW is %srunning in recovery mode!\n", (ret < 0)?"not ":"");

error_exit:
  return ret;
}

static int
_process_mchp_images(uint8_t slot_id, uint8_t idx, uint8_t intf, usb_dev* udev, bool is_usb, bool is_rcvry) {
  int ret = BIC_STATUS_FAILURE;
  char comp_name[5][15] = {{"RCVRY"}, {"PARTMAP"}, {"BOOTLOADER2"}, {"CFG"}, {"MAIN FW"}};

  printf("******Start sending %s******\n", comp_name[idx]);
  printf("File: %s\n", pesw_image_path[idx]);

  if ( is_usb == false ) {
    //do nothing
    printf("Please update it via USB!\n");
  } else {
    ret = bic_update_pesw_fw_usb(slot_id, pesw_image_path[idx], udev, comp_name[idx]);
    if ( ret < 0 ) {
      printf("Failed to update %s\n", comp_name[idx]);
      goto error_exit;
    }
  }

  // If it's not running for recovery mode, we can return
  if ( is_rcvry == false ) return ret;

  // check the status after applying the image
  if ( idx == PESW_BOOTLOADER_RCVRY ) {
    printf("Activating %s...\n", comp_name[idx]);
    ret = _check_pesw_status(slot_id, intf, idx, true, NULL); // execute bl2
    if ( ret < 0 ) {
      printf("Failed to activate it\n");
      goto error_exit;
    }
  }

  ret = _check_pesw_status(slot_id, intf, idx, false, NULL); // check status
  if ( ret < 0 ) {
    printf("Failed to get the phase status\n");
    goto error_exit;
  }

error_exit:
  return ret;
}

static int
_update_mchp(uint8_t slot_id, uint8_t type, uint8_t intf, bool is_usb, bool is_rcvry) {
  struct timeval start, end;
  int ret = BIC_STATUS_FAILURE;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  // USB is the default path for PESW update, we just make sure USB can be initialized,
  // If users choose to send data via IPMB, skip it
  if ( is_usb == true ) {
    udev->ci = 1;
    udev->epaddr = 0x1;
    if ( (ret = bic_init_usb_dev(slot_id, udev, EXP2_TI_PRODUCT_ID, EXP2_TI_VENDOR_ID)) < 0 )
      goto error_exit;
  }

  gettimeofday(&start, NULL);

  // should it run into rcvry?
  if ( _enter_pesw_rcvry_mode(slot_id, intf, is_rcvry) < 0 ) {
    goto error_exit;
  }

  // try to send the images
  for (int i = PESW_BOOTLOADER_RCVRY; i < PESW_IMG_CNT; i++) {
    if ( ((type >> i) & 0x1) == 0 ) continue;

    ret = _process_mchp_images(slot_id, i, intf, udev, is_usb, is_rcvry);
    if ( ret < 0 ) {
      goto error_exit;
    }
  }

  // quit
  if ( _quit_pesw_update(slot_id, type, intf, is_rcvry) < 0 ) {
    goto error_exit;
  }

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
error_exit:
  if ( is_usb == true ) {
    bic_close_usb_dev(udev);
  }

  return ret;
}

int update_bic_mchp(uint8_t slot_id, uint8_t comp, char *image, uint8_t intf, uint8_t force, bool is_usb) {
  int ret = BIC_STATUS_FAILURE;
  uint8_t type = 0;

  // check files
  ret = _is_valid_files(image, &type);
  if ( ret < 0 ) {
    printf("Invalid inputs: %s\n", image);
    goto error_exit;
  }

  // is it going to run rcvry?
  bool is_rcvry = (type >> PESW_BOOTLOADER_RCVRY) & 0x1;

  // start updating PESW
  ret = _update_mchp(slot_id, type, intf, is_usb, is_rcvry);
error_exit:
  return ret;
}
