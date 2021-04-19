/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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
 * Elbert specific FRUID related routine.
 */

#include "fruid.h"
#include <errno.h>
#include <facebook/elbert_eeprom.h>
#include <facebook/wedge_eeprom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define COMMON_HDR_VER 1
#define PROD_INFO_VER 1
#define PROD_INFO_AREA_OFFSET 0x10
#define PROD_INFO_CKSUM_OFFSET (PROD_INFO_AREA_OFFSET + 1)
#define LANG_CODE_ENGLISH 25
#define TYPE_STR 0xC0
#define TYPE_LAST 0xC1
#define ZERO_CKSUM_CONST 0x100
#define LEN_BYTE_SIZE 8

#define ELBERT_FRUID_SIZE 0x100

// FRU IDs for ELBERT
#define FRUID_ALL 0
#define FRUID_CHASSIS 16
#define FRUID_BMC 17
#define FRUID_SCM 1
// Switch card has two EEPROMs:
// 1. SMB is on the switch card with TH4
// 2. SMB_EXTRA is on the base card
#define FRUID_SMB 2
#define FRUID_SMB_EXTRA 18
#define FRUID_PIM_MIN 3
#define FRUID_PIM_MAX 10

// Target for elbert_eeprom API
#define ELBERT_CHASSIS "CHASSIS"
#define ELBERT_SCM "SCM"
#define ELBERT_PIM "PIM%d"
#define ELBERT_SMB "SMB"
#define ELBERT_SMB_EXTRA "SMB_EXTRA"
#define ELBERT_BMC "BMC"

typedef struct _fruid_common_hdr_t {
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
#define _APPEND_STR_VALUE(name)                                     \
  do {                                                              \
    if (sizeof(name) < 1 || i + 1 + sizeof(name) >= g_fruid_size) { \
      break;                                                        \
    }                                                               \
    g_fruid[i++] = TYPE_STR + sizeof(name);                         \
    memcpy(&g_fruid[i], name, sizeof(name));                        \
    i += sizeof(name);                                              \
  } while (0)

// Main function to populate the fruid data structure, using the data
// from the given target's eeprom
static void populate_fruid(
    const char* target,
    unsigned char* g_fruid,
    const int g_fruid_size) {
  struct wedge_eeprom_st eeprom;
  unsigned char zero_val = 0x0;
  int i = PROD_INFO_AREA_OFFSET;
  int rc = 0;
  int len = 0;

  fruid_common_hdr_t* chdr = (fruid_common_hdr_t*)g_fruid;
  memset(&g_fruid, g_fruid_size, zero_val);

  chdr->ver = COMMON_HDR_VER;
  chdr->prod_info_area_offset = PROD_INFO_AREA_OFFSET / LEN_BYTE_SIZE;
  chdr->cksum = chdr->ver + chdr->prod_info_area_offset;
  chdr->cksum = ZERO_CKSUM_CONST - chdr->cksum;
  rc = elbert_eeprom_parse(target, &eeprom);
  if (rc) {
    syslog(
        LOG_ALERT,
        "populate_fruid %s: elbert_eeprom_parse returns %d\n",
        target,
        rc);
    return;
  }

  g_fruid[i++] = PROD_INFO_VER;
  g_fruid[i++] = 0x00;
  g_fruid[i++] = LANG_CODE_ENGLISH;
  _APPEND_STR_VALUE(eeprom.fbw_system_manufacturer);
  _APPEND_STR_VALUE(eeprom.fbw_product_name);
  _APPEND_STR_VALUE(eeprom.fbw_product_number);
  char vbuf[8] = {0};
  snprintf(
      vbuf,
      sizeof(vbuf),
      "%d.%d",
      eeprom.fbw_product_version,
      eeprom.fbw_product_subversion);
  _APPEND_STR_VALUE(vbuf);
  _APPEND_STR_VALUE(eeprom.fbw_product_serial);
  _APPEND_STR_VALUE(eeprom.fbw_product_asset);
  char fruid_file_name[] = "fruid_1.0";
  _APPEND_STR_VALUE(fruid_file_name);
  g_fruid[i++] = TYPE_LAST;

  len = i - PROD_INFO_AREA_OFFSET + 1;
  if (len % LEN_BYTE_SIZE) {
    g_fruid[PROD_INFO_CKSUM_OFFSET] = len / LEN_BYTE_SIZE + 1;
    i += LEN_BYTE_SIZE - (len % LEN_BYTE_SIZE);
  } else {
    g_fruid[PROD_INFO_CKSUM_OFFSET] = len / LEN_BYTE_SIZE;
  }

  for (int j = PROD_INFO_AREA_OFFSET; j < i; j++) {
    g_fruid[i] += g_fruid[j];
  }
  g_fruid[i] = ZERO_CKSUM_CONST - g_fruid[i];

  return;
}

int plat_fruid_size(unsigned char payload_id) {
  return ELBERT_FRUID_SIZE;
}

int plat_fruid_data(
    unsigned char payload_id,
    int fru_id,
    int offset,
    int count,
    unsigned char* data) {
  unsigned char g_fruid[ELBERT_FRUID_SIZE] = {0};
  char target[10] = {0};

  if ((offset + count) > ELBERT_FRUID_SIZE) {
    syslog(
        LOG_ALERT, "plat_fruid_data: requested offset exceeds eeprom size\n");
    return -1;
  }

  if (fru_id == FRUID_ALL || fru_id == FRUID_CHASSIS) {
    sprintf(target, ELBERT_CHASSIS);
  } else if (fru_id == FRUID_SCM) {
    sprintf(target, ELBERT_SCM);
  } else if (FRUID_PIM_MIN <= fru_id && fru_id <= FRUID_PIM_MAX) {
    sprintf(target, ELBERT_PIM, fru_id - 1);
  } else if (fru_id == FRUID_SMB) {
    sprintf(target, ELBERT_SMB);
  } else if (fru_id == FRUID_SMB_EXTRA) {
    sprintf(target, ELBERT_SMB_EXTRA);
  } else if (fru_id == FRUID_BMC) {
    sprintf(target, ELBERT_BMC);
  } else {
    syslog(LOG_ALERT, "plat_fruid_data: unsupported fru_id %d\n", fru_id);
    return -1;
  }

  populate_fruid(target, g_fruid, sizeof(g_fruid));
  memcpy(data, &(g_fruid[offset]), count);
  return 0;
}

// Initialize FRUID
int plat_fruid_init(void) {
  return 0;
}
