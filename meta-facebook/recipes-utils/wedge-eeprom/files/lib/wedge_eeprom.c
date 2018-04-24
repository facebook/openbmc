/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#include "wedge_eeprom.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <openbmc/log.h>

#ifndef FBW_EEPROM_FILE
#define FBW_EEPROM_FILE "/sys/class/i2c-adapter/i2c-6/6-0050/eeprom"
#endif

#define FBW_EEPROM_VERSION0 0
#define FBW_EEPROM_V0_SIZE 162
#define FBW_EEPROM_VERSION1 1
#define FBW_EEPROM_V1_SIZE 174
#define FBW_EEPROM_VERSION2 2
#define FBW_EEPROM_V2_SIZE 176
#define FBW_EEPROM_VERSION3 3
#define FBW_EEPROM_V3_SIZE 196

/*
 * The eeprom size is 8K, we only use 157 bytes for v1 format.
 * Read 256 for now.
 */
#define FBW_EEPROM_SIZE 256


static inline uint8_t fbw_crc8(uint8_t crc, uint8_t data)
{
  /* donot verify crc now, always return 0 */
  return 0;
}

static uint8_t fbw_crc8_buf(const uint8_t *buf, int len)
{
  uint8_t crc = 0;
  int i;

  for (i = 0, crc = 0; i < len; i++) {
    crc = fbw_crc8(crc, buf[i]);
  }

  return crc;
}

static inline void fbw_copy_uint8(uint8_t *val, const uint8_t** src,
                                  int src_len)
{
  assert(src_len >= sizeof(*val));
  *val = **src;
  (*src) += src_len;
}

static inline void fbw_copy_uint16(uint16_t *val, const uint8_t** src,
                                  int src_len)
{
  assert(src_len >= sizeof(*val));
  *val = (**src) | ((*(*src + 1)) << 8);
  (*src) += src_len;
}

static inline void fbw_copy_uint32(uint32_t *val, const uint8_t** src,
                                  int src_len)
{
  assert(src_len >= sizeof(*val));
  *val = (**src)
    | ((*(*src + 1)) << 8)
    | ((*(*src + 2)) << 16)
    | ((*(*src + 3)) << 24);
  (*src) += src_len;
}

static inline void fbw_strcpy(char *dst, int dst_len,
                              const uint8_t **src, int src_len)
{
  assert(dst_len >= src_len + 1);    /* larger because of '\0' */
  strncpy(dst, (char *)*src, src_len);
  dst[src_len + 1] = '\0';
  (*src) += src_len;
}

static inline void fbw_copy_product_number(
    char *dst, int dst_len, const uint8_t **src, int src_len)
{
  int i;
  const uint8_t *cur = *src;
  /* 8 letter in the format of XX-XXXXXX, 2 additional letters */
  assert(dst_len >= src_len + 2);
  for (i = 0; i < 2; i++) {
    *dst++ = *cur++;
  }
  *dst++ = '-';
  for (i = 0; i < 6; i++) {
    *dst++ = *cur++;
  }
  *dst = '\0';
  (*src) += src_len;
}

static inline void fbw_copy_assembly_number(
    char *dst, int dst_len, const uint8_t **src, int src_len)
{
  int i;
  const uint8_t *cur = *src;
  /* 11 letter in the format of XXX-XXXXXX-XX, 3 additional letters */
  assert(dst_len >= src_len + 3);
  for (i = 0; i < 3; i++) {
    *dst++ = *cur++;
  }
  *dst++ = '-';
  for (i = 0; i < 6; i++) {
    *dst++ = *cur++;
  }
  *dst++ = '-';
  for (i = 0; i < 2; i++) {
    *dst++ = *cur++;
  }
  *dst = '\0';
  (*src) += src_len;
}

static inline void fbw_copy_facebook_pcb_part(
    char *dst, int dst_len, const uint8_t **src, int src_len)
{
  int i;
  const uint8_t *cur = *src;
  /* 11 letter in the format of XXX-XXXXXX-XX, 3 additional letters */
  assert(dst_len >= src_len + 3);
  for (i = 0; i < 3; i++) {
    *dst++ = *cur++;
  }
  *dst++ = '-';
  for (i = 0; i < 6; i++) {
    *dst++ = *cur++;
  }
  *dst++ = '-';
  for (i = 0; i < 2; i++) {
    *dst++ = *cur++;
  }
  *dst = '\0';
  (*src) += src_len;
}

static inline void fbw_copy_date(
    char *dst, int dst_len, const uint8_t **src, int src_len)
{
  const uint8_t *cur = *src;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  /* mm-dd-yy in output */
  assert(dst_len >= 9);
  /* input is 4 bytes YY YY MM DD */
  assert(src_len >= 4);
  fbw_copy_uint16(&year, &cur, 2);
  fbw_copy_uint8(&month, &cur, 1);
  fbw_copy_uint8(&day, &cur, 1);
  snprintf(dst, dst_len, "%02d-%02d-%02d", month % 13, day % 32, year % 100);
  (*src) += src_len;
}

static inline uint8_t _a2v(const uint8_t *a)
{
  uint8_t v = *a;
  if ('0' <= v && v <= '9') {
    return v - '0';
  }
  if ('a' <= v && v <= 'z') {
    return v - 'a' + 10;
  }
  if ('A' <= v && v <= 'Z') {
    return v - 'A' + 10;
  }
  return 0;
}

static inline void fbw_copy_mac(
    uint8_t* dst, int dst_len, const uint8_t **src, int src_len)
{
  int i;
  const uint8_t *cur = *src;

  assert(dst_len >= 6);
  assert(src_len >= 12);

  for (i = 0; i < 6; i++) {
    *dst = (_a2v(cur) << 4) | _a2v(cur + 1);
    dst++;
    cur +=2 ;
  }
  (*src) += src_len;
}

static int fbw_parse_buffer(
    const uint8_t *buf, int len, struct wedge_eeprom_st *eeprom) {
  int rc = 0;
  const uint8_t* cur = buf;
  uint16_t magic;
  int crc_len;
  uint8_t crc8;

  memset(eeprom, 0, sizeof(*eeprom));

  /* make sure the magic number */
  fbw_copy_uint16(&magic, &cur, FBW_EEPROM_F_MAGIC);
  if (magic != 0xfbfb) {
    rc = EFAULT;
    LOG_ERR(rc, "Unexpected magic word 0x%x", magic);
    goto out;
  }

  /* confirm the version number, only version is supported */
  fbw_copy_uint8(&eeprom->fbw_version, &cur, FBW_EEPROM_F_VERSION);
  if ((eeprom->fbw_version != FBW_EEPROM_VERSION0) &&
      (eeprom->fbw_version != FBW_EEPROM_VERSION1) &&
      (eeprom->fbw_version != FBW_EEPROM_VERSION2) &&
      (eeprom->fbw_version != FBW_EEPROM_VERSION3)) {
    rc = EFAULT;
    LOG_ERR(rc, "Unsupported version number %u", eeprom->fbw_version);
    goto out;
  } else {
    if (eeprom->fbw_version == FBW_EEPROM_VERSION0) {
      crc_len = FBW_EEPROM_V0_SIZE;
    } else if (eeprom->fbw_version == FBW_EEPROM_VERSION1) {
      crc_len = FBW_EEPROM_V1_SIZE;
    } else if (eeprom->fbw_version == FBW_EEPROM_VERSION2) {
      crc_len = FBW_EEPROM_V2_SIZE;
    } else if (eeprom->fbw_version == FBW_EEPROM_VERSION3) {
      crc_len = FBW_EEPROM_V3_SIZE;
    }
    assert(crc_len <= len);
  }

  /* check CRC */
  crc8 = fbw_crc8_buf(buf, crc_len);
  if (crc8 != 0) {
    rc = EFAULT;
    LOG_ERR(rc, "CRC check failed");
    goto out;
  }

  /* Product name: ASCII for 12 characters */
  fbw_strcpy(eeprom->fbw_product_name,
             sizeof(eeprom->fbw_product_name),
             &cur,
             (eeprom->fbw_version >= FBW_EEPROM_VERSION3)
             ? FBW_EEPROM_F_PRODUCT_NAME_V3
             : FBW_EEPROM_F_PRODUCT_NAME);

  /* Product Part #: 8 byte data shown as XX-XXXXXXX */
  fbw_copy_product_number(eeprom->fbw_product_number,
                          sizeof(eeprom->fbw_product_number),
                          &cur, FBW_EEPROM_F_PRODUCT_NUMBER);

  /* System Assembly Part Number: XXX-XXXXXX-XX */
  fbw_copy_assembly_number(eeprom->fbw_assembly_number,
                           sizeof(eeprom->fbw_assembly_number),
                           &cur, FBW_EEPROM_F_ASSEMBLY_NUMBER);

  /* Facebook PCBA Part Number: XXX-XXXXXXX-XX */
  fbw_copy_facebook_pcb_part(eeprom->fbw_facebook_pcba_number,
                             sizeof(eeprom->fbw_facebook_pcba_number),
                             &cur, FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER);

  /* Facebook PCBA Part Number: XXX-XXXXXXX-XX */
  if (eeprom->fbw_version >= FBW_EEPROM_VERSION1) {
    fbw_copy_facebook_pcb_part(eeprom->fbw_facebook_pcb_number,
                               sizeof(eeprom->fbw_facebook_pcb_number),
                               &cur, FBW_EEPROM_F_FACEBOOK_PCB_NUMBER);
  }

  /* ODM PCBA Part Number: XXXXXXXXXXXX */
  fbw_strcpy(eeprom->fbw_odm_pcba_number,
             sizeof(eeprom->fbw_odm_pcba_number),
             &cur, FBW_EEPROM_F_ODM_PCBA_NUMBER);

  /* ODM PCBA Serial Number: XXXXXXXXXXXX */
  fbw_strcpy(eeprom->fbw_odm_pcba_serial,
             sizeof(eeprom->fbw_odm_pcba_serial),
             &cur,
             (eeprom->fbw_version >= FBW_EEPROM_VERSION2)
             ? FBW_EEPROM_F_ODM_PCBA_SERIAL_V2
             : FBW_EEPROM_F_ODM_PCBA_SERIAL);

  /* Product Production State */
  fbw_copy_uint8(&eeprom->fbw_production_state,
                 &cur, FBW_EEPROM_F_PRODUCT_STATE);

  /* Product Version */
  fbw_copy_uint8(&eeprom->fbw_product_version,
                 &cur, FBW_EEPROM_F_PRODUCT_VERSION);

  /* Product Sub Version */
  fbw_copy_uint8(&eeprom->fbw_product_subversion,
                 &cur, FBW_EEPROM_F_PRODUCT_SUBVERSION);

  /* Product Serial Number: XXXXXXXX */
  fbw_strcpy(eeprom->fbw_product_serial,
             sizeof(eeprom->fbw_product_serial),
             &cur,
             (eeprom->fbw_version >= FBW_EEPROM_VERSION2)
             ? FBW_EEPROM_F_PRODUCT_SERIAL_V2
             : FBW_EEPROM_F_PRODUCT_SERIAL);

  /* Product Assert Tag: XXXXXXXX */
  fbw_strcpy(eeprom->fbw_product_asset,
             sizeof(eeprom->fbw_product_asset),
             &cur, FBW_EEPROM_F_PRODUCT_ASSET);

  /* System Manufacturer: XXXXXXXX */
  fbw_strcpy(eeprom->fbw_system_manufacturer,
             sizeof(eeprom->fbw_system_manufacturer),
             &cur, FBW_EEPROM_F_SYSTEM_MANUFACTURER);

  /* System Manufacturing Date: mm-dd-yy */
  fbw_copy_date(eeprom->fbw_system_manufacturing_date,
                sizeof(eeprom->fbw_system_manufacturing_date),
                &cur, FBW_EEPROM_F_SYSTEM_MANU_DATE);

  /* PCB Manufacturer: XXXXXXXXX */
  fbw_strcpy(eeprom->fbw_pcb_manufacturer,
             sizeof(eeprom->fbw_pcb_manufacturer),
             &cur, FBW_EEPROM_F_PCB_MANUFACTURER);

  /* Assembled At: XXXXXXXX */
  fbw_strcpy(eeprom->fbw_assembled,
             sizeof(eeprom->fbw_assembled),
             &cur, FBW_EEPROM_F_ASSEMBLED);

  /* Local MAC Address */
  fbw_copy_mac(eeprom->fbw_local_mac,
               sizeof(eeprom->fbw_local_mac),
               &cur, FBW_EEPROM_F_LOCAL_MAC);

  /* Extended MAC Address */
  fbw_copy_mac(eeprom->fbw_mac_base,
               sizeof(eeprom->fbw_mac_base),
               &cur, FBW_EEPROM_F_EXT_MAC_BASE);

  /* Extended MAC Address Size */
  fbw_copy_uint16(&eeprom->fbw_mac_size,
                  &cur,FBW_EEPROM_F_EXT_MAC_SIZE);

  /* Location on Fabric: "LEFT"/"RIGHT", "WEDGE", "LC" */
  fbw_strcpy(eeprom->fbw_location,
             sizeof(eeprom->fbw_location),
             &cur, 
             (eeprom->fbw_version >= FBW_EEPROM_VERSION3)
             ? FBW_EEPROM_F_LOCATION_V3
             : FBW_EEPROM_F_LOCATION);

  /* CRC8 */
  fbw_copy_uint8(&eeprom->fbw_crc8,
                 &cur, FBW_EEPROM_F_CRC8);

  assert((cur - buf) <= len);

 out:
  return rc;
}

int wedge_eeprom_parse(const char *fn, struct wedge_eeprom_st *eeprom)
{
  int rc = 0;
  uint32_t len;
  FILE *fin;
  char buf[FBW_EEPROM_SIZE];

  if (!eeprom) {
    return -EINVAL;
  }

  if (!fn) {
    fn = FBW_EEPROM_FILE;
  }

  fin = fopen(fn, "r");
  if (fin == NULL) {
    rc = errno;
    LOG_ERR(rc, "Failed to open %s", FBW_EEPROM_FILE);
    goto out;
  }

  /* check the file size */
  rc = fseek(fin, 0, SEEK_END);
  if (rc) {
    rc = errno;
    LOG_ERR(rc, "Failed to seek to the end of %s", FBW_EEPROM_FILE);
    goto out;
  }

  len = ftell(fin);
  if (len < FBW_EEPROM_SIZE) {
    rc = ENOSPC;
    LOG_ERR(rc, "File '%s' is too small (%u < %u)", FBW_EEPROM_FILE,
            len, FBW_EEPROM_SIZE);
    goto out;
  }

  /* go back to the beginning of the file */
  rewind(fin);

  rc = fread(buf, 1, sizeof(buf), fin);
  if (rc < sizeof(buf)) {
    LOG_ERR(ENOSPC, "Failed to complete the read. Only got %d", rc);
    rc = ENOSPC;
    goto out;
  }

  rc = fbw_parse_buffer((const uint8_t *)buf, sizeof(buf), eeprom);
  if (rc) {
    goto out;
  }

 out:
  if (fin) {
    fclose(fin);
  }

  return -rc;
}
