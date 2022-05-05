/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file provides platform specific implementation of FRUID information
 *
 * FRUID specification can be found at
 * www.intel.com/content/dam/www/public/us/en/documents/product-briefs/platform-management-fru-document-rev-1-2-feb-2013.pdf
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <facebook/fby35_common.h>
#include <facebook/fby35_fruid.h>
#include "fruid.h"

#define FRUID_SIZE        512

typedef struct {
  uint8_t fruid;
  uint8_t devid;
} fruid_to_devid;

fruid_to_devid mapping[] = {
  { FRU_ID_BOARD_1OU    ,BOARD_1OU    },
  { FRU_ID_BOARD_2OU    ,BOARD_2OU    },
  { FRU_ID_1OU_DEV0     ,DEV_ID0_1OU  },
  { FRU_ID_1OU_DEV1     ,DEV_ID1_1OU  },
  { FRU_ID_1OU_DEV2     ,DEV_ID2_1OU  },
  { FRU_ID_1OU_DEV3     ,DEV_ID3_1OU  },
  { FRU_ID_2OU_DEV0     ,DEV_ID0_2OU  },
  { FRU_ID_2OU_DEV1     ,DEV_ID1_2OU  },
  { FRU_ID_2OU_DEV2     ,DEV_ID2_2OU  },
  { FRU_ID_2OU_DEV3     ,DEV_ID3_2OU  },
  { FRU_ID_2OU_DEV4     ,DEV_ID4_2OU  },
  { FRU_ID_2OU_DEV5     ,DEV_ID5_2OU  },
  { FRU_ID_2OU_DEV6     ,DEV_ID6_2OU  },
  { FRU_ID_2OU_DEV7     ,DEV_ID7_2OU  },
  { FRU_ID_2OU_DEV8     ,DEV_ID8_2OU  },
  { FRU_ID_2OU_DEV9     ,DEV_ID9_2OU  },
  { FRU_ID_2OU_DEV10    ,DEV_ID10_2OU },
  { FRU_ID_2OU_DEV11    ,DEV_ID11_2OU },
  { FRU_ID_2OU_DEV12    ,DEV_ID12_2OU },
  { FRU_ID_2OU_DEV13    ,DEV_ID13_2OU },
  { FRU_ID_2OU_X8       ,BOARD_2OU_X8 },
  { FRU_ID_2OU_X16      ,BOARD_2OU_X16},
};

/*
 * copy_eeprom_to_bin - copy the eeprom to binary file im /tmp directory
 *
 * @eeprom_file   : path for the eeprom of the device
 * @bin_file      : path for the binary file
 *
 * returns 0 on successful copy
 * returns non-zero on file operation errors
 */
int copy_eeprom_to_bin(const char *eeprom_file, const char *bin_file) {

  int eeprom;
  int bin;
  uint64_t tmp[FRUID_SIZE];
  ssize_t bytes_rd, bytes_wr;

  errno = 0;

  eeprom = open(eeprom_file, O_RDONLY);
  if (eeprom == -1) {
    syslog(LOG_ERR, "%s: unable to open the %s file: %s",
	__func__, eeprom_file, strerror(errno));
    return errno;
  }

  bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
  if (bin == -1) {
    syslog(LOG_ERR, "%s: unable to create %s file: %s",
	__func__, bin_file, strerror(errno));
    goto err;
  }

  bytes_rd = read(eeprom, tmp, FRUID_SIZE);
  if (bytes_rd < 0) {
    syslog(LOG_ERR, "%s: read %s file failed: %s",
	__func__, eeprom_file, strerror(errno));
    goto exit;
  } else if (bytes_rd < FRUID_SIZE) {
    syslog(LOG_ERR, "%s: less than %d bytes", __func__, FRUID_SIZE);
    goto exit;
  }

  bytes_wr = write(bin, tmp, bytes_rd);
  if (bytes_wr != bytes_rd) {
    syslog(LOG_ERR, "%s: write to %s file failed: %s",
	__func__, bin_file, strerror(errno));
    goto exit;
  }

exit:
  close(bin);
err:
  close(eeprom);

  return errno;
}

static int
fruid_init_local_fru() {
  int ret = 0;
  char path[128] = {0};
  int path_len = sizeof(path);
  uint8_t fru_bus = 0;
  uint8_t fru_addr = 0;
  char *fru_path = NULL;
  char dev_path[MAX_FRU_PATH_LEN] = {0};
  uint8_t bmc_location = 0;
  uint8_t i = 0;
  uint8_t type_2ou = UNKNOWN_BOARD;

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if ( bmc_location == BB_BMC ) {
    fru_bus = CLASS1_FRU_BUS;
    fru_addr = BB_FRU_ADDR;
    fru_path = FRU_BB_BIN;
  } else {
    fru_bus = CLASS2_FRU_BUS;
    fru_addr = NICEXP_FRU_ADDR;
    fru_path = FRU_NICEXP_BIN;
  }

  //reinitialize ret
  ret = -1;

  //create the fru binary in /tmp/
  //fruid_bmc.bin
  snprintf(path, path_len, EEPROM_PATH, fru_bus, BMC_FRU_ADDR);
  if ( copy_eeprom_to_bin(path, FRU_BMC_BIN) < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_BMC_BIN);
    goto error_exit;
  }

  //fruid_nic.bin
  snprintf(path, path_len, EEPROM_PATH, NIC_FRU_BUS, NIC_FRU_ADDR);
  if ( copy_eeprom_to_bin(path, FRU_NIC_BIN) < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, FRU_NIC_BIN);
    goto error_exit;
  }

  snprintf(path, path_len, EEPROM_PATH, fru_bus, fru_addr);

  //fruid_nicexp.bin or fruid_bb.bin
  if ( copy_eeprom_to_bin(path, fru_path) < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, fru_path);
    goto error_exit;
  }

  //DPV2 X8 furid
  for (i = FRU_SLOT1; i <= FRU_SLOT3; i += 2) {
    if ( fby35_common_get_2ou_board_type(i, &type_2ou) < 0 ) {
      continue;
    } else {
      if ( (type_2ou & DPV2_X8_BOARD) == DPV2_X8_BOARD ) {
        snprintf(path, path_len, EEPROM_PATH, FRU_DPV2_X8_BUS(i), DPV2_FRU_ADDR);
        snprintf(dev_path, sizeof(dev_path), FRU_DEV_PATH, i, BOARD_2OU_X8);
        if ( copy_eeprom_to_bin(path, dev_path) < 0 ) {
          syslog(LOG_WARNING, "%s() Failed to copy %s to %s", __func__, path, dev_path);
          continue;
        }
      }
    }
  }

  ret = 0;

error_exit:
  return ret;
}

/* Populate the platform specific eeprom for fruid info */
int plat_fruid_init(void) {
  int ret;

  //export FRU that is connected to BMC directly.
  ret = fruid_init_local_fru();
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "%s() somethings went wrong in fruid_init_local_fru()", __func__);
  }

  return ret;
}

int plat_fruid_data(unsigned char payload_id, int fru_id, int offset, int count, unsigned char *data) {
  char fpath[64] = {0};
  int fd;
  int ret;
  int index;

  switch (fru_id) {
    case FRU_ID_SERVER:
      sprintf(fpath, FRU_SLOT_BIN, payload_id);
      break;
    case FRU_ID_BMC:
      sprintf(fpath, FRU_BMC_BIN);
      break;
    case FRU_ID_NIC:
      sprintf(fpath, FRU_NIC_BIN);
      break;
    case FRU_ID_BB:
      sprintf(fpath, FRU_BB_BIN);
      break;
    case FRU_ID_NICEXP:
      sprintf(fpath, FRU_NICEXP_BIN);
      break;

    default:
      for(index=0; index < (sizeof(mapping)/sizeof(mapping[0])); index++){
        if (mapping[index].fruid == fru_id) {
          break;
        }
      }

      if (index < (sizeof(mapping)/sizeof(mapping[0]))) {
        sprintf(fpath, FRU_DEV_PATH, payload_id, mapping[index].devid);
        syslog(LOG_INFO, "FRU path %s", fpath);
        break;
      } else {
        syslog(LOG_WARNING, "cannot find fruid %d in mapping table", fru_id);
        return -1;
      }
  }

  // open file for read purpose
  fd = open(fpath, O_RDONLY);
  if (fd < 0) {
    return fd;
  }

  // seek position based on given offset
  ret = lseek(fd, offset, SEEK_SET);
  if (ret < 0) {
    close(fd);
    return ret;
  }

  // read the file content
  ret = read(fd, data, count);
  if (ret != count) {
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

int plat_fruid_size(unsigned char payload_id) {
  char fpath[64] = {0};
  struct stat buf;
  int ret;

  // Fill the file path for a given slot
  sprintf(fpath, FRU_SLOT_BIN, payload_id);

  // check the size of the file and return size
  ret = stat(fpath, &buf);
  if (ret) {
    return 0;
  }

  return buf.st_size;
}
