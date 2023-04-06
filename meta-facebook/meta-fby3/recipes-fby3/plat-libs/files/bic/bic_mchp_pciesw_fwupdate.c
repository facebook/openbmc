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
#include <dirent.h>
#include <limits.h>
#include "bic_mchp_pciesw_fwupdate.h"
#include <openbmc/kv.h>

enum {
  // update flow
  PESW_BOOTLOADER_RCVRY = 0x00,
  PESW_PARTMAP          = 0x01,
  PESW_KEYM             = 0x02,
  PESW_BOOTLOADER2      = 0x03,
  PESW_CFG              = 0x04,
  PESW_MAIN             = 0x05,
  PESW_IMG_CNT          = 0x06,
  PESW_BLOB             = 0x06,
  PESW_INIT_BL          = 0x07,

  // image signed offset 0x274
  PESW_SIGNED_OFFSET    = 0x274,

  // image type(offset 1Ch)
  PESW_TYPE_OFFSET      = 0x1C,
  PESW_TYPE_PARTMAP     = 0x00,
  PESW_TYPE_KEYM        = 0x01,
  PESW_TYPE_BOOTLOADER2 = 0x02,
  PESW_TYPE_CFG         = 0x03,
  PESW_TYPE_PSX         = 0x04,

  // image start offset in blob image
  PESW_BLOB_BL2_LOCATION_OFFSET = 0x1074,
  PESW_BLOB_PSX_LOCATION_OFFSET = 0x10c4,
  PESW_BLOB_CFG_LOCATION_OFFSET = 0x109c,

  // pesw blob end offset
  PESW_BLOB_END         = 0x00800000,

  // Mask
  PESW_MASK_RCVRY       = 0x3B, // recovery
  PESW_MASK_S_RCVRY     = 0x3F, // secure recovery
  PESW_MASK_N_UPD       = 0x38, // normal update
  PESW_MASK_N_NOT_SUP   = 0x07, // normal update, bit0~2 are not supported

  // status
  PESW_PHASE1           = 0x01,
  PESW_PHASE2           = 0x02,
  PESW_ACT_STS          = 0x01,
  PESW_VALID_STS        = 0x01,

  // PCIe state
  PESW_DEFAULT_STATE    = 0x00,
  PESW_INIT_UNSECURED   = 0x01,
  PESW_INIT_SECURED     = 0x02,

  // pesw header and data length offset
  PESW_HEADER_LEN_OFFSET= 0x10,
  PESW_DATA_LEN_OFFSET  = 0x18,

};

static int cwc_usb_depth = 3;
static uint8_t cwc_usb_ports[] = {1, 3, 3};
static int top_gpv3_usb_depth = 5;
static int bot_gpv3_usb_depth = 5;
static uint8_t top_gpv3_usb_ports[] = {1, 3, 1, 2, 3};
static uint8_t bot_gpv3_usb_ports[] = {1, 3, 4, 2, 3};

// PESW_IMG_CNT + 1 is for the path of blob image
char pesw_image_path[PESW_IMG_CNT+1][PATH_MAX+1] = {{"\0"}, {"\0"}, {"\0"}, {"\0"}, {"\0"}};

static int
_process_mchp_images(uint8_t slot_id, uint8_t idx, uint8_t intf, usb_dev* udev, bool is_usb, bool is_rcvry, bool is_blob_image, int *image_location_blob);

// there are cfg, psx, and bl2 image in one blob file, need to calculate their own image size
static int
_calculate_image_size_blob(int fd, int header_len_offset, int data_len_offset, int *image_size) {

  uint32_t header_len = 0;
  uint32_t data_len = 0;

  // needs 4 bytes for calcualting the image size
  uint8_t temp_header_len[4] = {0};
  uint8_t temp_data_len[4]  = {0};

  if ( !image_size ) {
    printf("%s() pointer is NULL!\n",__func__);
    return -1;
  }

  lseek(fd, header_len_offset, SEEK_SET);
  if ( read(fd, temp_header_len, ARRAY_SIZE(temp_header_len)) != ARRAY_SIZE(temp_header_len) ) {
    printf("%s() Couldn't read header length's offset\n", __func__);
    return -1;
  }

  for ( int i = ARRAY_SIZE(temp_header_len)-1; i >= 0; i-- ) {
    header_len |= (temp_header_len[i] << 8*i);
  }

  lseek(fd, data_len_offset, SEEK_SET);
  if ( read(fd, temp_data_len, ARRAY_SIZE(temp_data_len)) != ARRAY_SIZE(temp_data_len) ) {
    printf("%s() Couldn't read image length's offset\n", __func__);
    return -1;
  }

  for ( int i = ARRAY_SIZE(temp_data_len)-1; i >= 0; i-- ) {
    data_len |= (temp_data_len[i] << 8*i);
  }

  *image_size = header_len + data_len;

  return 0;
}

static int
_check_blob_image(char *image, uint8_t *type, uint8_t *file_s_info, bool *is_blob_image, int *image_location) {

  int i = 0;
  int fd = 0;
  int file_size = 0;
  int ret = BIC_STATUS_FAILURE;
  uint8_t temp_offset[4] = {0};
  uint8_t magic_offset[8] = {0};
  uint8_t signed_flag = 0;

  // magic byte from each fw, to determine this blob image is valid or not
  const uint8_t MAGIC_BYTE[8] = {0x4D, 0x53, 0x43, 0x43, 0x5F, 0x4D, 0x44, 0x20};

  if ( !image || !type || !file_s_info || !is_blob_image || !image_location ) {
    printf("%s() pointer is NULL!\n",__func__);
    goto error_exit;
  }

  fd = open_and_get_size(image, &file_size);

  // blob image got only one image size
  if ( file_size != PESW_BLOB_END ) {
    goto error_exit;
  }

  // read its type/signed/magic byte in flash blob image for bl2, cfg, main
  for ( int file = PESW_BOOTLOADER2; file <= PESW_MAIN; file++ ) {

    // Read Image's location first
    switch(file) {
      case PESW_BOOTLOADER2:
        lseek(fd, PESW_BLOB_BL2_LOCATION_OFFSET, SEEK_SET);
        break;
      case PESW_CFG:
        lseek(fd, PESW_BLOB_CFG_LOCATION_OFFSET, SEEK_SET);
        break;
      case PESW_MAIN:
        lseek(fd, PESW_BLOB_PSX_LOCATION_OFFSET, SEEK_SET);
        break;
      default:
        goto error_exit;
    }

    // read and calculate image start offset
    if ( read(fd, temp_offset, ARRAY_SIZE(temp_offset)) != ARRAY_SIZE(temp_offset) ) {
      printf("%s() Couldn't read image location's Img: %s\n", __func__, image);
      goto error_exit;
    }

    for ( i = ARRAY_SIZE(temp_offset)-1; i >= 0; i-- ) {
      image_location[file] |= (temp_offset[i] << 8*i);
    }

    // check magic byte
    lseek(fd, image_location[file], SEEK_SET);
    if ( read(fd, magic_offset, ARRAY_SIZE(magic_offset)) != ARRAY_SIZE(magic_offset) ) {
      printf("%s() Couldn't read magic byte's. Img: %s\n", __func__, image);
      goto error_exit;
    }

    if ( memcmp(magic_offset, MAGIC_BYTE, ARRAY_SIZE(MAGIC_BYTE)) != 0 ) {
      printf("%s() Invalid file! Img: %s\n", __func__, image);
      goto error_exit;
    }

    // check image type
    lseek(fd, image_location[file] + PESW_TYPE_OFFSET, SEEK_SET);
    if ( read(fd, &temp_offset[0], 1) != 1 ) {
      printf("%s() Couldn't read its type! Img: %s\n", __func__, image);
      goto error_exit;
    }

    // make sure the file image is same as it's type
    switch(temp_offset[0]) {
      case PESW_TYPE_BOOTLOADER2:
        if ( file != PESW_BOOTLOADER2 ) {
          goto error_exit;
        }
        break;
      case PESW_TYPE_CFG:
        if ( file != PESW_CFG )  {
          goto error_exit;
        }
        break;
      case PESW_TYPE_PSX:
        if ( file != PESW_MAIN )  {
          goto error_exit;
        }
        break;
      default:
        goto error_exit;
    }

    // check signed offset
    lseek(fd, image_location[file] + PESW_SIGNED_OFFSET, SEEK_SET);
    if ( read(fd, temp_offset, ARRAY_SIZE(temp_offset)) != ARRAY_SIZE(temp_offset) ) {
      printf("%s() Couldn't read its signed data!\n", __func__);
      goto error_exit;
    }

    // set the flag
    *type |= 0x1 << file;

    // it would not be zero if it's a signed image
    for ( i = 1; i < (int)ARRAY_SIZE(temp_offset); i++ ) {
      temp_offset[0] += temp_offset[i];
    }

    if ( temp_offset[0] > 0 ) {
      signed_flag += 1;
      *file_s_info |= 0x1 << file;
    }
  }

  if ( signed_flag != 0 && signed_flag != 3 ) {
    printf("%s() Mixed signed and unsigned image in blob  img: %s\n", __func__, image);
    goto error_exit;
  }

  // store file path and set its a flash image flag
  strncpy(pesw_image_path[PESW_BLOB], image, PATH_MAX);
  *is_blob_image = true;
  ret = BIC_STATUS_SUCCESS;

error_exit:
  if ( fd > 0 ) {
    close(fd);
  }

  return ret;
}

static int
_check_image(char *image, uint8_t *type, uint8_t *file_s_info) {
  int ret = BIC_STATUS_FAILURE;
  int fd = 0;
  size_t str_len = strlen(image);

  // xxxx.fwimg, str_len should be > 5
  if ( str_len <= 5 || (strcmp(&image[str_len-5], "fwimg") != 0) ) {
    goto error_exit;
  }

  int file_size = 0;
  int index = 0;

  fd = open_and_get_size(image, &file_size);

  // check file size
  if ( file_size < 1024 ) {
    printf("%s() the file size is abnormal. img: %s, szie:%d\n", __func__, image, file_size);
    goto error_exit;
  }

  // read its type
  lseek(fd, PESW_TYPE_OFFSET, SEEK_SET);
  uint8_t temp[4] = {0};
  if ( read(fd, &temp[0], 1) != 1 ) {
    printf("%s() Couldn't read its type!\n", __func__);
    goto error_exit;
  }

  // set the index
  switch(temp[0]) {
    case PESW_TYPE_PARTMAP:     index = PESW_PARTMAP;     break;
    case PESW_TYPE_KEYM:        index = PESW_KEYM;        break;
    case PESW_TYPE_BOOTLOADER2: index = PESW_BOOTLOADER2; break;
    case PESW_TYPE_CFG:         index = PESW_CFG;         break;
    case PESW_TYPE_PSX:         index = PESW_MAIN;        break;
    default: goto error_exit;
  }

  // check if it's the recovery bootloader?
  // we can only check the file name because its type is the same as PESW_TYPE_BOOTLOADER2
  if ( temp[0] == PESW_TYPE_BOOTLOADER2 && strstr(image, "rcvry") != NULL ) {
    index = PESW_BOOTLOADER_RCVRY;
  }

  // set the flag
  *type |= 0x1 << index;

  // store path
  strncpy(pesw_image_path[index], image, PATH_MAX);

  // check if it's singed image
  lseek(fd, PESW_SIGNED_OFFSET, SEEK_SET);
  if ( read(fd, temp, 4) != 4 ) {
    printf("%s() Couldn't read its signed data!\n", __func__);
    goto error_exit;
  }

  // it would not be zero if it's a signed image
  for (int i = 1; i < 4; i++ ) temp[0] += temp[i];

  if ( temp[0] > 0 ) *file_s_info |= 0x1 << index;

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
_check_dirs(char *image, uint8_t *type, uint8_t *file_s_info) {
  int ret = BIC_STATUS_FAILURE;
  uint8_t image_count = 0;

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

    // check the files in the dir
    if ( _check_image(abs_path, type, file_s_info) == BIC_STATUS_SUCCESS ) {
      printf("Find: %s\n", entry->d_name);
    }
    image_count ++;
  }

  if (image_count == 0) {
    printf("It is an empty folder!\n");
    goto error_exit;
  }

  // return the result
  ret = BIC_STATUS_SUCCESS;
error_exit:
  if ( dir != NULL ) closedir(dir);
  return ret;
}

static int
_is_valid_files(char *image, uint8_t *type, uint8_t *file_s_info, bool *is_blob_image, int *image_location_blob) {
  int ret = BIC_STATUS_FAILURE;

  if ( !image || !type || !file_s_info || !is_blob_image || !image_location_blob ) {
    printf("%s() pointer is NULL!\n",__func__);
    goto error_exit;
  }

  for ( int i = 0; i < PESW_IMG_CNT+1; i++ ) {
    pesw_image_path[i][0] = '\n';
  }

  // set type according to files
  if ( _is_dir(image) == true ) {
    if ( _check_dirs(image, type, file_s_info) < 0 ) {
      goto error_exit;
    }
  } else {
    if ( _check_blob_image(image, type, file_s_info, is_blob_image, image_location_blob) < 0 ) {
      if ( _check_image(image, type, file_s_info) < 0 ) {
        goto error_exit;
      }
    }
  }

  ret = BIC_STATUS_SUCCESS;
error_exit:
  return ret;
}

static int
_switch_pesw_to_recovery(uint8_t slot_id, uint8_t intf, bool is_low) {
  int ret = BIC_STATUS_FAILURE;
  uint8_t tbuf[6] = {0x9c, 0x9c, 0x00, 1, 19, (is_low == true)?0:1};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;

  printf("Pulling %s FM_BIC_PESW_RECOVERY_0...\n", (is_low == true)?"down":"high");
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    printf("Failed to pull %s FM_BIC_PESW_RECOVERY_0\n", (is_low == true)?"down":"high");
  }
  return ret;
}

static int
_switch_cwc_pesw_to_recovery(uint8_t slot_id, uint8_t intf, bool is_low) {
  int ret = BIC_STATUS_FAILURE;
  uint8_t tbuf[6] = {0x9c, 0x9c, 0x00, 1, 18, (is_low == true)?0:1};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;

  printf("Pulling %s FM_BIC_PESW_RECOVERY_0...\n", (is_low == true)?"down":"high");
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, tbuf, tlen, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    printf("Failed to pull %s FM_BIC_PESW_RECOVERY_0\n", (is_low == true)?"down":"high");
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
_check_pesw_status(uint8_t slot_id, uint8_t intf, uint8_t sel_sts, bool run_rcvry, uint8_t *expected_sts, uint8_t cmp_len) {
  int ret = BIC_STATUS_SUCCESS;
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x0, };
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

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
    case PESW_KEYM:
      tbuf[3] = 0x19;
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

  // if we are not going to check the status, return
  if ( sel_sts == PESW_BOOTLOADER_RCVRY && (tbuf[3] == 0x5) ) return ret;
  if ( cmp_len == 0 ) return ret;

  // check the status
  ret = (expected_sts == NULL)?BIC_STATUS_FAILURE:memcmp(&rbuf[3], expected_sts, cmp_len);
  if ( ret != BIC_STATUS_SUCCESS ) ret = BIC_STATUS_FAILURE;

error_exit:
  return ret;
}

static int
_toggle_pesw(uint8_t slot_id, uint8_t intf, uint8_t type, bool is_rcvry) {
  uint8_t tbuf[7] = {0x9c, 0x9c, 0x00, (is_rcvry == true)?0x06:0x03, 0x00, 0x00, 0x00};
  uint8_t rbuf[16] = {0};
  uint8_t rlen = 0;

  if ( (type >> PESW_MAIN) & 0x1 ) tbuf[4] = 0x1;
  if ( (type >> PESW_CFG)  & 0x1 ) tbuf[5] = 0x1;
  if ( (type >> PESW_BOOTLOADER2) & 0x1 ) tbuf[6] = 0x1;
  if ( (type >> PESW_KEYM) & 0x1 ) {
    if ( tbuf[6] == 0x1 ) tbuf[6] = 0x3; /*BL2 and KEYM*/
    else tbuf[6] = 0x2; /*only KEYM*/
  }

  printf("Send the toggle command ");
  for (int i = 0; i < 7; i++) printf("%02X ", tbuf[i]);
  printf("\n");
  return bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, 0x38, tbuf, 7, rbuf, &rlen, intf);
}

static int
_get_pcie_sw_config_info(uint8_t slot_id, uint8_t *config, uint8_t intf) {
  uint8_t tbuf[] = {0x9c, 0x9c, 0x00, 0x08};  // IANA ID + data
  uint8_t rbuf[32] = {0};
  uint8_t rlen = 0;
  int ret = 0;

  printf("PESW firmware state: ");
  ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_PCIE_SWITCH_STATUS, tbuf, 4, rbuf, &rlen, intf);
  if ( ret < 0 ) {
    printf("Unrecognized, get: %02X\n", *config);
  } else {
    *config = (rbuf[11] >> 2) & 0x3; //bit7~6
    if ( *config == PESW_DEFAULT_STATE ) printf("Uninitialized/Unsecured\n");
    else if ( *config == PESW_INIT_UNSECURED ) printf("Initialized Unsecure\n");
    else if ( *config == PESW_INIT_SECURED ) printf("Initialized Secure\n");
    else { printf("Unrecognized but ret = 0(Get: %02X), aborted\n", *config); ret = BIC_STATUS_FAILURE; }
  }
  return ret;
}

static int
_get_pcie_sw_update_status(uint8_t slot_id, uint8_t *status, uint8_t intf) {
  uint8_t tbuf[4] = {0x9c, 0x9c, 0x00, 0x01};  // IANA ID + data
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;
  int retries = 3;

  do {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_PCIE_SWITCH_STATUS, tbuf, 4, rbuf, &rlen, intf);
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
_poll_pcie_sw_update_status(uint8_t slot_id, uint8_t intf) {
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
    ret = _get_pcie_sw_update_status(slot_id, status, intf);
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

/*******************************************************************************************************/
#include "bic_bios_fwupdate.h"

static int
_init_usb_intf(uint8_t slot_id, uint8_t board_type, uint8_t intf, usb_dev* udev) {
  uint8_t *sel_ports = NULL;
  int sel_depth = 0;

  udev->ci = 1;
  udev->epaddr = 0x1;

  // enable USB HUB
  if (board_type == CWC_MCHP_BOARD) {
    bic_open_cwc_usb(slot_id);
  } else {
    bic_set_gpio(slot_id, GPIO_RST_USB_HUB, VALUE_HIGH);
  }

  // check board type
  if ( board_type == CWC_MCHP_BOARD ) {
    if ( intf == RREXP_BIC_INTF1 ) {
      sel_depth = top_gpv3_usb_depth;
      sel_ports = top_gpv3_usb_ports;
    } else if (intf == RREXP_BIC_INTF2) {
      sel_depth = bot_gpv3_usb_depth;
      sel_ports = bot_gpv3_usb_ports;
    } else {
      sel_depth = cwc_usb_depth;
      sel_ports = cwc_usb_ports;
    }
  }

  // if board_type is not CWC_MCHP_BOARD, it means the USB dev can be found by default value. e.g, port = NULL, depth = 0.
  return bic_init_usb_dev_ports(slot_id, udev, EXP2_TI_PRODUCT_ID, EXP2_TI_VENDOR_ID, sel_depth, sel_ports);
}

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
bic_update_pesw_fw_usb(uint8_t slot_id, char *image_file, usb_dev* udev, char *comp_name, uint8_t intf, uint8_t comp_idx, bool is_blob_image, int *image_location_blob) {
  int ret = BIC_STATUS_FAILURE;
  int fd = 0;
  int file_size = 0;
  uint8_t *buf = NULL;

  if ( !image_file || !udev || !comp_name || !image_location_blob ) {
    printf("%s() pointer is NULL!\n",__func__);
    goto error_exit;
  }

  fd = open_and_get_size(image_file, &file_size);
  if ( fd < 0 ) {
    printf("%s() cannot open the file: %s, fd=%d", __func__, image_file, fd);
    goto error_exit;
  }

  // calculate image size and set start offset in BLOB image
  if ( is_blob_image ) {
    switch(comp_idx) {
      case PESW_BOOTLOADER2:
      case PESW_CFG:
      case PESW_MAIN:
        ret = _calculate_image_size_blob(fd, image_location_blob[comp_idx]+PESW_HEADER_LEN_OFFSET,\
                                          image_location_blob[comp_idx]+PESW_DATA_LEN_OFFSET, &file_size);
        if ( ret < 0 ) {
          printf("%s() cannot calculate image size in blob file! Img:%s, Comp Id:%d", __func__, image_file, comp_idx);
          goto error_exit;
        }

        lseek(fd, image_location_blob[comp_idx], SEEK_SET);
        break;
      default:
        goto error_exit;
    }
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

    if ( is_blob_image ) {
      if ( file_size - write_offset < MAX_FW_PCIE_SWITCH_BLOCK_SIZE ) {
        read_bytes = read(fd, file_buf, file_size - write_offset);
      } else {
        read_bytes = read(fd, file_buf, MAX_FW_PCIE_SWITCH_BLOCK_SIZE);
      }
    } else {
      // read MAX_FW_PCIE_SWITCH_BLOCK_SIZE
      read_bytes = read(fd, file_buf, MAX_FW_PCIE_SWITCH_BLOCK_SIZE);
    }

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

    if ( (ret = _poll_pcie_sw_update_status(slot_id, intf)) < 0 ) break;

    // update offset
    write_offset += read_bytes;
    if ((last_offset + dsize) <= write_offset ) {
      printf("updated PESW %s: %d %%\n", comp_name, (write_offset/dsize)*5);
      fflush(stdout);
      _set_fw_update_ongoing(slot_id, 1800);
      last_offset += dsize;
    }

    if ( write_offset >= (size_t)file_size ) {
      break;
    }
  }

error_exit:
  if ( fd > 0 ) close(fd);
  if ( buf != NULL ) free(buf);
  return ret;
}

static int
_quit_pesw_update(uint8_t slot_id, uint8_t type, usb_dev* udev, uint8_t intf, bool is_rcvry, uint8_t board_type, int *image_location_blob) {
  int ret = _toggle_pesw(slot_id, intf, (type & PESW_MASK_N_UPD), is_rcvry);

  if ( !udev || !image_location_blob ) {
    printf("%s() pointer is NULL!\n",__func__);
    goto error_exit;
  }

  if ( is_rcvry == false ) {
    return ret;
  }

  printf("Checking the new image status...\n");

  // check status
  uint8_t expected_sts[2] = {PESW_ACT_STS, PESW_VALID_STS};
  for (int i = PESW_BOOTLOADER2; i < PESW_IMG_CNT; i++ ) {
    ret = _check_pesw_status(slot_id, intf, i, false, expected_sts, 2);
    if ( ret < 0 ) {
      printf("%s() Failed to check status: %02X\n", __func__, i);
      goto error_exit;
    }
  }

  // check BL
  printf("Initializing the bootloader...\n");
  ret = _check_pesw_status(slot_id, intf, PESW_INIT_BL, false, NULL, 0);
  if ( ret < 0 ) {
    printf("%s() Failed to initialize bootloader\n", __func__);
  }

  // recover it
  if (board_type == CWC_MCHP_BOARD && intf == REXP_BIC_INTF) {
    ret = _switch_cwc_pesw_to_recovery(slot_id, intf, false);
  } else {
    ret = _switch_pesw_to_recovery(slot_id, intf, false);
  }
  if ( ret < 0 ) {
    goto error_exit;
  } else sleep(8);

  // unlock
  _set_fw_update_ongoing(slot_id, 0);
  sleep(2);

  printf("Doing power cycle to trigger the new images...\n");
  ret = bic_server_power_cycle(slot_id);
  if ( ret < 0 ) {
    printf("Failed to do power cycle\n");
    goto error_exit;
  }

  // wait for the host becomes ready
  char key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  uint8_t slp_cnt = 0;
  const uint8_t slp_cnt_max = 100;
  snprintf(key, MAX_KEY_LEN, "fru%d_host_ready", slot_id);
  printf("Wait for the host ");
  while ( slp_cnt < slp_cnt_max ) {
    ret = kv_get(key, value, NULL, 0);
    if ( ret == BIC_STATUS_SUCCESS && (strcmp(value, "1") == 0) ) {
      printf("READY\n");
      break;
    }

    printf(".");
    fflush(stdout);
    sleep(5);
    slp_cnt++;
  }

  if ( slp_cnt == slp_cnt_max ) {
    printf("TIMEOUT but still try to flash it\n");
  }

  // lock again
  _set_fw_update_ongoing(slot_id, 1800);
  sleep(2);

  ret = _init_usb_intf(slot_id, board_type, intf, udev);
  if ( ret < 0 ) {
    goto error_exit;
  }

  // update KEYM again if needed
  if ( (type >> PESW_KEYM) & 0x1 ) {
    printf("Updating the keym...\n");
    ret = _process_mchp_images(slot_id, PESW_KEYM, intf, udev, true, false, false, image_location_blob);
    if ( ret < 0 ) {
      printf("Failed to update it\n");
      if ( udev->handle != NULL ) bic_close_usb_dev(udev);
      goto error_exit;
    }
    type = (0x1 << PESW_KEYM);
  } else {
    type = 0;
  }

  type |= (0x1 << PESW_BOOTLOADER2);

  // update bl2
  printf("Updating the bootloader...\n");
  // usb cant be claimed twice
  ret = _process_mchp_images(slot_id, PESW_BOOTLOADER2, intf, udev, true, false, false, image_location_blob);
  if ( ret < 0 ) {
    printf("Failed to update it\n");
    if ( udev->handle != NULL ) bic_close_usb_dev(udev);
    goto error_exit;
  }

  // close USB HUB if needed
  if ( udev->handle != NULL ) bic_close_usb_dev(udev);

  // only toggle the BL2 and KEYM(optional)
  printf("Toggle the images...\n");
  ret = _toggle_pesw(slot_id, intf, type, false);
  if ( ret < 0 ) {
    printf("Failed to toggle the images...\n");
    goto error_exit;
  }

  sleep(2);

  printf("Doing power cycle to trigger the bootloader...\n");
  ret = bic_server_power_cycle(slot_id);
  if ( ret < 0 ) {
    printf("Failed to do power cycle\n");
    goto error_exit;
  }

  // need to wait for BIC not to be busy.
  sleep(8);

error_exit:
  return ret;
}

static int
_enter_pesw_rcvry_mode(uint8_t slot_id, uint8_t intf, bool is_rcvry, uint8_t board_type) {
  int ret = BIC_STATUS_FAILURE;
  uint8_t tbuf[6] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 6;
  uint8_t rlen = 0;


  if ( is_rcvry == false ) return BIC_STATUS_SUCCESS;

  printf("Starting running PESW recovery\n");

  if (board_type == CWC_MCHP_BOARD && intf == REXP_BIC_INTF) {
    ret = _switch_cwc_pesw_to_recovery(slot_id, intf, true);
  } else {
    ret = _switch_pesw_to_recovery(slot_id, intf, true);
  }
  if ( ret < 0 ) {
    goto error_exit;
  }

  printf("Doing power cycle to trigger TWI recovery mode...\n");
  ret = bic_server_power_cycle(slot_id);
  if ( ret < 0 ) {
    printf("Failed to do power cycle\n");
    goto error_exit;
  }

  // BIC spends 6s on initilizing M2 devices
  printf("Waiting for BIC...\n");
  sleep(8);

  printf("Stopping monitoring SSD...\n");
  ret = bic_enable_ssd_sensor_monitor(slot_id, false, intf);
  if ( ret < 0 ) {
    printf("Failed to stop monitoring SSD\n");
    goto error_exit;
  }

  // When BMC requests BIC to stop running SSD monitor,
  // it wouldn't be stopped immediately
  printf("Waiting for BIC...\n");
  sleep(8);

  if (board_type == CWC_MCHP_BOARD && intf == REXP_BIC_INTF) {
    printf("Skip switching I2C mux to PESW\n");
  } else {
    printf("Switching I2C mux to PESW...\n");
    ret = _switch_i2c_mux_to_pesw(slot_id, intf);
    if ( ret < 0 ) {
      printf("Failed to switch i2c mux\n");
      goto error_exit;
    }
  }

  printf("Detecting PESW...");
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);
  if (board_type == CWC_MCHP_BOARD && intf == REXP_BIC_INTF) {
    tbuf[3] = 0x01;
  } else {
    tbuf[3] = 0x09;
  }
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
      printf("Found");
      ret = BIC_STATUS_SUCCESS;
      break;
    }
  }
  printf("\n");

  uint8_t expected_sts = PESW_PHASE1;
  // check bootloader phase
  ret = (ret < 0)?BIC_STATUS_FAILURE: _check_pesw_status(slot_id, intf, PESW_BOOTLOADER_RCVRY, false, &expected_sts, 1);
  printf("PESW is %srunning in recovery mode!\n", (ret < 0)?"not ":"");

error_exit:
  return ret;
}

static int
_process_mchp_images(uint8_t slot_id, uint8_t idx, uint8_t intf, usb_dev* udev, bool is_usb, bool is_rcvry, bool is_blob_image, int *image_location_blob) {
  int ret = BIC_STATUS_FAILURE;
  char comp_name[][15] = {{"RCVRY"}, {"PARTMAP"}, {"KEYM"},{"BOOTLOADER2"}, {"CFG"}, {"MAIN FW"}};

  if ( !image_location_blob || !udev ) {
    printf("%s() pointer is NULL!\n",__func__);
    goto error_exit;
  }

  printf("******Start sending %s******\n", comp_name[idx]);

  if ( is_blob_image ) {
    printf("File: %s\n", pesw_image_path[PESW_BLOB]);
  } else {
    printf("File: %s\n", pesw_image_path[idx]);
  }

  if ( is_usb == false ) {
    //do nothing
    printf("Please update it via USB!\n");
  } else {
    if ( is_blob_image ) {
      ret = bic_update_pesw_fw_usb(slot_id, pesw_image_path[PESW_BLOB], udev, comp_name[idx], intf, idx, is_blob_image, image_location_blob);
    } else {
      ret = bic_update_pesw_fw_usb(slot_id, pesw_image_path[idx], udev, comp_name[idx], intf, idx, is_blob_image, image_location_blob);
    }
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
    ret = _check_pesw_status(slot_id, intf, idx, true, NULL, 0); // execute bl2
    if ( ret < 0 ) {
      printf("Failed to activate it\n");
      goto error_exit;
    }
  }

  uint8_t expected_sts[2] = {PESW_PHASE2};
  uint8_t cmp_len = 1;
  // make sure PESW executes RCVRY image
  if ( idx == PESW_BOOTLOADER_RCVRY ) {
    printf("Checking the current phase of PESW...\n");
  } else if ( idx == PESW_KEYM ) {
    printf("Toggle KEYM image...\n");
    ret = _toggle_pesw(slot_id, intf, (0x1 << PESW_KEYM), is_rcvry);
    if ( ret < 0 ) {
      printf("Failed to toggle KEYM\n");
      goto error_exit;
    }
  } else {
    expected_sts[0] = PESW_ACT_STS;
    expected_sts[1] = PESW_VALID_STS;
    cmp_len = 2;
    if ( idx != PESW_PARTMAP ) expected_sts[0] = !PESW_ACT_STS;
  }

  // no need to check the phase if idx is PESW_KEYM
  ret = (idx == PESW_KEYM)?ret:_check_pesw_status(slot_id, intf, idx, false, &expected_sts[0], cmp_len); // check status
  if ( ret < 0 ) {
    printf("Failed to get the phase status\n");
    goto error_exit;
  }

error_exit:
  return ret;
}

static int
_update_mchp(uint8_t slot_id, uint8_t type, uint8_t intf, bool is_usb, bool is_rcvry, uint8_t board_type, bool is_blob_image, int *image_location_blob) {
  struct timeval start, end;
  int ret = BIC_STATUS_FAILURE;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  if ( !image_location_blob ) {
    printf("%s() pointer is NULL!\n",__func__);
    goto error_exit;
  }

  // start
  gettimeofday(&start, NULL);

  // enable USB port
  if ( is_usb == true ) {
    ret = _init_usb_intf(slot_id, board_type, intf, udev);
    if ( ret < 0 ) {
      goto error_exit;
    }
  }

  // try to send the images
  for (int i = PESW_BOOTLOADER_RCVRY; i < PESW_IMG_CNT; i++) {
    if ( ((type >> i) & 0x1) == 0 ) continue;

    ret = _process_mchp_images(slot_id, i, intf, udev, is_usb, is_rcvry, is_blob_image, image_location_blob);
    if ( ret < 0 ) {
      // the USB HUB will lost during PESW recovery update
      // make sure udev can be released gracefully
      if ( udev->handle != NULL ) bic_close_usb_dev(udev);
      goto error_exit;
    }
  }

  // disable USB port because all firmware images are sent
  if ( is_usb == true ) {
    // only close it when it's claimed successfully
    if ( udev->handle != NULL ) bic_close_usb_dev(udev);
  }

  syslog(LOG_ERR, "%s() quit\n", __func__);
  // quit
  ret = _quit_pesw_update(slot_id, type, udev, intf, is_rcvry, board_type, image_location_blob);
  if ( ret < 0 ) {
    goto error_exit;
  }

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

error_exit:
  return ret;
}

int update_bic_mchp(uint8_t slot_id, uint8_t comp __attribute__((unused)), char *image, uint8_t intf, uint8_t force, bool is_usb) {
  int ret = BIC_STATUS_FAILURE;
  bool is_rcvry = false;
  uint8_t type = 0;
  uint8_t file_s_info = 0;
  uint8_t sys_conf = 0;
  uint8_t bmc_location = 0, board_type = UNKNOWN_BOARD;
  bool is_blob_image = false;
  int image_location_blob[PESW_IMG_CNT] = {0};

  // check files - image path, image type, image secure information
  ret = _is_valid_files(image, &type, &file_s_info, &is_blob_image, image_location_blob);
  if ( ret < 0 ) {
    printf("Invalid inputs: %s\n", image);
    goto error_exit;
  }

  ret = fby3_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
    goto error_exit;
  }

  if (bmc_location == NIC_BMC) {
    ret = fby3_common_get_2ou_board_type(slot_id, &board_type);
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Fails to get 2ou board type", __func__);
      goto error_exit;
    }
  }

  //enter recovery mode first before query pesw info
  if (type == PESW_MASK_S_RCVRY || type == PESW_MASK_RCVRY) {
    // blob image do not support rcvry function
    if ( is_blob_image ) {
      goto error_exit;
    }
    ret = _enter_pesw_rcvry_mode(slot_id, intf, true, board_type);
    if ( ret < 0 ) {
      goto error_exit;
    }
  }

  if ( force == false ) {
    // get the current PCIe switch fw info
    ret = _get_pcie_sw_config_info(slot_id, &sys_conf, intf);
    if ( ret < 0 ) {
      printf("Failed to get PCIe config info\n");
      goto error_exit;
    }
  }

  switch (type) {
    case PESW_MASK_S_RCVRY:
      is_rcvry = true;
      break;
    case PESW_MASK_RCVRY:
      // if file_s_info > 0, it means KEYM is missing or mixed files(signed and unsigned files are put together)
      if ( file_s_info > 0 || sys_conf == PESW_INIT_SECURED ) ret = BIC_STATUS_FAILURE;
      is_rcvry = true;
      break;
    default:
      // if input files dont meet the condition above, check it further
      if ( (type & PESW_MASK_N_NOT_SUP) > 0 ) {
        printf("Err: Want to update but recovery images(rcvry, partmap, or keym) are detected\n");
        ret = BIC_STATUS_FAILURE;
      } else {
        // check the PESW firmware state and image information
        if ( sys_conf == PESW_INIT_SECURED ) {
          // type and file_s_info should be the same
          if ( type != file_s_info ) ret = BIC_STATUS_FAILURE;
        }
      }
      break;
  }

  if ( ret == BIC_STATUS_FAILURE ) {
    printf("You are trying to update the PESW firmware with the unexpected package\n");
    printf("type: %02X, file_s:%02X\n", type, file_s_info);
    if ( force == false ) goto error_exit;
    else {
      printf("force update is set, keep running...\n");
    }
  }

  // start updating PESW
  ret = _update_mchp(slot_id, type, intf, is_usb, is_rcvry, board_type, is_blob_image, image_location_blob);

error_exit:
  return ret;
}
