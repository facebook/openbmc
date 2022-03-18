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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <openbmc/obmc-i2c.h>
#include "bic_cpld_altera_fwupdate.h"
#include "bic_ipmi.h"
#include "bic_xfer.h"

//#define DEBUG

/****************************/
/*      CPLD fw update      */
/****************************/
#define MAX_RETRY 3
#define CPLD_UPDATE_ADDR 0x80

const char *board_stage[] = {"Unknown", "EVT", "DVT", "PVT", "MP"};

#define SET_READ_DATA(buf, reg, offset) \
                     do { \
                       buf[offset++] = (reg >> 24) & 0xff; \
                       buf[offset++] = (reg >> 16) & 0xff; \
                       buf[offset++] = (reg >> 8) & 0xff; \
                       buf[offset++] = (reg >> 0) & 0xff; \
                     } while(0)\

#define SET_WRITE_DATA(buf, reg, value, offset) \
                     do { \
                       buf[offset++] = (reg >> 24) & 0xff; \
                       buf[offset++] = (reg >> 16) & 0xff; \
                       buf[offset++] = (reg >> 8)  & 0xff; \
                       buf[offset++] = (reg >> 0)  & 0xff; \
                       buf[offset++] = (value >> 0 )  & 0xFF; \
                       buf[offset++] = (value >> 8 )  & 0xFF; \
                       buf[offset++] = (value >> 16 ) & 0xFF; \
                       buf[offset++] = (value >> 24 ) & 0xFF; \
                    } while(0)\

#define GET_VALUE(buf) (buf[0] << 0) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24)

static int
set_register_via_bypass(uint8_t slot_id, int reg, int val, uint8_t intf) {
  int ret;
  int retries = MAX_RETRY;
  uint8_t txbuf[32] = {0};
  uint8_t rxbuf[32] = {0};
  uint8_t txlen = 0;
  uint8_t rxlen = 0;

  if ( intf == NONE_INTF ) {
    //BIC is located on the server board
    txbuf[0] = CPLD_SB_BUS; //bus
  } else {
    //BIC is located on the baseboard
    txbuf[0] = CPLD_BB_BUS; //bus
  }

  txbuf[1] = CPLD_UPDATE_ADDR; //slave
  txbuf[2] = 0x00; //read cnt
  txlen = 3;
  SET_WRITE_DATA(txbuf, reg, val, txlen);

  while ( retries > 0 ) {
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, txbuf, txlen, rxbuf, &rxlen, intf);
    if (ret) {
      sleep(1);
      retries--;
    } else {
      break;
    }
  }

  if ( retries == 0 ) {
     printf("%s() write register fails after retry %d times. ret=%d \n", __func__, MAX_RETRY, ret);
  }
  return ret;
}

static int
get_register_via_bypass(uint8_t slot_id, int reg, int *val, uint8_t intf) {
  int ret;
  int retries = MAX_RETRY;
  uint8_t txbuf[32] = {0};
  uint8_t rxbuf[32] = {0};
  uint8_t txlen = 0;
  uint8_t rxlen = 0;

  if ( intf == NONE_INTF ) {
    //BIC is located on the server board
    txbuf[0] = CPLD_SB_BUS; //bus
  } else {
    //BIC is located on the baseboard
    txbuf[0] = CPLD_BB_BUS; //bus
  }

  txbuf[1] = CPLD_UPDATE_ADDR; //slave
  txbuf[2] = 0x04; //read cnt
  txlen = 3;
  SET_READ_DATA(txbuf, reg, txlen);

  while ( retries > 0 ) {
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, txbuf, txlen, rxbuf, &rxlen, intf);
    if (ret) {
      sleep(1);
      retries--;
    } else {
      break;
    }
  }

  if ( retries == 0 ) {
     printf("%s() write register fails after retry %d times. ret=%d \n", __func__, MAX_RETRY, ret);
  }

  *val = GET_VALUE(rxbuf);
  return ret;
}

#define ON_CHIP_FLASH_IP_DATA_REG         0x00000000
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG   0x00200020
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG     0x00200024
#define ON_CHIP_FLASH_USER_VER            0x00200028
#define DUAL_BOOT_IP_BASE                 0x00200000

// status register
#define BUSY_IDLE      0x00
#define BUSY_ERASE     0x01
#define BUSY_WRITE     0x02
#define BUSY_READ      0x03

#define READ_SUCCESS   0x04
#define WRITE_SUCCESS  0x08
#define ERASE_SUCCESS  0x10

enum{
  WP_CFM0 = 0x1 << 27,
  WP_CFM1 = 0x1 << 26,
  WP_CFM2 = 0x1 << 25,
  WP_UFM0 = 0x1 << 24,
  WP_UFM1 = 0x1 << 23,
};

typedef enum{
  Sector_CFM0,
  Sector_CFM1,
  Sector_CFM2,
  Sector_UFM0,
  Sector_UFM1,
  Sector_NotSet,
} SectorType_t;

static int
read_register(uint8_t slot_id, int address, uint8_t intf) {
  int ret = 0;
  int data = 0;

  ret = get_register_via_bypass(slot_id, address, &data, intf);
  if ( ret < 0 ) {
    printf("%s() Cannot read 0x%x", __func__, address);
  }

  return data;
}

static int
write_register(uint8_t slot_id, int address, int data, uint8_t intf) {
  return set_register_via_bypass(slot_id, address, data, intf);
}

static int
Max10_get_status(uint8_t slot_id, uint8_t intf) {
  return read_register(slot_id, ON_CHIP_FLASH_IP_CSR_STATUS_REG, intf);
}

static int
Max10_protect_sectors(uint8_t slot_id, uint8_t intf) {
  int value = 0;

  value = read_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, intf);
  value |= (0x1F<<23);

  return write_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, value, intf);
}

static int
Max10_unprotect_sector(uint8_t slot_id, SectorType_t secType, uint8_t intf) {

  int value = 0;
  value = read_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, intf);

#ifdef DEBUG
  printf("%s() 0x%x\n", __func__, value);
#endif

  switch(secType) {
    case Sector_CFM0:
      value = value & (~WP_CFM0);
      break;
    case Sector_CFM1:
      value = value & (~WP_CFM1);
      break;
    case Sector_CFM2:
      value = value & (~WP_CFM2);
      break;
    case Sector_UFM0:
      value = value & (~WP_UFM0);
      break;
    case Sector_UFM1:
      value = value & (~WP_UFM1);
      break;
    case Sector_NotSet:
      value = 0xFFFFFFFF;
      break;
  }

#ifdef DEBUG
  write_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, value, intf);
  value = read_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, intf);
  printf("%s() 0x%x\n", __func__, value);
  return 0;
#else
  return write_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, value, intf);
#endif
}

static int
Max10_erase_sector(uint8_t slot_id, SectorType_t secType, uint8_t intf) {
  int ret = 0;
  int value = 0;
  int done = 0;

  //Control register bit 20~22
  enum{
    Sector_erase_CFM0   = 0b101 << 20,
    Sector_erase_CFM1   = 0b100 << 20,
    Sector_erase_CFM2   = 0b011 << 20,
    Sector_erase_UFM0   = 0b010 << 20,
    Sector_erase_UFM1   = 0b001 << 20,
    Sector_erase_NotSet = 0b111 << 20,
  };

  value = read_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, intf);
  value &= ~(0x7<<20);  // clear bit 20~22.

#ifdef DEBUG
  printf("%s() 0x%x\n", __func__, value);
#endif

  switch(secType)
  {
    case Sector_CFM0:
      value |= Sector_erase_CFM0;
      break;
    case Sector_CFM1:
      value |= Sector_erase_CFM1;
      break;
    case Sector_CFM2:
      value |= Sector_erase_CFM2;
      break;
    case Sector_UFM0:
      value |= Sector_erase_UFM0;
      break;
    case Sector_UFM1:
      value |= Sector_erase_UFM1;
      break;
    case Sector_NotSet:
      value |= Sector_erase_NotSet;
      break;
  }

#ifdef DEBUG
  printf("%s() w 0x%x\n", __func__, value);
#endif

  write_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, value, intf);

#ifdef DEBUG
  printf("%s() r 0x%x\n", __func__, read_register(slot_id, ON_CHIP_FLASH_IP_CSR_CTRL_REG, intf));
#endif

  while ( done == 0 ) {
    value = read_register(slot_id, ON_CHIP_FLASH_IP_CSR_STATUS_REG, intf);
#ifdef DEBUG
    printf("%s() busy 0x%x\n", __func__, value);
#endif

    if( value & BUSY_ERASE) {
      usleep(500*1000);
      continue;
    }

    if ( value & ERASE_SUCCESS ) {
      printf("Erase sector SUCCESS. \n");
      done = 1;
    } else {
      printf("Failed to erase the sector. Is the dev read only?\n");
      ret = -1;
      sleep(1);
    }
  }

  return ret;
}

static int
is_valid_cpld_image(uint8_t slot_id, uint8_t signed_byte, uint8_t intf) {
  int ret = -1;
  uint8_t tbuf[4] = {0};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int retry= 0;
  int board_type_index = 0;
  bool board_rev_is_invalid = false;

  switch (intf) {
    case BB_BIC_INTF:
        // Read Board Revision from BB CPLD
        tbuf[0] = CPLD_BB_BUS;
        tbuf[1] = CPLD_FLAG_REG_ADDR;
        tbuf[2] = 0x01;
        tbuf[3] = BB_CPLD_BOARD_REV_ID_REGISTER;
        tlen = 4;
        retry = 0;
        while (retry < RETRY_TIME) {
          ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
          if ( ret < 0 ) {
            retry++;
            msleep(100);
          } else {
            break;
          }
        }
        if (retry == RETRY_TIME) {
          syslog(LOG_WARNING, "%s() Failed to get board revision via BB CPLD, tlen=%d", __func__, tlen);
          goto error_exit;
        }

        board_type_index = rbuf[0] - 1;
        if (board_type_index < 0) {
          board_type_index = 0;
        }

        if (board_type_index <= BB_REV_POC2) {
          if (REVISION_ID(signed_byte) != FW_REV_POC) {
            board_rev_is_invalid = true;
          }
        } else {
          if (REVISION_ID(signed_byte) < FW_REV_EVT) {
            board_rev_is_invalid = true;
          }
        }

        if (board_rev_is_invalid) {
          printf("To prevent this update on slot%d , please use the f/w of %s on the %s system\n",
                  slot_id, board_stage[board_type_index], board_stage[board_type_index]);
          printf("To force the update, please use the --force option.\n");
        }

        return ((COMPONENT_ID(signed_byte) == BICBB) && !board_rev_is_invalid)?0:-1;
      break;
  }

error_exit:
  return ret;
}

static int
_update_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retries = MAX_RETRY;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;

  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;

  do {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      sleep(1);
      printf("_update_fw: slot: %d, target %d, offset: %u, len: %d retrying..\n", slot_id, target, offset, len);
    } else break;
  } while ( retries-- > 0 );

  return ret;
}

int
update_bic_cpld_altera(uint8_t slot_id, char *image, uint8_t intf, uint8_t force) {
#define MAX10_RPD_SIZE 0x5C000
  uint8_t *rpd_file = NULL;
  uint8_t buf[256] = {0};
  const uint8_t target = UPDATE_CPLD;
  const uint16_t read_count = 128;
  uint32_t offset = 0, last_offset = 0, dsize = 0;
  struct stat finfo;
  struct timeval start, end;
  int ret = 0;
  int fd = 0;
  int rpd_filesize = 0;
  int read_bytes = 0;

  //Two BMCs are designed in Config C.
  //They can access the CPLD of the baseboard at the same time.
  //This means the CPLD can be updated in parallel.
  //The update is unacceptable in Config C. Therefore, the BMC notify another BMC when update start.
  //Before the update, BMC check twice if the update flag is set.
  //Since the update don't start, return ret if one of them is failure.
  if ( intf == BB_BIC_INTF && force != FORCE_UPDATE_SET) {
    if (bic_check_bb_fw_update_ongoing() != 0) {
      return -1;
    }
    if (bic_set_bb_fw_update_ongoing(FW_BB_CPLD, SEL_ASSERT) != 0) {
      printf("Failed to set firmware update ongoing\n");
      return -1;
    }
  }

  SectorType_t secType = Sector_CFM1;

  printf("OnChip Flash Status = 0x%X., slot_id 0x%x, sectype 0x%x, intf: 0x%x, ", Max10_get_status(slot_id, intf), slot_id, secType, intf);

  //step 0 - Open the file
  if ((fd = open(image, O_RDONLY)) < 0) {
    printf("Fail to open file: %s.\n", image);
    ret = -1;
    goto error_exit;
  }

  fstat(fd, &finfo);
  rpd_filesize = finfo.st_size;
  rpd_file = malloc(rpd_filesize);
  if( rpd_file == 0 ) {
    printf("Failed to allocate memory\n");
    ret = -1;
    goto error_exit;
  }

  read_bytes = read(fd, rpd_file, rpd_filesize);
  printf("read %d bytes.\n", read_bytes);

  switch (force) {
    case FORCE_UPDATE_UNSET:
      ret = BIC_STATUS_FAILURE;
      if ( rpd_filesize == MAX10_RPD_SIZE ) {
        printf("The image is the unsigned CPLD image!\n");
      } else if ( (MAX10_RPD_SIZE + 1) == rpd_filesize ) {
        ret = is_valid_cpld_image(slot_id, rpd_file[MAX10_RPD_SIZE] & 0xff, intf);
        if ( ret < 0 ) {
          printf("The image is not the valid CPLD image for this component.\n");
        } else {
          ret = BIC_STATUS_SUCCESS;
        }
      } else {
        printf("The size of image is not expected!\n");
      }
    case FORCE_UPDATE_SET:
      /*fall through*/
    default:
      //adjust the size since one byte is the signature of the CPLD image.
      if ( read_bytes == (MAX10_RPD_SIZE + 1) ) read_bytes -= 1;
      break;
  }

  //something went wrong. exit.
  if ( ret == BIC_STATUS_FAILURE ) {
    goto error_exit;
  }

  //step 1 - UnprotectSector
  ret = Max10_unprotect_sector(slot_id, secType, intf);
  if ( ret < 0 ) {
    printf("Failed to unprotect the sector.\n");
    goto error_exit;
  }

  //step 2 - Erase a sector
  ret = Max10_erase_sector(slot_id, secType, intf);
  if ( ret < 0 ) {
    printf("Failed to erase the sector.\n");
    goto error_exit;
  }

  //step 3 - Set Erase to `None`
  ret = Max10_erase_sector(slot_id, Sector_NotSet, intf);
  if ( ret < 0 ) {
    printf("Failed to set None.\n");
    goto error_exit;
  }

  dsize = read_bytes/100;
  gettimeofday(&start, NULL);
  while (1) {
    // Read from file
    memcpy(buf, &rpd_file[offset], read_count);

    // Send data to Bridge-IC
    ret = _update_fw(slot_id, target, offset, read_count, buf, intf);
    if ( ret < 0 ) {
      goto error_exit;
    }

    // Update counter
    offset += read_count;
    if ((last_offset + dsize) <= offset) {
      _set_fw_update_ongoing(slot_id, 60);
      printf("\rupdated cpld: %u %%", offset/dsize);
      fflush(stdout);
      last_offset += dsize;
    }

    //if all data are sent, break it.
    if ( offset == read_bytes ) break;
  }

  //step 5 - protect sectors
  ret = Max10_protect_sectors(slot_id, intf);
  if ( ret < 0 ) {
    printf("Failed to protect the sector.\n");
    goto error_exit;
  }

  gettimeofday(&end, NULL);
  printf("\nElapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

error_exit:
  if ( intf == BB_BIC_INTF ) {
    if (bic_set_bb_fw_update_ongoing(FW_BB_CPLD, SEL_DEASSERT) != 0) {
      printf("Failed to notify firmware update finish\n");
      ret = -1;
    }
  }
  if ( fd > 0 ) close(fd);
  if ( rpd_file != NULL ) free(rpd_file);

  return ret;
}
