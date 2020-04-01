/*
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
 * This file provides platform specific implementation of FRUID information
 *
 * FRUID specification can be found at
 * www.intel.com/content/dam/www/public/us/en/documents/product-briefs/
 * platform-management-fru-document-rev-1-2-feb-2013.pdf
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

/*
 * Wedge400 specific FRUID related routine. Porting old Wedge100 style
 * fruid code to fit into new IPMID v0.2 code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <openbmc/log.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/fruid.h>
#include <facebook/wedge_eeprom.h>
#include "fruid.h"

#define WEDGE400_FRUID_SIZE 0x100
#define WEDGE400_FRU_NUM 11

struct fruid_dev
{
  /* data */
  int id;
  int bus;
  int addr;
  unsigned char* mem;
  char format;
};

enum { FRU_STD_FORMAT, FRU_FB_FORMAT, FRU_PSU_FORMAT};

// Global structures
static unsigned char g_fruid[WEDGE400_FRU_NUM * WEDGE400_FRUID_SIZE] = {0};

static struct fruid_dev fruid_list[] = {
    {FRU_SCM, 17, 0x52, &g_fruid[0 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_SMB, 6, 0x51, &g_fruid[1 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_PEM1, 22, 0x50, &g_fruid[2 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_PEM2, 23, 0x50, &g_fruid[3 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_FCM, 31, 0x51, &g_fruid[4 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_PSU1, 22, 0x50, &g_fruid[5 * WEDGE400_FRUID_SIZE], FRU_PSU_FORMAT},
    {FRU_PSU2, 23, 0x50, &g_fruid[6 * WEDGE400_FRUID_SIZE], FRU_PSU_FORMAT},
    {FRU_FAN1, 34, 0x52, &g_fruid[7 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_FAN2, 35, 0x52, &g_fruid[8 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_FAN3, 36, 0x52, &g_fruid[9 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT},
    {FRU_FAN4, 37, 0x52, &g_fruid[10 * WEDGE400_FRUID_SIZE], FRU_FB_FORMAT}};

void *get_frudev(unsigned char id)
{
  for (int i = 0; i < WEDGE400_FRU_NUM; i++)
  {
    if (fruid_list[i].id == id)
    {
      return &fruid_list[i];
    }
  }
  return NULL;
}

#define COMMON_HDR_VER 1
#define PROD_INFO_VER 1
#define PROD_INFO_AREA_OFFSET 0x10
#define PROD_INFO_CKSUM_OFFSET (PROD_INFO_AREA_OFFSET + 1)
#define LANG_CODE_ENGLISH 25
#define TYPE_STR 0xC0
#define TYPE_LAST 0xC1
#define ZERO_CKSUM_CONST 0x100
#define LEN_BYTE_SIZE 8

typedef struct _fruid_common_hdr_t
{
  unsigned char ver;
  unsigned char internal_use_area_offset;
  unsigned char chassis_info_area_offset;
  unsigned char board_info_area_offset;
  unsigned char prod_info_area_offset;
  unsigned char multi_record_area_offset;
  unsigned char padding;
  unsigned char cksum;
} fruid_common_hdr_t;

// A macro to add an entry.
#define _APPEND_STR_VALUE(name)                                          \
  do                                                                     \
  {                                                                      \
    if (strlen(name) < 1 || i + 1 + strlen(name) >= WEDGE400_FRUID_SIZE) \
    {                                                                    \
      break;                                                             \
    }                                                                    \
    frumem[i++] = TYPE_STR + strlen(name);                               \
    memcpy(&frumem[i], name, strlen(name));                              \
    i += strlen(name);                                                   \
  } while (0)

// Main function to populate the fruid data structure, using the data
// from eeprom
static void populate_fruid(unsigned char id)
{
  int rc = 0;
  int len = 0;
  unsigned char zero_val = 0x0;
  char filename[64];
  struct fruid_dev *fruid = get_frudev(id);
  if (fruid == NULL)
  {
    OBMC_WARN("populate_fruid: can't find fruid in list %d\n", id);
    return;
  }
  unsigned char *frumem = fruid->mem;
  i2c_sysfs_slave_abspath(filename, sizeof(filename), fruid->bus, fruid->addr);
  strncat(filename, "/eeprom", sizeof(filename) - strlen(filename) - 1);
  if (fruid->format == FRU_STD_FORMAT)
  {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
      OBMC_WARN("populate_fruid: can't open file fruid %d : %s\n",
             id, filename);
      return;
    }
    if (fread(frumem, 1, WEDGE400_FRUID_SIZE, fp) != WEDGE400_FRUID_SIZE) {
      OBMC_WARN("populate_fruid: Read less than requested FRUID\n");
    }
    fclose(fp);
    return;
  }
  else if (fruid->format == FRU_PSU_FORMAT)
  {
    char eeprom[16];
    int i = PROD_INFO_AREA_OFFSET;
    fruid_common_hdr_t *chdr = (fruid_common_hdr_t *)frumem;
    fruid_info_t frupsu;
    memset(&frupsu, 0, sizeof(fruid_info_t));

    sprintf(eeprom,"24c02");
    pal_add_i2c_device(fruid->bus,fruid->addr,eeprom);
    rc = fruid_parse(filename, &frupsu);
    pal_del_i2c_device(fruid->bus,fruid->addr);

    if (rc)
    {
      sprintf(eeprom,"24c64");
      pal_add_i2c_device(fruid->bus,fruid->addr,eeprom);
      rc = fruid_parse(filename, &frupsu);
      pal_del_i2c_device(fruid->bus,fruid->addr);
      if (rc) {
        OBMC_WARN(
              "populate_fruid: wedge_eeprom_parse fruid_parse fail %d returns %d\n",
              id, rc);
        return;
      }
    }

    chdr->ver = COMMON_HDR_VER;
    chdr->cksum = chdr->ver + chdr->prod_info_area_offset;
    chdr->cksum = ZERO_CKSUM_CONST - chdr->cksum;

    chdr->prod_info_area_offset = PROD_INFO_AREA_OFFSET / LEN_BYTE_SIZE;
    frumem[i++] = PROD_INFO_VER;
    frumem[i++] = 0x00;
    frumem[i++] = LANG_CODE_ENGLISH;
    _APPEND_STR_VALUE(frupsu.product.mfg);
    _APPEND_STR_VALUE(frupsu.product.name);
    _APPEND_STR_VALUE(frupsu.product.part);
    _APPEND_STR_VALUE(frupsu.product.version);
    _APPEND_STR_VALUE(frupsu.product.serial);
    _APPEND_STR_VALUE(frupsu.product.asset_tag);
    _APPEND_STR_VALUE(frupsu.product.fruid);
    frumem[i++] = TYPE_LAST;

    len = i - PROD_INFO_AREA_OFFSET + 1;
    if (len % LEN_BYTE_SIZE)
    {
      frumem[PROD_INFO_CKSUM_OFFSET] = len / LEN_BYTE_SIZE + 1;
      i += LEN_BYTE_SIZE - (len % LEN_BYTE_SIZE);
    }
    else
    {
      frumem[PROD_INFO_CKSUM_OFFSET] = len / LEN_BYTE_SIZE;
    }

    for (int j = PROD_INFO_AREA_OFFSET; j < i; j++)
    {
      frumem[i] += frumem[j];
    }

    frumem[i] = ZERO_CKSUM_CONST - frumem[i];

    free_fruid_info(&frupsu);
    return;
  }
  else if (fruid->format == FRU_FB_FORMAT)
  {
    int i = 0x10;
    char vbuf[5] = {0};
    char fruid_file_name[] = "fruid_1.0";
    struct wedge_eeprom_st eeprom;
    // Will keep data to buffer in IPMI FRU format
    fruid_common_hdr_t *chdr = (fruid_common_hdr_t *)frumem;
    memset(&fruid, WEDGE400_FRUID_SIZE, zero_val);

    chdr->ver = COMMON_HDR_VER;
    chdr->prod_info_area_offset = PROD_INFO_AREA_OFFSET / LEN_BYTE_SIZE;
    chdr->cksum = chdr->ver + chdr->prod_info_area_offset;
    chdr->cksum = ZERO_CKSUM_CONST - chdr->cksum;

    rc = wedge_eeprom_parse(filename, &eeprom);
    if (rc)
    {
      OBMC_WARN(
             "populate_fruid: wedge_eeprom_parse on fruid %d returns %d\n",
             id, rc);
      return;
    }

    frumem[i++] = PROD_INFO_VER;
    frumem[i++] = 0x00;
    frumem[i++] = LANG_CODE_ENGLISH;
    _APPEND_STR_VALUE(eeprom.fbw_system_manufacturer);
    _APPEND_STR_VALUE(eeprom.fbw_product_name);
    _APPEND_STR_VALUE(eeprom.fbw_product_number);
    snprintf(vbuf, sizeof(vbuf), "%0X", eeprom.fbw_product_version);
    _APPEND_STR_VALUE(vbuf);
    _APPEND_STR_VALUE(eeprom.fbw_product_serial);
    _APPEND_STR_VALUE(eeprom.fbw_product_asset);
    _APPEND_STR_VALUE(fruid_file_name);
    frumem[i++] = TYPE_LAST;

    len = i - PROD_INFO_AREA_OFFSET + 1;
    if (len % LEN_BYTE_SIZE)
    {
      frumem[PROD_INFO_CKSUM_OFFSET] = len / LEN_BYTE_SIZE + 1;
      i += LEN_BYTE_SIZE - (len % LEN_BYTE_SIZE);
    }
    else
    {
      frumem[PROD_INFO_CKSUM_OFFSET] = len / LEN_BYTE_SIZE;
    }

    for (int j = PROD_INFO_AREA_OFFSET; j < i; j++)
    {
      frumem[i] += frumem[j];
    }

    frumem[i] = ZERO_CKSUM_CONST - frumem[i];

    return;
  }
  else
  {
    OBMC_WARN("populate_fruid: fruid %d not define format %d\n", id,
           fruid->format);
    return;
  }
}

int plat_fruid_size(unsigned char fru_id)
{
  struct fruid_dev *frudev = get_frudev(fru_id);
  if (!frudev)
    return 0;

  populate_fruid(fru_id);
  return WEDGE400_FRUID_SIZE;
}

int plat_fruid_data(unsigned char fru_id, int offset, int count,
                    unsigned char *data)
{

  struct fruid_dev *frudev = get_frudev(fru_id);
  if (!frudev)
    return -1;
  unsigned char *frumem = frudev->mem;

  if ((offset + count) > WEDGE400_FRUID_SIZE)
  {
    return -1;
  }

  memcpy(data, &(frumem[offset]), count);
  return 0;
}

int plat_fruid_init(void)
{
  for (unsigned char id = 0; id < WEDGE400_FRU_NUM; id++)
  {
    populate_fruid(fruid_list[id].id);
  }
  return 0;
}
