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

#include "yamp_eeprom.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <openbmc/log.h>

// YAMP definition
#define YAMP_EEPROM_PATH_BASE "/sys/bus/i2c/drivers/at24/"
#define YAMP_EEPROM_CHS_OBJ "CHASSIS"
#define YAMP_EEPROM_SCD_OBJ "SCD"
#define YAMP_EEPROM_SUP_OBJ "SUP"
#define YAMP_EEPROM_LC_OBJ "LC"
#define YAMP_EEPROM_CHASSIS "13-0052"
#define YAMP_EEPROM_SWITCH_CARD "4-0050"
#define YAMP_EEPROM_SIZE_MAX 255
#define YAMP_EEPROM_HEADER_B1 0x0
#define YAMP_EEPROM_HEADER_B2 0x0
#define YAMP_EEPROM_HEADER_B3 0x0
#define YAMP_EEPROM_HEADER_B4 0x3
#define YAMP_EEPROM_TYPE_SIZE 4
#define YAMP_EEPROM_PCA_SIZE 12
#define YAMP_EEPROM_SERIAL_SIZE 11
#define YAMP_EEPROM_KVN_SIZE 3
#define YAMP_EEPROM_FIELD_END          0x00
#define YAMP_EEPROM_FIELD_RESERVED1    0x01
#define YAMP_EEPROM_FIELD_MFGTIME      0x02
#define YAMP_EEPROM_FIELD_SKU          0x03
#define YAMP_EEPROM_FIELD_ASY          0x04
#define YAMP_EEPROM_FIELD_MACBASE      0x05
#define YAMP_EEPROM_FIELD_RESERVED2    0x06
#define YAMP_EEPROM_FIELD_RESERVED3    0x07
#define YAMP_EEPROM_FIELD_RESERVED4    0x08
#define YAMP_EEPROM_FIELD_FLDVAR       0x09
#define YAMP_EEPROM_FIELD_HWAPI        0x0A
#define YAMP_EEPROM_FIELD_HWREV        0x0B
#define YAMP_EEPROM_FIELD_SID          0x0C
#define YAMP_EEPROM_FIELD_PCA          0x0D
#define YAMP_EEPROM_FIELD_SERIAL       0x0E
#define YAMP_EEPROM_FIELD_RESERVED5    0x0F
#define YAMP_EEPROM_FIELD_RESERVED6    0x10
#define YAMP_EEPROM_FIELD_RESERVED7    0x11
#define YAMP_EEPROM_SIZE 256
#define YAMP_LC_AT24_BUS_BASE 14
#define YAMP_LC_AT24_SLAVE_ADDR 0x50
#define YAMP_SUP_EEPROM_OFFSET 0x2000
#define YAMP_TEMP_PATH_BASE "/tmp"
#define YAMP_TEMP_FILE_NAME "sup_eeprom"
#define YAMP_RAW_FILE_NAME "sup_bios"
#define YAMP_CMD_BUF_SIZE 256
#define YAMP_BIOS_CMD "/usr/local/bin/bios_util.sh"
#define YAMP_FILE_NAME_SIZE 64

int yamp_htoi(char a)
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
void yamp_parse_str(char *dest, char *src, int location, int len)
{
  int i = 0;
  for (i = 0; i < len; i++)
    dest[i] = src[i + location];
  dest[len] = '\0';
}

void yamp_parse_mac(uint8_t *dest, char *src, int location)
{
  int i = 0;
  int upper_val = 0;
  int lower_val = 0;
  for (i = 0; i < 6; i++) {
      upper_val = yamp_htoi(src[3*i]);
      lower_val = yamp_htoi(src[3*i + 1]);
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
void yamp_calculate_mac(uint8_t *base, int offset, uint8_t *result)
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

void yamp_str_toupper(char *str)
{
  if (str != NULL)
    for (int i = 0; i < strlen(str); i++)
      str[i] = toupper(str[i]);
  return;
}


void yamp_parse_string(int *read_pointer, char *dest,
                       char *buf, int len)
{
  memcpy(dest, &buf[*read_pointer], len);
  dest[len] = '\0';
  *read_pointer += len;
}

int yamp_parse_hexadecimal(int *read_pointer,
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
    parsed_digit = yamp_htoi(buf[*read_pointer + marker]);
    if (parsed_digit < 0)
      rc = -1;
    else
      *dest = *dest + parsed_digit;
    marker++;
  }

  *read_pointer += len;
  return rc;
}

int yamp_get_lc_bus_name(const char *lc_name, char *bus_name)
{
  int lc_number = -1;

  // Boundary check
  if (!lc_name || !bus_name)
    return -1;

  // If lc_name is LCx format where x is a number between 1 and 8
  // translate this into lc_number (0-base)
  if (strlen(lc_name) == 3) {
    if ((lc_name[0] == 'L')&&(lc_name[1]=='C')) {
      if ((lc_name[2] >= '1')&&(lc_name[2] <= '8')) {
        lc_number = lc_name[2] - '1';  // it is 0-base
      }
    }
  }
  if (lc_number == -1)
    return -1;

  snprintf(bus_name, 12, "%2d-00%02x", lc_number + YAMP_LC_AT24_BUS_BASE,
          YAMP_LC_AT24_SLAVE_ADDR);

  return 0;
}

void yamp_get_eeprom_filename(char *fn)
{
  sprintf(fn, "%s/%s", YAMP_TEMP_PATH_BASE,
                       YAMP_TEMP_FILE_NAME);
  return;

}

void yamp_get_raw_filename(char *fn)
{
  sprintf(fn, "%s/%s", YAMP_TEMP_PATH_BASE,
                       YAMP_RAW_FILE_NAME);
  return;
}

int yamp_prepare_sup_eeprom_cache()
{
    int rc = 0;
    char sup_eeprom_filename[YAMP_FILE_NAME_SIZE];
    char sup_raw_filename[YAMP_FILE_NAME_SIZE];
    char cmd_buf[YAMP_CMD_BUF_SIZE];
    yamp_get_eeprom_filename(sup_eeprom_filename);
    yamp_get_raw_filename(sup_raw_filename);

    // First, read raw BIOS contents
    snprintf(cmd_buf, YAMP_CMD_BUF_SIZE, "%s read %s\n",
             YAMP_BIOS_CMD, sup_raw_filename);
    rc = system(cmd_buf);
    if (rc != 0) {
      OBMC_ERROR(ENOSPC, "Failed to read raw BIOS binary. rc = %d\n", rc);
      return -ENOSPC;
    }

    // Then, parse the EEPROM block only
    snprintf(cmd_buf, YAMP_CMD_BUF_SIZE,
             "/bin/dd if=%s of=%s bs=1 skip=%d count=%d\n",
             sup_raw_filename, sup_eeprom_filename, YAMP_SUP_EEPROM_OFFSET,
             YAMP_EEPROM_SIZE);
    rc = system(cmd_buf);
    if (rc != 0) {
      OBMC_ERROR(ENOSPC,
              "Failed to parse BIOS binary into an EEPROM block. rc = %d\n",
              rc);
      rc = -ENOSPC;
    }

    // Remove interim temp file anyway
    remove(sup_raw_filename);
    return rc;
}

/*
 * YAMP EEPROM has different format and field names from FB standard
 * This function will read the eeprom, and try to map the YAMP fields
 * to the FB standard fields
 */
int yamp_eeprom_parse(const char *target, struct wedge_eeprom_st *eeprom)
{
  int rc = 0;
  int read_pointer = 0;
  uint32_t len;
  FILE *fin;
  int field_type;
  int field_len;
  int ver_major = 0;
  int ver_minor = 0;
  int dot_location = 0;
  int parse_sup_eeprom = 0;
  char local_target[256];
  char field_value[YAMP_EEPROM_SIZE]; // Will never overflow
  char fn[64];
  char buf[YAMP_EEPROM_SIZE];
  char bus_name[32];
  char sup_eeprom_filename[YAMP_FILE_NAME_SIZE];

  if (!eeprom) {
    return -EINVAL;
  }

  if (!target)
    return -EINVAL;

  snprintf(local_target, sizeof(local_target), "%s", target);
  yamp_str_toupper((char *)local_target);

  if (!strcmp(local_target, YAMP_EEPROM_CHS_OBJ)) {
    sprintf(fn, "%s%s/eeprom", YAMP_EEPROM_PATH_BASE,
                               YAMP_EEPROM_CHASSIS);
  } else if (!strcmp(local_target, YAMP_EEPROM_SCD_OBJ)) {
    sprintf(fn, "%s%s/eeprom", YAMP_EEPROM_PATH_BASE,
                               YAMP_EEPROM_SWITCH_CARD);
  } else if (!strcmp(local_target, YAMP_EEPROM_SUP_OBJ)) {
    rc = yamp_prepare_sup_eeprom_cache();
    if (rc != 0) {
      OBMC_ERROR(ENOSPC, "Unable to dump the SUP EEPROM contents to ramdisk."
              " Likely due to the lack of disk space. rc = %d\n", rc);
      return -ENOSPC;
    }
    yamp_get_eeprom_filename(sup_eeprom_filename);
    sprintf(fn, "%s", sup_eeprom_filename);
    // Note that SUP's EEPROM format is a bit different from others,
    // in that it doesn't start with eeprom header.
    // Instead, it starts from prefdl field.
    parse_sup_eeprom = 1;
  } else if (yamp_get_lc_bus_name((const char *)local_target, bus_name) == 0) {
    sprintf(fn, "%s%s/eeprom", YAMP_EEPROM_PATH_BASE, bus_name);
  } else {
    return -EINVAL;
  }

  fin = fopen(fn, "r");
  if (fin == NULL) {
    rc = errno;
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
   * YAMP eeprom. Start with all fields filled up as NULL or 0x0,
   * then overwrite the values as we get the information from eeprom
   */
  memset(eeprom, 0, sizeof(struct wedge_eeprom_st));

  /*
   * The following check is only for NON-SUP eeproms, which
   * has all these fields.
   */
  if (parse_sup_eeprom != 1) {

      /* First, make sure the eeprom header is correct */
      if ((buf[read_pointer] != YAMP_EEPROM_HEADER_B1) ||
          (buf[read_pointer + 1] != YAMP_EEPROM_HEADER_B2) ||
          (buf[read_pointer + 2] != YAMP_EEPROM_HEADER_B3) ||
          (buf[read_pointer + 3] != YAMP_EEPROM_HEADER_B4)) {
        OBMC_ERROR(EINVAL, "Wrong EEPROM header! expected %02x %02x %02x %02x, \
            got %02x %02x %02x %02x",
            YAMP_EEPROM_HEADER_B1, YAMP_EEPROM_HEADER_B2,
            YAMP_EEPROM_HEADER_B3, YAMP_EEPROM_HEADER_B4,
            buf[read_pointer], buf[read_pointer+1],
            buf[read_pointer + 2], buf[read_pointer+3]);
        rc = -EINVAL;
      }
      /* As we read the first four bytes, advance the read pointer */
      read_pointer += YAMP_EEPROM_TYPE_SIZE;
      /* Bypass the eeprom size field */
      read_pointer += YAMP_EEPROM_TYPE_SIZE;
  }

  /* Bypass prefdl symbol and PCA */
  read_pointer += YAMP_EEPROM_TYPE_SIZE + YAMP_EEPROM_PCA_SIZE;

  /* Parse serial number. If there is additional serial TLV,
   * this value will be overwritten */
  yamp_parse_string(&read_pointer, (char *)&eeprom->fbw_product_serial,
                    buf, YAMP_EEPROM_SERIAL_SIZE);
  read_pointer += YAMP_EEPROM_KVN_SIZE;
  /* Product type is hardcoded to YAMP */
  sprintf(eeprom->fbw_product_name, "YAMP");

  rc = 0;
  /* Now go though EEPROM contents to fill out the fields */
  while ((rc == 0) && (read_pointer < YAMP_EEPROM_SIZE)) {
    rc = yamp_parse_hexadecimal(&read_pointer, &field_type, buf, 2);
    if (rc < 0)
      goto out;
    rc = yamp_parse_hexadecimal(&read_pointer, &field_len, buf, 4);
    if (rc < 0)
      goto out;
    /*
     * If this is the end of EEPROM (specified as "END" TLV),
     * do not read any value, but just bail out
     */
    if (field_type == YAMP_EEPROM_FIELD_END)
      break;
    /* Otherwise, read the field */
    memcpy(field_value, &buf[read_pointer], field_len);
    read_pointer += field_len;

    /* Now, map the value into similar field in FB EEPROM */
    switch(field_type)
    {
      case YAMP_EEPROM_FIELD_MFGTIME:
          memcpy(eeprom->fbw_system_manufacturing_date, field_value, 10);
          break;

      case YAMP_EEPROM_FIELD_ASY:
          memcpy(eeprom->fbw_assembly_number, field_value,
              FBW_EEPROM_F_ASSEMBLY_NUMBER);
          eeprom->fbw_assembly_number[FBW_EEPROM_F_ASSEMBLY_NUMBER -1] = 0;
          break;

      case YAMP_EEPROM_FIELD_SKU:
          memcpy(eeprom->fbw_product_number, field_value,
              FBW_EEPROM_F_PRODUCT_NUMBER);
          eeprom->fbw_assembly_number[FBW_EEPROM_F_PRODUCT_NUMBER -1] = 0;
          break;

      case YAMP_EEPROM_FIELD_MACBASE:
          for (int i = 0; i < 6; i++)
            eeprom->fbw_mac_base[i] = yamp_htoi(field_value[2*i]) * 16 +
                                      yamp_htoi(field_value[2*i + 1]);
          /* Also hardcade mac size as the same value as Minipack */
          eeprom->fbw_mac_size = 139;
          break;

      case YAMP_EEPROM_FIELD_HWREV:
          /* Scan for period character */
          dot_location = field_len;
          for (int i = 0; i < field_len; i++)
            if (field_value[i] == '.')
              dot_location = i;

          /* Parse the number before dot, and fit it into uint8_t */
          for (int i = 0; i < dot_location; i++) {
            ver_major *= 10;
            ver_major += yamp_htoi(field_value[i]);
          }
          /* Parse the number after dot, and fit it into uint8_t */
          ver_major = ver_major % 256;
          for (int i = dot_location + 1; i < field_len; i++) {
            ver_minor *= 10;
            ver_minor += yamp_htoi(field_value[i]);
          }
          /* Fit the value into uint8_t */
          ver_minor = ver_minor % 256;
          eeprom->fbw_product_version = ver_major;
          eeprom->fbw_product_subversion = ver_minor;
          break;

      case YAMP_EEPROM_FIELD_SERIAL:
          memcpy(&eeprom->fbw_product_serial, field_value,
              YAMP_EEPROM_SERIAL_SIZE);
          eeprom->fbw_product_serial[YAMP_EEPROM_SERIAL_SIZE] = '\0';
          break;

      case YAMP_EEPROM_FIELD_RESERVED1:
      case YAMP_EEPROM_FIELD_RESERVED2:
      case YAMP_EEPROM_FIELD_RESERVED3:
      case YAMP_EEPROM_FIELD_RESERVED4:
      case YAMP_EEPROM_FIELD_RESERVED5:
      case YAMP_EEPROM_FIELD_RESERVED6:
      case YAMP_EEPROM_FIELD_RESERVED7:
      case YAMP_EEPROM_FIELD_FLDVAR:
      case YAMP_EEPROM_FIELD_HWAPI:
      case YAMP_EEPROM_FIELD_SID:
      case YAMP_EEPROM_FIELD_PCA:
          /* These fields are not really useful. Do nothing (discard) */
          break;

      default:
          /* Invalid value. Bail out */
          rc = -EINVAL;
          goto out;
    }
  }

 out:
  if (fin) {
    fclose(fin);
  }
  if (parse_sup_eeprom == 1) {
    // Remove temporary file created for SUP EEPROM read
    remove(sup_eeprom_filename);
  }

  // rc > 0 means success here.
  // (it will usually have the number of bytes read)
  return (rc >= 0) ? 0 : -rc;
}
