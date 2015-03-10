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

#include "../fruid.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <facebook/wedge_eeprom.h>

#define WEDGE_FRUID_SIZE 0x100

#define COMMON_HDR_VER  1
#define PROD_INFO_VER  1

#define PROD_INFO_AREA_OFFSET 0x10
#define PROD_INFO_CKSUM_OFFSET (PROD_INFO_AREA_OFFSET + 1)
#define LANG_CODE_ENGLISH 25
#define TYPE_STR 0xC0
#define TYPE_LAST 0xC1
#define ZERO_CKSUM_CONST 0x100
#define LEN_BYTE_SIZE 8

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

// Global structures
static unsigned char g_fruid[WEDGE_FRUID_SIZE] = {0};

static void
populate_fruid(void) {

  memset(&g_fruid, sizeof(g_fruid), 0);

  fruid_common_hdr_t *chdr = g_fruid;

  // Set Common Header version
  chdr->ver = COMMON_HDR_VER;

  // Product Info Area offset in multiples of 8 bytes
  chdr->prod_info_area_offset = PROD_INFO_AREA_OFFSET/LEN_BYTE_SIZE;

  // Calculate zero checksum
  chdr->cksum = chdr->ver + chdr->prod_info_area_offset;
  chdr->cksum = ZERO_CKSUM_CONST - chdr->cksum;

  // Retrieve Wedge EEPROM content
  struct wedge_eeprom_st eeprom;
  int rc = 0;
  rc = wedge_eeprom_parse(NULL, &eeprom);
  if (rc)
  {
    syslog(LOG_ALERT, "populate_fruid: wedge_eeprom_parse returns %d\n", rc);
    return;
  }

  // Start index at beginning of product info area
  int i = PROD_INFO_AREA_OFFSET;
  g_fruid[i++] = PROD_INFO_VER;
  g_fruid[i++] = 0x00; // prod area length; filled at end
  g_fruid[i++] = LANG_CODE_ENGLISH;

#define _APPEND_STR_VALUE(name) do { \
      if (sizeof(name) < 1 || i + 1 + sizeof(name) >= sizeof(g_fruid)) { \
                break; \
            } \
      g_fruid[i++] = TYPE_STR + sizeof(name); \
      memcpy(&g_fruid[i], name, sizeof(name)); \
      i += sizeof(name); \
} while(0)

  // Fill system manufacturer field

  _APPEND_STR_VALUE(eeprom.fbw_system_manufacturer);

  // Fill product name field
  _APPEND_STR_VALUE(eeprom.fbw_product_name);

  // Fill product number field
  _APPEND_STR_VALUE(eeprom.fbw_product_number);

  // convert product version to string and fill
  char vbuf[5] = {0};
  snprintf(vbuf, sizeof(vbuf), "%0X", eeprom.fbw_product_version);
  _APPEND_STR_VALUE(vbuf);

  // Fill product serial number field
  _APPEND_STR_VALUE(eeprom.fbw_product_serial);

  // Fill product asset tag field
  _APPEND_STR_VALUE(eeprom.fbw_product_asset);

  // Fill fruid file with dummy file name
  char fruid_file_name[] = "fruid_1.0";
  _APPEND_STR_VALUE(fruid_file_name);

  // Field to indicate the last entry
  g_fruid[i++] = TYPE_LAST;

  // length of the area in multiples of 8 bytes
  int len = i-PROD_INFO_AREA_OFFSET+1;
  if (len % LEN_BYTE_SIZE){
    // For non-multiple of 8 bytes, add one for partial data
    g_fruid[PROD_INFO_CKSUM_OFFSET] = len/LEN_BYTE_SIZE + 1;
    // And also increment index to keep checksum byte
    i += (len % LEN_BYTE_SIZE);
  } else {
    g_fruid[PROD_INFO_CKSUM_OFFSET] = len/LEN_BYTE_SIZE;
  }

  // Calculate zero checksum by adding all the values in product info area
  for (int j = PROD_INFO_AREA_OFFSET; j < i; j++) {
    g_fruid[i] += g_fruid[j];
  }

  // Calculate final cksum by subtraction
  g_fruid[i] = ZERO_CKSUM_CONST - g_fruid[i];

#undef _APPEND_STR_VALUE

  return;
}

// Access functions for FRUID
int
plat_fruid_size(void) {
  return WEDGE_FRUID_SIZE;
}

int
plat_fruid_data(int offset, int count, unsigned char *data) {
  // Check for the boundary condition
  if ((offset + count) > WEDGE_FRUID_SIZE) {
    return -1;
  }

  // Copy the FRUID content from the global structure
  memcpy(data, &(g_fruid[offset]), count);

  return 0;
}

// Initialize FRUID
int
plat_fruid_init(void) {

  // Populate FRUID global structure
  populate_fruid();

  return 0;
}
