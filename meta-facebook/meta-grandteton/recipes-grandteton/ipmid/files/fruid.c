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
#include <openbmc/hal_fruid.h>
#include "fruid.h"

#define BIN_MB        "/tmp/fruid_mb.bin"
#define FRUID_SIZE    512
#define MAX_FRU_READ_RETRY (3)

/*
 * copy_eeprom_to_bin - copy the eeprom to binary file im /tmp directory
 *
 * @eeprom_file   : path for the eeprom of the device
 * @bin_file      : path for the binary file
 *
 * returns 0 on successful copy
 * returns non-zero on file operation errors
 */
int copy_eeprom_to_bin(const char * eeprom_file, const char * bin_file) {
  int eeprom = -1;
  int bin = -1;
  uint64_t tmp[FRUID_SIZE];
  ssize_t bytes_rd, bytes_wr;

  errno = 0;

  if (access(eeprom_file, F_OK) != -1) {
    eeprom = open(eeprom_file, O_RDONLY);
    if (eeprom == -1) {
      syslog(LOG_ERR, "%s: unable to open the %s file: %s",
          __func__, eeprom_file, strerror(errno));
      goto exit;
    }

    bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
    if (bin == -1) {
      syslog(LOG_ERR, "%s: unable to create %s file: %s",
         __func__, bin_file, strerror(errno));
      goto exit;
    }

    bytes_rd = read(eeprom, tmp, FRUID_SIZE);
    if (bytes_rd < 0) {
      syslog(LOG_ERR, "%s: read %s file failed: %s", __func__, eeprom_file, strerror(errno));
      goto exit;
    } else if (bytes_rd < FRUID_SIZE) {
      syslog(LOG_ERR, "%s: FRU size:%d byte, is not equal to define FRU size:%d bytes", __func__, bytes_rd, FRUID_SIZE);
      errno = -1;
      goto exit;
    }

    bytes_wr = write(bin, tmp, bytes_rd);
    if (bytes_wr != bytes_rd) {
      syslog(LOG_ERR, "%s: write to %s file failed: %s",
          __func__, bin_file, strerror(errno));
      goto exit;
    }
exit:
    if (bin >= 0) {
      close(bin);
    }
    if (eeprom >= 0) {
      close(eeprom);
    }
  }

  return errno;
}

static void
*pldm_fru_reader(void *arg) {
  uint8_t retry = MAX_FRU_READ_RETRY;
  char fru_bin_path[32] = {0};
  int fru = (int)arg;
  uint8_t status = FRU_PRSNT;
  bic_intf fru_bic_info = {0};

  fru_bic_info.fru_id = fru;
  fru_bic_info.dev_id = DEV_NONE;
  pal_get_bic_intf(&fru_bic_info);

  if (!pal_is_artemis()) {
    fru_bic_info.fru_id = pal_get_pldm_fru_id(fru);
  }

  while (retry > 0) {
    if (!pal_is_fru_prsnt(fru, &status) && status == FRU_PRSNT) {
      break;
    }
    retry--;
    sleep(1);
  }
  if (retry == 0) {
    syslog(LOG_WARNING, "%s: Get FRU:%d Present Failed", __func__, fru);
    goto end;
  }

  if (pal_get_fruid_path(fru, fru_bin_path)) {
    syslog(LOG_WARNING, "%s: Get FRU:%d Bin Path Failed", __func__, fru);
  }

  sleep(2);
  retry = MAX_FRU_READ_RETRY;
  while(retry > 0) {
    if (hal_read_pldm_fruid(fru_bic_info, fru_bin_path, FRUID_SIZE) == 0) {
      break;
    }

    retry--;
    sleep(1);
  }
  if (retry == 0) {
    syslog(LOG_WARNING, "%s: Read FRU:%d Failed", __func__, fru);
  }

end:
  pthread_exit(NULL);
}

int plat_fruid_init(void) {
  pthread_t tid_fru[FRU_CNT] = {0};
  char eeprom_path[64] = {0};
  char bin_path[32] = {0};
  unsigned int caps = 0;
  uint8_t retry = MAX_FRU_READ_RETRY;

  for (int fru = 1; fru < FRU_CNT; fru ++) {
    // Get FRU data from PLDM
    if ( (pal_get_fru_path_type(fru) == FRU_PATH_PLDM) && !pal_get_fru_capability(fru, &caps)) {
      if ((caps & FRU_CAPABILITY_FRUID_READ)) {
        if (pthread_create(&tid_fru[fru], NULL, pldm_fru_reader, (void *)fru) < 0) {
          syslog(LOG_WARNING, "%s: Create pldm_fru_reader Failed, FRU: %d", __func__, fru);
        }
      }
      continue;
    }


    if (pal_get_fruid_path(fru, bin_path)) {
      syslog(LOG_WARNING, "%s: Get Fruid%d Bin Path Failed", __func__, fru);
      continue;
    }

    if (pal_get_fruid_eeprom_path(fru, eeprom_path)) {
      syslog(LOG_WARNING, "%s: Get Fruid%d EEPROM Path Failed", __func__, fru);
      continue;
    }
    retry = MAX_FRU_READ_RETRY;
    while (retry > 0) {
      if (copy_eeprom_to_bin(eeprom_path, bin_path) == 0) {
        break;
      }
      syslog(LOG_WARNING, "%s: Copy FRU%d EEPROM Failed, Retry:%d", __func__, fru, retry);
      retry --;
      sleep(1);
    }
    if (retry == 0) {
      syslog(LOG_WARNING, "%s: Read FRU:%d Failed", __func__, fru);
    }
    msleep(100);
  }

  for (int fru = 1; fru < FRU_CNT; fru ++) {
    if ( (pal_get_fru_path_type(fru) == FRU_PATH_PLDM) && !pal_get_fru_capability(fru, &caps)) {
      if ((caps & FRU_CAPABILITY_FRUID_READ)) {
        if (pthread_join(tid_fru[fru], NULL)) {
          syslog(LOG_WARNING, "%s pthread join Fru: %u failed", __func__, fru);
        }
      }
    }
  }
  return 0;
}

int plat_fruid_size(unsigned char payload_id) {
  struct stat buf;
  int ret;
  char bin[32] = {0};

  if (pal_get_fruid_path(FRU_MB, bin)) {
    syslog(LOG_WARNING, "%s: Get Fruid MB Bin Path Failed", __func__);
    return -1;
  }

  // check the size of the file and return size
  ret = stat(bin, &buf);
  if (ret) {
    return 0;
  }

  return buf.st_size;
}

int plat_fruid_data(unsigned char payload_id, int fru_id, int offset, int count, unsigned char *data) {
  int fd;
  int ret;
  char bin[32] = {0};

  if (pal_get_fruid_path(FRU_MB, bin)) {
    syslog(LOG_WARNING, "%s: Get Fruid MB Bin Path Failed", __func__);
    return -1;
  }

  // open file for read purpose
  fd = open(bin, O_RDONLY);
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
