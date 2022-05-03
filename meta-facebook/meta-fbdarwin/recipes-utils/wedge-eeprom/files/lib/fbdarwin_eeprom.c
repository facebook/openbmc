/*
 * Copyright 2022-present Facebook. All Rights Reserved.
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

#include "fbdarwin_eeprom.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <openbmc/log.h>

// FBDARWIN definition
#define FBDARWIN_EEPROM_PATH_BASE       "/sys/bus/i2c/drivers/at24/"
#define FBDARWIN_EEPROM_BMC_OBJ         "BMC"
#define FBDARWIN_EEPROM_BMC             "0-0050"
#define FBDARWIN_EEPROM_CHASSIS_OBJ     "CHASSIS"
#define FBDARWIN_EEPROM_CHASSIS_PATH    "/mnt/data1/.chassis_eeprom"

#define FBDARWIN_EEPROM_SIZE            256
#define FBDARWIN_EEPROM_FORMAT_V2       "0002"
#define FBDARWIN_EEPROM_HEADER_B1       0x0
#define FBDARWIN_EEPROM_HEADER_B2       0x0
#define FBDARWIN_EEPROM_HEADER_B3       0x0
#define FBDARWIN_EEPROM_HEADER_B4       0x3
#define FBDARWIN_EEPROM_TYPE_SIZE       4
#define FBDARWIN_EEPROM_PCA_SIZE        12
#define FBDARWIN_EEPROM_SERIAL_SIZE     11
#define FBDARWIN_EEPROM_KVN_SIZE        3
#define FBDARWIN_EEPROM_FIELD_END       0x00
#define FBDARWIN_EEPROM_FIELD_MFGTIME   0x02
#define FBDARWIN_EEPROM_FIELD_SKU       0x03
#define FBDARWIN_EEPROM_FIELD_ASY       0x04
#define FBDARWIN_EEPROM_FIELD_MACBASE   0x05
#define FBDARWIN_EEPROM_FIELD_FLDVAR    0x09
#define FBDARWIN_EEPROM_FIELD_HWREV     0x0B
#define FBDARWIN_EEPROM_FIELD_SERIAL    0x0E
#define FBDARWIN_EEPROM_FIELD_MFGTIME2  0x17

int fbdarwin_htoi(char a)
{
  if ((a >= 'a') && (a <= 'f'))
    return a - 'a' + 10;
  else if ((a >= 'A') && (a <= 'F'))
    return a - 'A' + 10;
  else if ((a >= '0') && (a <= '9'))
    return a - '0';

  // Error. return -1
  return -1;
}
void fbdarwin_parse_str(char *dest, char *src, int location, int len)
{
  int i = 0;
  for (i = 0; i < len; i++)
    dest[i] = src[i + location];
  dest[len] = '\0';
}

void fbdarwin_parse_mac(uint8_t *dest, char *src, int location)
{
  int i = 0;
  int upper_val = 0;
  int lower_val = 0;
  for (i = 0; i < 6; i++) {
      upper_val = fbdarwin_htoi(src[3*i]);
      lower_val = fbdarwin_htoi(src[3*i + 1]);
      /* If invalid character is found, return ffffffffff */
      if ((upper_val < 0) || (lower_val < 0)) {
        for (i = 0; i < 6; i++)
          dest[i] = 0xff;
        return;
      }
      dest[i] = upper_val * 16 + lower_val;
  }
  return;
}

/* Assumes that offset is in the range of [-255 , 255] */
void fbdarwin_calculate_mac(uint8_t *base, int offset, uint8_t *result)
{
  int i = 0;
  int overflow = 0;
  int field_result = 0;
  for (i = 0; i < 6; i++)
    result[i] = base[i];
  for (i = 0; i < 6; i++)
  {
    overflow = 0;
    field_result = result[5-i] + offset;
    if (field_result >= 256) {
      overflow = 1;
      field_result -= 256;
    }
    if (field_result < 0) {
      overflow = -1;
      field_result += 256;
    }
    offset = overflow;
    result[5-i] = field_result%256;
  }
  return;
}

void fbdarwin_str_toupper(char *str)
{
  if (str != NULL)
    for (int i = 0; i < strlen(str); i++)
      str[i] = toupper(str[i]);
  return;
}


void fbdarwin_parse_string(int *read_pointer, char *dest,
                       char *buf, int len)
{
  memcpy(dest, &buf[*read_pointer], len);
  dest[len] = '\0';
  *read_pointer += len;
}

int fbdarwin_parse_hexadecimal(int *read_pointer,
                           unsigned int *dest,
                           char *buf,
                           int len)
{
  int rc = 0;
  int parsed_digit = 0;
  int marker = 0;
  *dest = 0;
  while ((rc == 0) && (marker < len)) {
    *dest = *dest * 16;
    parsed_digit = fbdarwin_htoi(buf[*read_pointer + marker]);
    if (parsed_digit < 0)
      rc = -1;
    else
      *dest = *dest + parsed_digit;
    marker++;
  }

  *read_pointer += len;
  return rc;
}

/*
 * FBDARWIN EEPROM has different format and field names from FB standard
 * This function will read the eeprom, and try to map the FBDARWIN fields
 * to the FB standard fields
 */
int fbdarwin_eeprom_parse(const char *target, struct wedge_eeprom_st *eeprom)
{
  int rc = 0;
  int read_pointer = 0;
  FILE *fin;
  int field_type;
  int field_len;
  int ver_major = 0;
  int ver_minor = 0;
  int dot_location = 0;
  int sku_length = FBW_EEPROM_F_PRODUCT_NUMBER + 1; // FBDarwin uses 9 char fields
  char local_target[256];
  char field_value[FBDARWIN_EEPROM_SIZE]; // Will never overflow
  char fn[64];
  char buf[FBDARWIN_EEPROM_SIZE];
  bool mfgtime2 = false; // set to True if MFGTIME2 field is detected
  int x;

  if (!eeprom) {
    return -EINVAL;
  }

  if (!target)
    return -EINVAL;

  x = 0;
  snprintf(local_target, sizeof(local_target), "%s", target);
  fbdarwin_str_toupper((char *)local_target);

  if (!strcmp(local_target, FBDARWIN_EEPROM_BMC_OBJ)) {
    sprintf(fn, "%s%s/eeprom", FBDARWIN_EEPROM_PATH_BASE,
                               FBDARWIN_EEPROM_BMC);
    sku_length = 8; // BMC field is only 8 chars long
  } else if (!strcmp(local_target, FBDARWIN_EEPROM_CHASSIS_OBJ)) {
    sprintf(fn, "%s", FBDARWIN_EEPROM_CHASSIS_PATH );
  } else {
    return -EINVAL;
  }

  fin = fopen(fn, "r");
  if (fin == NULL) {
    rc = -EINVAL;
    OBMC_ERROR(rc, "Failed to open %s", fn);
    goto out;
  }

  /* Read the file into buffer */
  rc = fread(buf, 1, sizeof(buf), fin);
  if (rc <= 0) {
    OBMC_ERROR(ENOSPC, "Failed to complete the read. Error code %d", rc);
    rc = ENOSPC;
    goto out;
  }

  /*
   * There are many fields in FB standard eeprom, that doesn't exist in
   * FBDARWIN eeprom. Start with all fields filled up as NULL or 0x0,
   * then overwrite the values as we get the information from eeprom
   */
  memset(eeprom, 0, sizeof(struct wedge_eeprom_st));

   /* Check that the eeprom field format header is as expected */
   if ((buf[read_pointer] != FBDARWIN_EEPROM_HEADER_B1) ||
       (buf[read_pointer + 1] != FBDARWIN_EEPROM_HEADER_B2) ||
       (buf[read_pointer + 2] != FBDARWIN_EEPROM_HEADER_B3) ||
       (buf[read_pointer + 3] != FBDARWIN_EEPROM_HEADER_B4)) {
     OBMC_ERROR(EINVAL, "Wrong EEPROM header! Expected %02x %02x %02x %02x, \
         got %02x %02x %02x %02x",
         FBDARWIN_EEPROM_HEADER_B1, FBDARWIN_EEPROM_HEADER_B2,
         FBDARWIN_EEPROM_HEADER_B3, FBDARWIN_EEPROM_HEADER_B4,
         buf[read_pointer], buf[read_pointer +1],
         buf[read_pointer + 2], buf[read_pointer + 3]);
     rc = -EINVAL;
     goto out;
   }

   /* As we read the first four bytes, advance the read pointer */
   read_pointer += FBDARWIN_EEPROM_TYPE_SIZE;

   /* Bypass the eeprom size field */
   read_pointer += FBDARWIN_EEPROM_TYPE_SIZE;

   /* Check eeprom format */
   if ((buf[read_pointer] != FBDARWIN_EEPROM_FORMAT_V2[0]) ||
       (buf[read_pointer + 1] != FBDARWIN_EEPROM_FORMAT_V2[1]) ||
       (buf[read_pointer + 2] != FBDARWIN_EEPROM_FORMAT_V2[2]) ||
       (buf[read_pointer + 3] != FBDARWIN_EEPROM_FORMAT_V2[3])) {
     OBMC_ERROR(EINVAL, "Unsupported eeprom format! Expected %02x %02x %02x %02x, \
         got %02x %02x %02x %02x",
         FBDARWIN_EEPROM_FORMAT_V2[0], FBDARWIN_EEPROM_FORMAT_V2[1],
         FBDARWIN_EEPROM_FORMAT_V2[2], FBDARWIN_EEPROM_FORMAT_V2[3],
         buf[read_pointer], buf[read_pointer + 1],
         buf[read_pointer + 2], buf[read_pointer + 3]);
     rc = -EINVAL;
     goto out;
   }
   read_pointer += FBDARWIN_EEPROM_TYPE_SIZE;

  /* Parse PCA and serial. If there is additional serial TLV parsed,,
   * this value will be overwritten */
  fbdarwin_parse_string(&read_pointer, (char *)&eeprom->fbw_odm_pcba_number,
                    buf, FBDARWIN_EEPROM_PCA_SIZE);

  fbdarwin_parse_string(&read_pointer, (char *)&eeprom->fbw_product_serial,
                    buf, FBDARWIN_EEPROM_SERIAL_SIZE);
  read_pointer += FBDARWIN_EEPROM_KVN_SIZE;
  /* Product type is hardcoded to FBDARWIN */
  sprintf(eeprom->fbw_product_name, "FBDARWIN");

  rc = 0;
  /* FBDarwin Chassis option just has PCA/SN and no other fields */
  if (!strcmp(local_target, FBDARWIN_EEPROM_CHASSIS_OBJ))
     goto out;

  /* Now go though EEPROM contents to fill out the fields */
  while ((rc == 0) && (read_pointer < FBDARWIN_EEPROM_SIZE)) {
    rc = fbdarwin_parse_hexadecimal(&read_pointer, &field_type, buf, 2);
    if (rc < 0)
      goto out;
    rc = fbdarwin_parse_hexadecimal(&read_pointer, &field_len, buf, 4);
    if (rc < 0)
      goto out;
    /*
     * If this is the end of EEPROM (specified as "END" TLV),
     * do not read any value, but just bail out
     */
    if (field_type == FBDARWIN_EEPROM_FIELD_END)
      break;
    /* Otherwise, read the field */
    /* Clear field_value from previous reads */
    memset(field_value, 0, sizeof(field_value));
    memcpy(field_value, &buf[read_pointer], field_len);
    read_pointer += field_len;

    /* Now, map the value into similar field in FB EEPROM */
    switch(field_type)
    {
      case FBDARWIN_EEPROM_FIELD_MFGTIME:
          if (!mfgtime2)
            memcpy(eeprom->fbw_system_manufacturing_date, field_value, 10);
          break;

      case FBDARWIN_EEPROM_FIELD_MFGTIME2:
          mfgtime2 = true; // Do not allow MFGTIME to override MFGTIME2
          memcpy(eeprom->fbw_system_manufacturing_date, field_value, 10);
          break;

      case FBDARWIN_EEPROM_FIELD_ASY:
          memcpy(eeprom->fbw_assembly_number, field_value,
              FBW_EEPROM_F_ASSEMBLY_NUMBER);
          // We allocate FBW_EEPROM_F_ASSEMBLY_NUMBER + 3
          eeprom->fbw_assembly_number[FBW_EEPROM_F_ASSEMBLY_NUMBER + 2] = '\0';

          // Set production state based on ASY:
          // Production = 1, Prototype  = 0
          if (isalpha(eeprom->fbw_assembly_number[FBW_EEPROM_F_ASSEMBLY_NUMBER - 2]) ||
              isalpha(eeprom->fbw_assembly_number[FBW_EEPROM_F_ASSEMBLY_NUMBER - 1]))
            eeprom->fbw_production_state = 1;
          else
            eeprom->fbw_production_state = 0;
          break;

      case FBDARWIN_EEPROM_FIELD_SKU:
          memcpy(eeprom->fbw_product_number, field_value, sku_length);
          // This cuts off the Product field into 9 characters max
          // We allocate FBW_EEPROM_F_PRODUCT_NUMBER + 2
          eeprom->fbw_product_number[FBW_EEPROM_F_PRODUCT_NUMBER + 1] = '\0';
          break;

      case FBDARWIN_EEPROM_FIELD_MACBASE:
          for (int i = 0; i < 6; i++)
            eeprom->fbw_mac_base[i] = fbdarwin_htoi(field_value[2*i]) * 16 +
                                      fbdarwin_htoi(field_value[2*i + 1]);
          /* Also hardcade mac size as the same value as Minipack */
          eeprom->fbw_mac_size = 139;
          break;

      case FBDARWIN_EEPROM_FIELD_HWREV:
          /* Scan for period character */
          dot_location = field_len;
          for (int i = 0; i < field_len; i++)
            if (field_value[i] == '.')
              dot_location = i;

          /* Parse the number before dot, and fit it into uint8_t */
          for (int i = 0; i < dot_location; i++) {
            ver_major *= 10;
            ver_major += fbdarwin_htoi(field_value[i]);
          }
          /* Parse the number after dot, and fit it into uint8_t */
          ver_major = ver_major % 256;
          for (int i = dot_location + 1; i < field_len; i++) {
            ver_minor *= 10;
            ver_minor += fbdarwin_htoi(field_value[i]);
          }
          /* Fit the value into uint8_t */
          ver_minor = ver_minor % 256;
          eeprom->fbw_product_version = ver_major;
          eeprom->fbw_product_subversion = ver_minor;
          break;

      case FBDARWIN_EEPROM_FIELD_SERIAL:
          memcpy(&eeprom->fbw_product_serial, field_value,
              FBDARWIN_EEPROM_SERIAL_SIZE);
          eeprom->fbw_product_serial[FBDARWIN_EEPROM_SERIAL_SIZE] = '\0';
          break;

      default:
          /* Unknown field */
          break;
    }
  }

 out:
  if (fin) {
    fclose(fin);
  }

  // rc > 0 means success here.
  // (it will usually have the number of bytes read)
  return (rc >= 0) ? 0 : -rc;
}
