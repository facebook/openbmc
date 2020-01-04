/*
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include "fruid.h"

#define FIELD_TYPE(x)     ((x & (0x03 << 6)) >> 6)
#define FIELD_LEN(x)      (x & ~(0x03 << 6))
#define FIELD_EMPTY       "N/A"
#define NO_MORE_DATA_BYTE 0xC1
#define MAX_FIELD_LENGTH  63  // 6-bit for length

/* Unix time difference between 1970 and 1996. */
#define UNIX_TIMESTAMP_1996   820454400

/* Array for BCD Plus definition. */
const char bcd_plus_array[] = "0123456789 -.XXX";

/* Array for 6-Bit ASCII definition. */
const char * ascii_6bit[4] = {
  " !\"#$%&'()*+,-./",
  "0123456789:;<=>?",
  "@ABCDEFGHIJKLMNO",
  "PQRSTUVWXYZ[\\]^_"
};

/*
 * calculate_time - calculate time from the unix time stamp stored
 *
 * @mfg_time    : Unix timestamp since 1996
 *
 * returns char * for mfg_time_str
 * returns NULL for memory allocation failure
 */
static char * calculate_time(uint8_t * mfg_time)
{
  int len;
  struct tm * local;
  time_t unix_time = 0;
  unix_time = ((mfg_time[2] << 16) + (mfg_time[1] << 8) + mfg_time[0]) * 60;
  unix_time += UNIX_TIMESTAMP_1996;

  local = localtime(&unix_time);
  char * str = asctime(local);

  len = strlen(str);

  char * mfg_time_str = (char *) malloc(len);
  if (!mfg_time_str) {
#ifdef DEBUG
    syslog(LOG_WARNING, "fruid: malloc: memory allocation failed\n");
#endif
    return NULL;
  }

  memset(mfg_time_str, 0, len);

  memcpy(mfg_time_str, str, len);

  mfg_time_str[len - 1] = '\0';

  return mfg_time_str;
}

/*
 * verify_chksum - verify the zero checksum of the data
 *
 * @area        : offset of the area
 * @len         : len of the area in bytes
 * @chksum_read : stored checksum in the data.
 *
 * returns 0 if chksum is verified
 * returns -1 if there exist a mismatch
 */
static int verify_chksum(uint8_t * area, uint8_t len, uint8_t chksum_read)
{
  int i;
  uint8_t chksum = 0;

  for (i = 0; i < len - 1; i++)
    chksum += area[i];

  /* Zero checksum calculation */
  chksum = ~(chksum) + 1;

  return (chksum == chksum_read) ? 0 : -1;
}

/*
 * get_chassis_type - get the Chassis type
 *
 * @type_hex  : type stored in the data
 *
 * returns char ptr for chassis type string
 * returns NULL if type not in the list
 */
static char * get_chassis_type(uint8_t type_hex)
{
  int type, ret;
  char type_int[4];

  ret = sprintf(type_int, "%u", type_hex);
  type = atoi(type_int) - 1;

  /* If the type is not in the list defined.*/
  if (type > FRUID_CHASSIS_TYPECODE_MAX || type < FRUID_CHASSIS_TYPECODE_MIN) {
#ifdef DEBUG
    syslog(LOG_INFO, "fruid: chassis area: invalid chassis type\n");
#endif
    return NULL;
  }

  char * type_str = (char *) malloc(strlen(fruid_chassis_type[type])+1);
  if (!type_str) {
#ifdef DEBUG
    syslog(LOG_WARNING, "fruid: malloc: memory allocation failed\n");
#endif
    return NULL;
  }

  memcpy(type_str, fruid_chassis_type[type], strlen(fruid_chassis_type[type]));
  type_str[strlen(fruid_chassis_type[type])] = '\0';

  return type_str;
}

/*
 * _fruid_area_field_read - read the field data
 *
 * @offset    : offset of the field
 *
 * returns char ptr for the field data string
 */
static char * _fruid_area_field_read(uint8_t *offset)
{
  int field_type, field_len, field_len_eff;
  int idx, idx_eff, val;
  char * field;

  /* Bits 7:6 */
  field_type = FIELD_TYPE(offset[0]);
  /* Bits 5:0 */
  field_len = FIELD_LEN(offset[0]);

  /* Calculate the effective length of the field data based on type stored. */
  switch (field_type) {

  case TYPE_BINARY:
    /* TODO: Need to add support to read data stored in binary type. */
    field_len_eff = 1;
    break;

  case TYPE_ASCII_6BIT:
    /*
     * Every 3 bytes have four 6-bit packed values
     * + 6-bit values from the remaining field bytes.
     */
    field_len_eff = (field_len / 3) * 4 + (field_len % 3);
    break;

  case TYPE_BCD_PLUS:
  case TYPE_ASCII_8BIT:
    field_len_eff = field_len;
    break;
  }

  /* If field data is zero, store 'N/A' for that field. */
  field_len_eff > 0 ? (field = (char *) malloc(field_len_eff + 1)) :
                      (field = (char *) malloc(strlen(FIELD_EMPTY)));
  if (!field) {
#ifdef DEBUG
    syslog(LOG_WARNING, "fruid: malloc: memory allocation failed\n");
#endif
    return NULL;
  }

  memset(field, 0, field_len + 1);

  if (field_len_eff < 1) {
    strcpy(field, FIELD_EMPTY);
    return field;
  }

  /* Retrieve field data depending on the type it was stored. */
  switch (field_type) {
  case TYPE_BINARY:
   /* TODO: Need to add support to read data stored in binary type. */
    break;

  case TYPE_BCD_PLUS:

    idx = 0;
    while (idx != field_len) {
      field[idx] = bcd_plus_array[offset[idx + 1] & 0x0F];
      idx++;
    }
    field[idx] = '\0';
    break;

  case TYPE_ASCII_6BIT:

    idx_eff = 0, idx = 1;

    while (field_len > 0) {

      /* 6-Bits => Bits 5:0 of the first byte */
      val = offset[idx] & 0x3F;
      field[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];
      field_len--;

      if (field_len > 0) {
        /* 6-Bits => Bits 3:0 of second byte + Bits 7:6 of first byte. */
        val = ((offset[idx] & 0xC0) >> 6) |
              ((offset[idx + 1] & 0x0F) << 2);
        field[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];
        field_len--;
      }

      if (field_len > 0) {
        /* 6-Bits => Bits 1:0 of third byte + Bits 7:4 of second byte. */
        val = ((offset[idx + 1] & 0xF0) >> 4) |
              ((offset[idx + 2] & 0x03) << 4);
        field[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];

        /* 6-Bits => Bits 7:2 of third byte. */
        val = ((offset[idx + 2] & 0xFC) >> 2);
        field[idx_eff++] = ascii_6bit[(val & 0xF0) >> 4][val & 0x0F];

        field_len--;
        idx += 3;
      }
    }
    /* Add Null terminator */
    field[idx_eff] = '\0';
    break;

  case TYPE_ASCII_8BIT:

    memcpy(field, offset + 1, field_len);
    /* Add Null terminator */
    field[field_len] = '\0';
    break;
  }

  return field;
}

/* Free all the memory allocated for fruid information */
void free_fruid_info(fruid_info_t * fruid)
{
  if (fruid->chassis.flag) {
    free(fruid->chassis.type_str);
    free(fruid->chassis.part);
    free(fruid->chassis.serial);
    free(fruid->chassis.custom1);
    free(fruid->chassis.custom2);
    free(fruid->chassis.custom3);
    free(fruid->chassis.custom4);
    free(fruid->chassis.custom5);
    free(fruid->chassis.custom6);
  }

  if (fruid->board.flag) {
    free(fruid->board.mfg_time_str);
    free(fruid->board.mfg_time);
    free(fruid->board.mfg);
    free(fruid->board.name);
    free(fruid->board.serial);
    free(fruid->board.part);
    free(fruid->board.fruid);
    free(fruid->board.custom1);
    free(fruid->board.custom2);
    free(fruid->board.custom3);
    free(fruid->board.custom4);
    free(fruid->board.custom5);
    free(fruid->board.custom6);
  }

  if (fruid->product.flag) {
    free(fruid->product.mfg);
    free(fruid->product.name);
    free(fruid->product.part);
    free(fruid->product.version);
    free(fruid->product.serial);
    free(fruid->product.asset_tag);
    free(fruid->product.fruid);
    free(fruid->product.custom1);
    free(fruid->product.custom2);
    free(fruid->product.custom3);
    free(fruid->product.custom4);
    free(fruid->product.custom5);
    free(fruid->product.custom6);
  }
}

/* Initialize the fruid information struct */
static void init_fruid_info(fruid_info_t * fruid)
{
  fruid->chassis.flag = 0;
  fruid->board.flag = 0;
  fruid->product.flag = 0;
  fruid->chassis.type_str = NULL;
  fruid->chassis.part_type_len = 0;
  fruid->chassis.part = NULL;
  fruid->chassis.serial_type_len = 0;
  fruid->chassis.serial = NULL;
  fruid->chassis.custom1_type_len = 0;
  fruid->chassis.custom1 = NULL;
  fruid->chassis.custom2_type_len = 0;
  fruid->chassis.custom2 = NULL;
  fruid->chassis.custom3_type_len = 0;
  fruid->chassis.custom3 = NULL;
  fruid->chassis.custom4_type_len = 0;
  fruid->chassis.custom4 = NULL;
  fruid->chassis.custom5_type_len = 0;
  fruid->chassis.custom5 = NULL;
  fruid->chassis.custom6_type_len = 0;
  fruid->chassis.custom6 = NULL;
  fruid->chassis.chksum = 0;
  fruid->board.mfg_time_str = NULL;
  fruid->board.mfg_time = NULL;
  fruid->board.mfg_type_len = 0;
  fruid->board.mfg = NULL;
  fruid->board.name_type_len = 0;
  fruid->board.name = NULL;
  fruid->board.serial_type_len = 0;
  fruid->board.serial = NULL;
  fruid->board.part_type_len = 0;
  fruid->board.part = NULL;
  fruid->board.fruid_type_len = 0;
  fruid->board.fruid = NULL;
  fruid->board.custom1_type_len = 0;
  fruid->board.custom1 = NULL;
  fruid->board.custom2_type_len = 0;
  fruid->board.custom2 = NULL;
  fruid->board.custom3_type_len = 0;
  fruid->board.custom3 = NULL;
  fruid->board.custom4_type_len = 0;
  fruid->board.custom4 = NULL;
  fruid->board.custom5_type_len = 0;
  fruid->board.custom5 = NULL;
  fruid->board.custom6_type_len = 0;
  fruid->board.custom6 = NULL;
  fruid->board.chksum = 0;
  fruid->product.mfg_type_len = 0;
  fruid->product.mfg = NULL;
  fruid->product.name_type_len = 0;
  fruid->product.name = NULL;
  fruid->product.part_type_len = 0;
  fruid->product.part = NULL;
  fruid->product.version_type_len = 0;
  fruid->product.version = NULL;
  fruid->product.serial_type_len = 0;
  fruid->product.serial = NULL;
  fruid->product.asset_tag_type_len = 0;
  fruid->product.asset_tag = NULL;
  fruid->product.fruid_type_len = 0;
  fruid->product.fruid = NULL;
  fruid->product.custom1_type_len = 0;
  fruid->product.custom1 = NULL;
  fruid->product.custom2_type_len = 0;
  fruid->product.custom2 = NULL;
  fruid->product.custom3_type_len = 0;
  fruid->product.custom3 = NULL;
  fruid->product.custom4_type_len = 0;
  fruid->product.custom4 = NULL;
  fruid->product.custom5_type_len = 0;
  fruid->product.custom5 = NULL;
  fruid->product.custom6_type_len = 0;
  fruid->product.custom6 = NULL;
  fruid->product.chksum = 0;
}

/* Parse the Product area data */
int parse_fruid_area_product(uint8_t * product,
      fruid_area_product_t * fruid_product)
{
  int ret, index;

  index = 0;

  /* Reset the struct to zero */
  memset(fruid_product, 0, sizeof(fruid_area_product_t));

  /* Check if the format version is as per IPMI FRUID v1.0 format spec */
  fruid_product->format_ver = product[index++];
  if (fruid_product->format_ver != FRUID_FORMAT_VER) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: product_area: format version not supported");
#endif
    return EPROTONOSUPPORT;
  }

  fruid_product->area_len = product[index++] * FRUID_AREA_LEN_MULTIPLIER;
  fruid_product->lang_code = product[index++];

  fruid_product->chksum = product[fruid_product->area_len - 1];
  ret = verify_chksum((uint8_t *) product,
          fruid_product->area_len, fruid_product->chksum);

  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: product_area: chksum not verified.");
#endif
    return EBADF;
  }

  fruid_product->mfg_type_len = product[index];
  fruid_product->mfg = _fruid_area_field_read(&product[index]);
  if (fruid_product->mfg == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  fruid_product->name_type_len = product[index];
  fruid_product->name = _fruid_area_field_read(&product[index]);
  if (fruid_product->name == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  fruid_product->part_type_len = product[index];
  fruid_product->part = _fruid_area_field_read(&product[index]);
  if (fruid_product->part == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  fruid_product->version_type_len = product[index];
  fruid_product->version = _fruid_area_field_read(&product[index]);
  if (fruid_product->version == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  fruid_product->serial_type_len = product[index];
  fruid_product->serial = _fruid_area_field_read(&product[index]);
  if (fruid_product->serial == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  fruid_product->asset_tag_type_len = product[index];
  fruid_product->asset_tag = _fruid_area_field_read(&product[index]);
  if (fruid_product->asset_tag == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  fruid_product->fruid_type_len = product[index];
  fruid_product->fruid = _fruid_area_field_read(&product[index]);
  if (fruid_product->fruid == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_product->custom1_type_len = product[index];
  if (product[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_product->custom1 = _fruid_area_field_read(&product[index]);
  if (fruid_product->custom1 == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_product->custom2_type_len = product[index];
  if (product[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_product->custom2 = _fruid_area_field_read(&product[index]);
  if (fruid_product->custom2 == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_product->custom3_type_len = product[index];
  if (product[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_product->custom3 = _fruid_area_field_read(&product[index]);
  if (fruid_product->custom3 == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_product->custom4_type_len = product[index];
  if (product[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_product->custom4 = _fruid_area_field_read(&product[index]);
  if (fruid_product->custom4 == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_product->custom5_type_len = product[index];
  if (product[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_product->custom5 = _fruid_area_field_read(&product[index]);
  if (fruid_product->custom5 == NULL)
    return ENOMEM;
  index += FIELD_LEN(product[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_product->custom6_type_len = product[index];
  if (product[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_product->custom6 = _fruid_area_field_read(&product[index]);
  if (fruid_product->custom6 == NULL)
    return ENOMEM;

  return 0;
}

/* Parse the Board area data */
int parse_fruid_area_board(uint8_t * board,
      fruid_area_board_t * fruid_board)
{
  int ret, index, i;
  time_t unix_time;

  index = 0;

  /* Reset the struct to zero */
  memset(fruid_board, 0, sizeof(fruid_area_board_t));

  /* Check if the format version is as per IPMI FRUID v1.0 format spec */
  fruid_board->format_ver = board[index++];
  if (fruid_board->format_ver != FRUID_FORMAT_VER) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: board_area: format version not supported");
#endif
    return EPROTONOSUPPORT;
  }
  fruid_board->area_len = board[index++] * FRUID_AREA_LEN_MULTIPLIER;
  fruid_board->lang_code = board[index++];

  fruid_board->chksum = board[fruid_board->area_len - 1];
  ret = verify_chksum((uint8_t *) board,
          fruid_board->area_len, fruid_board->chksum);

  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: board_area: chksum not verified.");
#endif
    return EBADF;
  }

  fruid_board->mfg_time = (uint8_t *) malloc(3*sizeof(uint8_t));
  if (fruid_board->mfg_time == NULL)
    return ENOMEM;
  for (i = 0; i < 3; i++) {
    fruid_board->mfg_time[i] = board[index++];
  }

  fruid_board->mfg_time_str = calculate_time(fruid_board->mfg_time);
  if (fruid_board->mfg_time_str == NULL)
    return ENOMEM;

  fruid_board->mfg_type_len = board[index];
  fruid_board->mfg = _fruid_area_field_read(&board[index]);
  if (fruid_board->mfg == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  fruid_board->name_type_len = board[index];
  fruid_board->name = _fruid_area_field_read(&board[index]);
  if (fruid_board->name == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  fruid_board->serial_type_len = board[index];
  fruid_board->serial = _fruid_area_field_read(&board[index]);
  if (fruid_board->serial == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  fruid_board->part_type_len = board[index];
  fruid_board->part = _fruid_area_field_read(&board[index]);
  if (fruid_board->part == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  fruid_board->fruid_type_len = board[index];
  fruid_board->fruid = _fruid_area_field_read(&board[index]);
  if (fruid_board->fruid == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_board->custom1_type_len = board[index];
  if (board[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_board->custom1 = _fruid_area_field_read(&board[index]);
  if (fruid_board->custom1 == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_board->custom2_type_len = board[index];
  if (board[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_board->custom2 = _fruid_area_field_read(&board[index]);
  if (fruid_board->custom2 == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_board->custom3_type_len = board[index];
  if (board[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_board->custom3 = _fruid_area_field_read(&board[index]);
  if (fruid_board->custom3 == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_board->custom4_type_len = board[index];
  if (board[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_board->custom4 = _fruid_area_field_read(&board[index]);
  if (fruid_board->custom4 == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_board->custom5_type_len = board[index];
  if (board[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_board->custom5 = _fruid_area_field_read(&board[index]);
  if (fruid_board->custom5 == NULL)
    return ENOMEM;
  index += FIELD_LEN(board[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_board->custom6_type_len = board[index];
  if (board[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_board->custom6 = _fruid_area_field_read(&board[index]);
  if (fruid_board->custom6 == NULL)
    return ENOMEM;

  return 0;
}

/* Parse the Chassis area data */
int parse_fruid_area_chassis(uint8_t * chassis,
      fruid_area_chassis_t * fruid_chassis)
{
  int ret, index;

  index = 0;

  /* Reset the struct to zero */
  memset(fruid_chassis, 0, sizeof(fruid_area_chassis_t));

  /* Check if the format version is as per IPMI FRUID v1.0 format spec */
  fruid_chassis->format_ver = chassis[index++];
  if (fruid_chassis->format_ver != FRUID_FORMAT_VER) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: chassis_area: format version not supported");
#endif
    return EPROTONOSUPPORT;
  }

  fruid_chassis->area_len = chassis[index++] * FRUID_AREA_LEN_MULTIPLIER;
  fruid_chassis->type = chassis[index++];

  fruid_chassis->chksum = chassis[fruid_chassis->area_len - 1];
  ret = verify_chksum((uint8_t *) chassis,
          fruid_chassis->area_len, fruid_chassis->chksum);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: chassis_area: chksum not verified.");
#endif
    return EBADF;
  }

  fruid_chassis->type_str = get_chassis_type(fruid_chassis->type);
  if (fruid_chassis->type_str == NULL)
    return ENOMSG;

  fruid_chassis->part_type_len = chassis[index];
  fruid_chassis->part = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->part == NULL)
    return ENOMEM;
  index += FIELD_LEN(chassis[index]) + 1;

  fruid_chassis->serial_type_len = chassis[index];
  fruid_chassis->serial = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->serial == NULL)
    return ENOMEM;
  index += FIELD_LEN(chassis[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_chassis->custom1_type_len = chassis[index];
  if (chassis[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_chassis->custom1 = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->custom1 == NULL)
    return ENOMEM;
  index += FIELD_LEN(chassis[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_chassis->custom2_type_len = chassis[index];
  if (chassis[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_chassis->custom2 = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->custom2 == NULL)
    return ENOMEM;
  index += FIELD_LEN(chassis[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_chassis->custom3_type_len = chassis[index];
  if (chassis[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_chassis->custom3 = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->custom3 == NULL)
    return ENOMEM;
  index += FIELD_LEN(chassis[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_chassis->custom4_type_len = chassis[index];
  if (chassis[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_chassis->custom4 = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->custom4 == NULL)
    return ENOMEM;
  index += FIELD_LEN(chassis[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_chassis->custom5_type_len = chassis[index];
  if (chassis[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_chassis->custom5 = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->custom5 == NULL)
    return ENOMEM;
  index += FIELD_LEN(chassis[index]) + 1;

  /* Check if this field was last and there is no more custom data */
  fruid_chassis->custom6_type_len = chassis[index];
  if (chassis[index] == NO_MORE_DATA_BYTE)
    return 0;
  fruid_chassis->custom6 = _fruid_area_field_read(&chassis[index]);
  if (fruid_chassis->custom6 == NULL)
    return ENOMEM;

  return 0;
}

/* Calculate the area offsets and populate the fruid_eeprom_t struct */
void set_fruid_eeprom_offsets(uint8_t * eeprom, fruid_header_t * header,
      fruid_eeprom_t * fruid_eeprom)
{
  fruid_eeprom->header = eeprom + 0x00;

  header->offset_area.chassis ? (fruid_eeprom->chassis = eeprom + \
    (header->offset_area.chassis * FRUID_OFFSET_MULTIPLIER)) : \
    (fruid_eeprom->chassis = NULL);

  header->offset_area.board ? (fruid_eeprom->board = eeprom + \
    (header->offset_area.board * FRUID_OFFSET_MULTIPLIER)) : \
    (fruid_eeprom->board = NULL);

  header->offset_area.product ? (fruid_eeprom->product = eeprom + \
    (header->offset_area.product * FRUID_OFFSET_MULTIPLIER)) : \
    (fruid_eeprom->product = NULL);

  header->offset_area.multirecord ? (fruid_eeprom->multirecord = eeprom + \
    (header->offset_area.multirecord * FRUID_OFFSET_MULTIPLIER)) : \
    (fruid_eeprom->multirecord = NULL);
}

/* Populate the common header struct */
int parse_fruid_header(uint8_t * eeprom, fruid_header_t * header)
{
  int ret;

  memcpy((uint8_t *)header, (uint8_t *)eeprom, sizeof(fruid_header_t));
  ret = verify_chksum((uint8_t *) header,
          sizeof(fruid_header_t), header->chksum);
  if (ret) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: common_header: chksum not verified.");
#endif
    return EBADF;
  }

  return ret;
}

/* Parse the eeprom dump and populate the fruid info in struct */
int populate_fruid_info(fruid_eeprom_t * fruid_eeprom, fruid_info_t * fruid)
{
  int ret;

  /* Initial all the required fruid structures */
  fruid_area_chassis_t fruid_chassis;
  fruid_area_board_t fruid_board;
  fruid_area_product_t fruid_product;

  /* If Chassis area is present, parse and print it */
  if (fruid_eeprom->chassis) {
    ret = parse_fruid_area_chassis(fruid_eeprom->chassis, &fruid_chassis);
    if (!ret) {
      fruid->chassis.flag = 1;
      fruid->chassis.format_ver = fruid_chassis.format_ver;
      fruid->chassis.area_len = fruid_chassis.area_len;
      fruid->chassis.type = fruid_chassis.type;
      fruid->chassis.type_str = fruid_chassis.type_str;
      fruid->chassis.part_type_len = fruid_chassis.part_type_len;
      fruid->chassis.part = fruid_chassis.part;
      fruid->chassis.serial_type_len = fruid_chassis.serial_type_len;
      fruid->chassis.serial = fruid_chassis.serial;
      fruid->chassis.custom1_type_len = fruid_chassis.custom1_type_len;
      fruid->chassis.custom1 = fruid_chassis.custom1;
      fruid->chassis.custom2_type_len = fruid_chassis.custom2_type_len;
      fruid->chassis.custom2 = fruid_chassis.custom2;
      fruid->chassis.custom3_type_len = fruid_chassis.custom3_type_len;
      fruid->chassis.custom3 = fruid_chassis.custom3;
      fruid->chassis.custom4_type_len = fruid_chassis.custom4_type_len;
      fruid->chassis.custom4 = fruid_chassis.custom4;
      fruid->chassis.custom5_type_len = fruid_chassis.custom5_type_len;
      fruid->chassis.custom5 = fruid_chassis.custom5;
      fruid->chassis.custom6_type_len = fruid_chassis.custom6_type_len;
      fruid->chassis.custom6 = fruid_chassis.custom6;
      fruid->chassis.chksum = fruid_chassis.chksum;
    } else
      return ret;
  }

  /* If Board area is present, parse and print it */
  if (fruid_eeprom->board) {
    ret = parse_fruid_area_board(fruid_eeprom->board, &fruid_board);
    if (!ret) {
      fruid->board.flag = 1;
      fruid->board.format_ver = fruid_board.format_ver;
      fruid->board.area_len = fruid_board.area_len;
      fruid->board.lang_code = fruid_board.lang_code;
      fruid->board.mfg_time = fruid_board.mfg_time;
      fruid->board.mfg_time_str = fruid_board.mfg_time_str;
      fruid->board.mfg_type_len = fruid_board.mfg_type_len;
      fruid->board.mfg = fruid_board.mfg;
      fruid->board.name_type_len = fruid_board.name_type_len;
      fruid->board.name = fruid_board.name;
      fruid->board.serial_type_len = fruid_board.serial_type_len;
      fruid->board.serial = fruid_board.serial;
      fruid->board.part_type_len = fruid_board.part_type_len;
      fruid->board.part = fruid_board.part;
      fruid->board.fruid_type_len = fruid_board.fruid_type_len;
      fruid->board.fruid = fruid_board.fruid;
      fruid->board.custom1_type_len = fruid_board.custom1_type_len;
      fruid->board.custom1 = fruid_board.custom1;
      fruid->board.custom2_type_len = fruid_board.custom2_type_len;
      fruid->board.custom2 = fruid_board.custom2;
      fruid->board.custom3_type_len = fruid_board.custom3_type_len;
      fruid->board.custom3 = fruid_board.custom3;
      fruid->board.custom4_type_len = fruid_board.custom4_type_len;
      fruid->board.custom4 = fruid_board.custom4;
      fruid->board.custom5_type_len = fruid_board.custom5_type_len;
      fruid->board.custom5 = fruid_board.custom5;
      fruid->board.custom6_type_len = fruid_board.custom6_type_len;
      fruid->board.custom6 = fruid_board.custom6;
      fruid->board.chksum = fruid_board.chksum;
    } else
      return ret;
  }

  /* If Product area is present, parse and print it */
  if (fruid_eeprom->product) {
    ret = parse_fruid_area_product(fruid_eeprom->product, &fruid_product);
    if (!ret) {
      fruid->product.flag = 1;
      fruid->product.format_ver = fruid_product.format_ver;
      fruid->product.area_len = fruid_product.area_len;
      fruid->product.lang_code = fruid_product.lang_code;
      fruid->product.mfg_type_len = fruid_product.mfg_type_len;
      fruid->product.mfg = fruid_product.mfg;
      fruid->product.name_type_len = fruid_product.name_type_len;
      fruid->product.name = fruid_product.name;
      fruid->product.part_type_len = fruid_product.part_type_len;
      fruid->product.part = fruid_product.part;
      fruid->product.version_type_len = fruid_product.version_type_len;
      fruid->product.version = fruid_product.version;
      fruid->product.serial_type_len = fruid_product.serial_type_len;
      fruid->product.serial = fruid_product.serial;
      fruid->product.asset_tag_type_len = fruid_product.asset_tag_type_len;
      fruid->product.asset_tag = fruid_product.asset_tag;
      fruid->product.fruid_type_len = fruid_product.fruid_type_len;
      fruid->product.fruid = fruid_product.fruid;
      fruid->product.custom1_type_len = fruid_product.custom1_type_len;
      fruid->product.custom1 = fruid_product.custom1;
      fruid->product.custom2_type_len = fruid_product.custom2_type_len;
      fruid->product.custom2 = fruid_product.custom2;
      fruid->product.custom3_type_len = fruid_product.custom3_type_len;
      fruid->product.custom3 = fruid_product.custom3;
      fruid->product.custom4_type_len = fruid_product.custom4_type_len;
      fruid->product.custom4 = fruid_product.custom4;
      fruid->product.custom5_type_len = fruid_product.custom5_type_len;
      fruid->product.custom5 = fruid_product.custom5;
      fruid->product.custom6_type_len = fruid_product.custom6_type_len;
      fruid->product.custom6 = fruid_product.custom6;
      fruid->product.chksum = fruid_product.chksum;
    } else
      return ret;
  }

  return 0;
}

/*
 * fruid_parse - To parse the bin file (eeprom) and populate
 *               the fruid information in the struct
 * @bin       : Eeprom binary file
 * @fruid     : ptr to the struct that holds the fruid information
 *
 * returns 0 on success
 * returns non-zero errno value on error
 */
int fruid_parse(const char * bin, fruid_info_t * fruid)
{
  int fruid_len, ret;
  FILE *fruid_fd;
  uint8_t * eeprom;

  /* Reset parser return value */
  ret = 0;

  /* Open the FRUID binary file */
  fruid_fd = fopen(bin, "rb");
  if (!fruid_fd) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: unable to open the file");
#endif
    return ENOENT;
  }

  /* Get the size of the binary file */
  fseek(fruid_fd, 0, SEEK_END);
  fruid_len = (uint32_t) ftell(fruid_fd);
  if (fruid_len == 0) {
    fclose(fruid_fd);
    syslog(LOG_WARNING, "fruid: file %s is empty");
    return -1;
  }

  fseek(fruid_fd, 0, SEEK_SET);

  eeprom = (uint8_t *) malloc(fruid_len);
  if (!eeprom) {
    fclose(fruid_fd);
#ifdef DEBUG
    syslog(LOG_WARNING, "fruid: malloc: memory allocation failed\n");
#endif
    return ENOMEM;
  }

  /* Read the binary file */
  fread(eeprom, sizeof(uint8_t), fruid_len, fruid_fd);

  /* Close the FRUID binary file */
  fclose(fruid_fd);

  /* Parse eeprom dump*/
  ret = fruid_parse_eeprom(eeprom, fruid_len, fruid);

  /* Free the eeprom malloced memory */
  free(eeprom);
  return ret;
}

/* Populate the fruid from eeprom dump*/
int fruid_parse_eeprom(const uint8_t * eeprom, int eeprom_len, fruid_info_t * fruid)
{
  int ret = 0;

  /* Initial all the required fruid structures */
  fruid_header_t fruid_header;
  fruid_eeprom_t fruid_eeprom;

  memset(&fruid_header, 0, sizeof(fruid_header_t));
  memset(&fruid_eeprom, 0, sizeof(fruid_eeprom_t));

  /* Parse the common header data */
  ret = parse_fruid_header(eeprom, &fruid_header);
  if (ret) {
    return ret;
  }

  /* Calculate all the area offsets */
  set_fruid_eeprom_offsets(eeprom, &fruid_header, &fruid_eeprom);

  init_fruid_info(fruid);
  /* Parse the eeprom and populate the fruid information */
  ret = populate_fruid_info(&fruid_eeprom, fruid);
  if (ret) {
    /* Free the malloced memory for the fruid information */
    free_fruid_info(fruid);
  }

  return ret;
}

static 
char *extract_content(const char *content) {
  int i = 0, j = 0;
  char *tmp_str = (char *) malloc ((strlen(content) + 1) * sizeof(char));
  
  while(content[i] != '\0') {
    if(content[i] != '\"') {
      tmp_str[j++] = content[i];  
    }
    i++;
  }
  tmp_str[j] = '\0';

  return tmp_str;
}

static
int calculate_chksum(uint8_t * start_offset, uint8_t area_length) {

  uint8_t chksum = 0;
  int i, ret;

  for (i = 0; i < area_length - 1; i++) {
    chksum += start_offset[i];
  }

  /* Zero checksum calculation */
  chksum = ~(chksum) + 1;

  /* Update new checksum */
  start_offset[area_length - 1] = chksum; 

  ret = verify_chksum((uint8_t *) start_offset, area_length, start_offset[area_length - 1]);
      
  return ret;
}

static
int set_mfg_time( uint8_t * time_field , char * set_time) {
  uint32_t time_value; 
  uint32_t datetime;
  time_t ipmi_time;
  char *ptr = NULL;

  if (!strcmp(set_time, "")) {   //Invalid time setting
    return -1;
  }

  strtol(set_time, &ptr, 10);

  if(strcmp(ptr, "")) {       //Invalid time setting
    return -1;
  }

  time_value = (uint32_t)atoi(set_time);
   
  ipmi_time = UNIX_TIMESTAMP_1996;

  datetime = (time_value - ipmi_time) / 60;

  time_field[0] = (datetime & 0xFF);
  time_field[1] = ((datetime >> 8) & 0xFF);
  time_field[2] = ((datetime >> 16) & 0xFF);

  return 0;
}

static
int alter_field_content(char **fru_field , char *content) {
  int content_len = strlen(content);
  if (*fru_field != NULL)
    free(*fru_field);
  *fru_field = content;
  return 0;
}

int fruid_modify(const char * cur_bin, const char * new_bin, const char * field, const char * content)
{
  int fruid_len, ret;
  FILE *fruid_fd;
  uint8_t *old_eeprom, *eeprom = NULL;
  int total_field_opt_size = sizeof(fruid_field_all_opt)/sizeof(fruid_field_all_opt[0]);
  fruid_info_t fruid;
  int i, j, len;
  int target = -1;
  char *tmp_content = NULL;
  int content_len;
  int new_fruid_len;
  uint8_t type_length;

  /* Reset parser return value */
  ret = 0;

  /* Open the FRUID binary file */
  fruid_fd = fopen(cur_bin, "rb");
  if (!fruid_fd) {
#ifdef DEBUG
    syslog(LOG_ERR, "fruid: unable to open the file");
#endif
    return ENOENT;
  }

  /* Get the size of the binary file */
  fseek(fruid_fd, 0, SEEK_END);
  fruid_len = (uint32_t) ftell(fruid_fd);

  fseek(fruid_fd, 0, SEEK_SET);

  old_eeprom = (uint8_t *) malloc(fruid_len);
  if (!old_eeprom) {
#ifdef DEBUG
    syslog(LOG_WARNING, "fruid: malloc: memory allocation failed\n");
#endif
    return ENOMEM;
  }

  /* Read the binary file */
  fread(old_eeprom, sizeof(uint8_t), fruid_len, fruid_fd);

  /* Close the FRUID binary file */
  fclose(fruid_fd);

  /* Parse eeprom dump*/
  ret = fruid_parse_eeprom(old_eeprom, fruid_len, &fruid);

  /* Free the eeprom malloced memory */
  free(old_eeprom);

  if (ret) {
    printf("Failed to parse FRU!\n");
    return -1;
  } 

  for(i = 0; i < total_field_opt_size; i++) {
    if (!strcmp(field, fruid_field_all_opt[i])) {
      target = i;
      break;
    }
  }
  if (target == -1) {
    printf("Parameter \"%s\" is invalid!\n", field);
    return -1;
  }

  if (target <= CCD6) {
    if (fruid.chassis.flag != 1) {
      printf("Chassis Area is invalid!\n");
      return -1;
    }
  } else if (target <= BCD6) {
    if (fruid.board.flag != 1) {
      printf("Board Area is invalid!\n");
      return -1;
    }
  } else {
    if (fruid.product.flag != 1) {
      printf("Product Area is invalid!\n");
      return -1;
    }
  }

  tmp_content = extract_content(content);
  content_len = strlen(tmp_content);

  if (content_len > MAX_FIELD_LENGTH) {
    printf("Content length \"%s\" is more than its maximum length:%d !\n", field, MAX_FIELD_LENGTH);
    return -1;
  }

  if (content_len)
    type_length = 0xc0 + content_len;
  else
    type_length = 0;

  // check custom field type length
  switch (target) {
    case CCD1:
    case CCD2:
    case CCD3:
    case CCD4:
    case CCD5:
    case CCD6:
    case BCD1:
    case BCD2:
    case BCD3:
    case BCD4:
    case BCD5:
    case BCD6:
    case PCD1:
    case PCD2:
    case PCD3:
    case PCD4:
    case PCD5:
    case PCD6:
      if (type_length == NO_MORE_DATA_BYTE) {
        printf("Content length \"%s\" should not be 1 !\n", field);
        return -1;
      }
      break;
    default:
      break;
  }

  // alter field content
  switch (target) {
    case CPN:
      fruid.chassis.part_type_len = type_length;
      alter_field_content(&fruid.chassis.part,tmp_content);
      break; 
    case CSN:
      fruid.chassis.serial_type_len = type_length;
      alter_field_content(&fruid.chassis.serial,tmp_content);
      break;
    case CCD1:
      fruid.chassis.custom1_type_len = type_length;
      alter_field_content(&fruid.chassis.custom1,tmp_content);
      break;
    case CCD2:
      if (fruid.chassis.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom1_type_len = 0;
      fruid.chassis.custom2_type_len = type_length;
      alter_field_content(&fruid.chassis.custom2,tmp_content);
      break;
    case CCD3:
      if (fruid.chassis.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom1_type_len = 0;
      if (fruid.chassis.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom2_type_len = 0;
      fruid.chassis.custom3_type_len = type_length;
      alter_field_content(&fruid.chassis.custom3,tmp_content);
      break;
    case CCD4:
      if (fruid.chassis.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom1_type_len = 0;
      if (fruid.chassis.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom2_type_len = 0;
      if (fruid.chassis.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom3_type_len = 0;
      fruid.chassis.custom4_type_len = type_length;
      alter_field_content(&fruid.chassis.custom4,tmp_content);
      break;
    case CCD5:
      if (fruid.chassis.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom1_type_len = 0;
      if (fruid.chassis.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom2_type_len = 0;
      if (fruid.chassis.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom3_type_len = 0;
      if (fruid.chassis.custom4_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom4_type_len = 0;
      fruid.chassis.custom5_type_len = type_length;
      alter_field_content(&fruid.chassis.custom5,tmp_content);
      break;
    case CCD6:
      if (fruid.chassis.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom1_type_len = 0;
      if (fruid.chassis.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom2_type_len = 0;
      if (fruid.chassis.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom3_type_len = 0;
      if (fruid.chassis.custom4_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom4_type_len = 0;
      if (fruid.chassis.custom5_type_len == NO_MORE_DATA_BYTE)
        fruid.chassis.custom5_type_len = 0;
      fruid.chassis.custom6_type_len = type_length;
      alter_field_content(&fruid.chassis.custom6,tmp_content);
      break;
    case BMD:
      if(set_mfg_time(fruid.board.mfg_time, tmp_content) < 0) {
        return -1;
      }
      break;
    case BM:
      fruid.board.mfg_type_len = type_length;
      alter_field_content(&fruid.board.mfg,tmp_content);
      break;
    case BP:
      fruid.board.name_type_len = type_length;
      alter_field_content(&fruid.board.name,tmp_content);
      break;
    case BSN:
      fruid.board.serial_type_len = type_length;
      alter_field_content(&fruid.board.serial,tmp_content);
      break;
    case BPN:
      fruid.board.part_type_len = type_length;
      alter_field_content(&fruid.board.part,tmp_content);
      break;
    case BFI:
      fruid.board.fruid_type_len = type_length;
      alter_field_content(&fruid.board.fruid,tmp_content);
      break;
    case BCD1:
      fruid.board.custom1_type_len = type_length;
      alter_field_content(&fruid.board.custom1,tmp_content);
      break;
    case BCD2:
      if (fruid.board.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom1_type_len = 0;
      fruid.board.custom2_type_len = type_length;
      alter_field_content(&fruid.board.custom2,tmp_content);
      break;
    case BCD3:
      if (fruid.board.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom1_type_len = 0;
      if (fruid.board.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom2_type_len = 0;
      fruid.board.custom3_type_len = type_length;
      alter_field_content(&fruid.board.custom3,tmp_content);
      break;
    case BCD4:
      if (fruid.board.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom1_type_len = 0;
      if (fruid.board.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom2_type_len = 0;
      if (fruid.board.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom3_type_len = 0;
      fruid.board.custom4_type_len = type_length;
      alter_field_content(&fruid.board.custom4,tmp_content);
      break;
    case BCD5:
      if (fruid.board.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom1_type_len = 0;
      if (fruid.board.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom2_type_len = 0;
      if (fruid.board.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom3_type_len = 0;
      if (fruid.board.custom4_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom4_type_len = 0;
      fruid.board.custom5_type_len = type_length;
      alter_field_content(&fruid.board.custom5,tmp_content);
      break;
    case BCD6:
      if (fruid.board.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom1_type_len = 0;
      if (fruid.board.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom2_type_len = 0;
      if (fruid.board.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom3_type_len = 0;
      if (fruid.board.custom4_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom4_type_len = 0;
      if (fruid.board.custom5_type_len == NO_MORE_DATA_BYTE)
        fruid.board.custom5_type_len = 0;
      fruid.board.custom6_type_len = type_length;
      alter_field_content(&fruid.board.custom6,tmp_content);
      break;
    case PM:
      fruid.product.mfg_type_len = type_length;
      alter_field_content(&fruid.product.mfg,tmp_content);
      break;
    case PN:
      fruid.product.name_type_len = type_length;
      alter_field_content(&fruid.product.name,tmp_content);
      break;
    case PPN:
      fruid.product.part_type_len = type_length;
      alter_field_content(&fruid.product.part,tmp_content);
      break;
    case PV:
      fruid.product.version_type_len = type_length;
      alter_field_content(&fruid.product.version,tmp_content);
      break;
    case PSN:
      fruid.product.serial_type_len = type_length;
      alter_field_content(&fruid.product.serial,tmp_content);
      break;
    case PAT:
      fruid.product.asset_tag_type_len = type_length;
      alter_field_content(&fruid.product.asset_tag,tmp_content);
      break;
    case PFI:
      fruid.product.fruid_type_len = type_length;
      alter_field_content(&fruid.product.fruid,tmp_content);
      break;
    case PCD1:
      fruid.product.custom1_type_len = type_length;
      alter_field_content(&fruid.product.custom1,tmp_content);
      break;
    case PCD2:
      if (fruid.product.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom1_type_len = 0;
      fruid.product.custom2_type_len = type_length;
      alter_field_content(&fruid.product.custom2,tmp_content);
      break;
    case PCD3:
      if (fruid.product.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom1_type_len = 0;
      if (fruid.product.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom2_type_len = 0;
      fruid.product.custom3_type_len = type_length;
      alter_field_content(&fruid.product.custom3,tmp_content);
      break;
    case PCD4:
      if (fruid.product.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom1_type_len = 0;
      if (fruid.product.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom2_type_len = 0;
      if (fruid.product.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom3_type_len = 0;
      fruid.product.custom4_type_len = type_length;
      alter_field_content(&fruid.product.custom4,tmp_content);
      break;
    case PCD5:
      if (fruid.product.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom1_type_len = 0;
      if (fruid.product.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom2_type_len = 0;
      if (fruid.product.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom3_type_len = 0;
      if (fruid.product.custom4_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom4_type_len = 0;
      fruid.product.custom5_type_len = type_length;
      alter_field_content(&fruid.product.custom5,tmp_content);
      break;
    case PCD6:
      if (fruid.product.custom1_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom1_type_len = 0;
      if (fruid.product.custom2_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom2_type_len = 0;
      if (fruid.product.custom3_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom3_type_len = 0;
      if (fruid.product.custom4_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom4_type_len = 0;
      if (fruid.product.custom5_type_len == NO_MORE_DATA_BYTE)
        fruid.product.custom5_type_len = 0;
      fruid.product.custom6_type_len = type_length;
      alter_field_content(&fruid.product.custom6,tmp_content);
      break;
    default:
      return -1;
      break;
  }

  // create new FRU alloc new eeporm
  new_fruid_len = fruid_len + ((content_len / 8) + 1) * 8;
  eeprom = (uint8_t *) malloc(new_fruid_len);
  memset(eeprom, 0, new_fruid_len);
  if (!eeprom) {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s: malloc: memory allocation failed", __func__);
#endif
    return ENOMEM;
  }

  // chassis area
  i = 8;
  len = 0;
  int start = i;
  if (fruid.chassis.flag == 1) {
    eeprom[i++] = fruid.chassis.format_ver;
    eeprom[i++] = fruid.chassis.area_len;
    eeprom[i++] = fruid.chassis.type;

    eeprom[i++] = fruid.chassis.part_type_len;
    len = FIELD_LEN(fruid.chassis.part_type_len);
    memcpy(&eeprom[i], fruid.chassis.part, len);
    i += len;

    eeprom[i++] = fruid.chassis.serial_type_len;
    len = FIELD_LEN(fruid.chassis.serial_type_len);
    memcpy(&eeprom[i], fruid.chassis.serial, len);
    i += len;

    eeprom[i++] = fruid.chassis.custom1_type_len;
    if (fruid.chassis.custom1_type_len == NO_MORE_DATA_BYTE)
      goto chasis_chksum;
    len = FIELD_LEN(fruid.chassis.custom1_type_len);
    memcpy(&eeprom[i], fruid.chassis.custom1, len);
    i += len;

    eeprom[i++] = fruid.chassis.custom2_type_len;
    if (fruid.chassis.custom2_type_len == NO_MORE_DATA_BYTE)
      goto chasis_chksum;
    len = FIELD_LEN(fruid.chassis.custom2_type_len);
    memcpy(&eeprom[i], fruid.chassis.custom2, len);
    i += len;

    eeprom[i++] = fruid.chassis.custom3_type_len;
    if (fruid.chassis.custom3_type_len == NO_MORE_DATA_BYTE)
      goto chasis_chksum;
    len = FIELD_LEN(fruid.chassis.custom3_type_len);
    memcpy(&eeprom[i], fruid.chassis.custom3, len);
    i += len;

    eeprom[i++] = fruid.chassis.custom4_type_len;
    if (fruid.chassis.custom4_type_len == NO_MORE_DATA_BYTE)
      goto chasis_chksum;
    len = FIELD_LEN(fruid.chassis.custom4_type_len);
    memcpy(&eeprom[i], fruid.chassis.custom4, len);
    i += len;

    eeprom[i++] = fruid.chassis.custom5_type_len;
    if (fruid.chassis.custom5_type_len == NO_MORE_DATA_BYTE)
      goto chasis_chksum;
    len = FIELD_LEN(fruid.chassis.custom5_type_len);
    memcpy(&eeprom[i], fruid.chassis.custom5, len);
    i += len;

    eeprom[i++] = fruid.chassis.custom6_type_len;
    if (fruid.chassis.custom6_type_len == NO_MORE_DATA_BYTE)
      goto chasis_chksum;
    len = FIELD_LEN(fruid.chassis.custom6_type_len);
    memcpy(&eeprom[i], fruid.chassis.custom6, len);
    i += len;

  chasis_chksum:
    if (eeprom[i - 1] != NO_MORE_DATA_BYTE) {
      eeprom[i++] = NO_MORE_DATA_BYTE;
    }
    i += (7 - (i % 8));
    fruid.chassis.area_len = i - start + 1;
    eeprom[start + 1] = fruid.chassis.area_len / FRUID_AREA_LEN_MULTIPLIER;
    ret = calculate_chksum(&eeprom[start], fruid.chassis.area_len);
    if (ret < 0) {
      printf("Update chassis checksum fail!\n");
      goto error_exit;
    }
    i++;
  } else {
    fruid.chassis.area_len = 0;
  }

  // board area
  start = i;
  if (fruid.board.flag == 1) {
    eeprom[i++] = fruid.board.format_ver;
    eeprom[i++] = fruid.board.area_len;
    eeprom[i++] = fruid.board.lang_code;

    memcpy(&eeprom[i], fruid.board.mfg_time, 3);
    i += 3;

    eeprom[i++] = fruid.board.mfg_type_len;
    len = FIELD_LEN(fruid.board.mfg_type_len);
    memcpy(&eeprom[i], fruid.board.mfg, len);
    i += len;

    eeprom[i++] = fruid.board.name_type_len;
    len = FIELD_LEN(fruid.board.name_type_len);
    memcpy(&eeprom[i], fruid.board.name, len);
    i += len;

    eeprom[i++] = fruid.board.serial_type_len;
    len = FIELD_LEN(fruid.board.serial_type_len);
    memcpy(&eeprom[i], fruid.board.serial, len);
    i += len;

    eeprom[i++] = fruid.board.part_type_len;
    len = FIELD_LEN(fruid.board.part_type_len);
    memcpy(&eeprom[i], fruid.board.part, len);
    i += len;

    eeprom[i++] = fruid.board.fruid_type_len;
    len = FIELD_LEN(fruid.board.fruid_type_len);
    memcpy(&eeprom[i], fruid.board.fruid, len);
    i += len;

    eeprom[i++] = fruid.board.custom1_type_len;
    if (fruid.board.custom1_type_len == NO_MORE_DATA_BYTE)
      goto board_chksum;
    len = FIELD_LEN(fruid.board.custom1_type_len);
    memcpy(&eeprom[i], fruid.board.custom1, len);
    i += len;

    eeprom[i++] = fruid.board.custom2_type_len;
    if (fruid.board.custom2_type_len == NO_MORE_DATA_BYTE)
      goto board_chksum;
    len = FIELD_LEN(fruid.board.custom2_type_len);
    memcpy(&eeprom[i], fruid.board.custom2, len);
    i += len;

    eeprom[i++] = fruid.board.custom3_type_len;
    if (fruid.board.custom3_type_len == NO_MORE_DATA_BYTE)
      goto board_chksum;
    len = FIELD_LEN(fruid.board.custom3_type_len);
    memcpy(&eeprom[i], fruid.board.custom3, len);
    i += len;

    eeprom[i++] = fruid.board.custom4_type_len;
    if (fruid.board.custom4_type_len == NO_MORE_DATA_BYTE)
      goto board_chksum;
    len = FIELD_LEN(fruid.board.custom4_type_len);
    memcpy(&eeprom[i], fruid.board.custom4, len);
    i += len;

    eeprom[i++] = fruid.board.custom5_type_len;
    if (fruid.board.custom5_type_len == NO_MORE_DATA_BYTE)
      goto board_chksum;
    len = FIELD_LEN(fruid.board.custom5_type_len);
    memcpy(&eeprom[i], fruid.board.custom5, len);
    i += len;

    eeprom[i++] = fruid.board.custom6_type_len;
    if (fruid.board.custom6_type_len == NO_MORE_DATA_BYTE)
      goto board_chksum;
    len = FIELD_LEN(fruid.board.custom6_type_len);
    memcpy(&eeprom[i], fruid.board.custom6, len);
    i += len;

  board_chksum:
    if (eeprom[i - 1] != NO_MORE_DATA_BYTE) {
      eeprom[i++] = NO_MORE_DATA_BYTE;
    }
    i += (7 - (i % 8));
    fruid.board.area_len = i - start + 1;
    eeprom[start + 1] = fruid.board.area_len / FRUID_AREA_LEN_MULTIPLIER;
    ret = calculate_chksum(&eeprom[start], fruid.board.area_len);
    if (ret < 0) {
      printf("Update board checksum fail!\n");
      goto error_exit;
    }
    i++;
  } else {
    fruid.board.area_len = 0;
  }

  // product area
  start = i;
  if (fruid.product.flag == 1) {
    eeprom[i++] = fruid.product.format_ver;
    eeprom[i++] = fruid.product.area_len;
    eeprom[i++] = fruid.product.lang_code;

    eeprom[i++] = fruid.product.mfg_type_len;
    len = FIELD_LEN(fruid.product.mfg_type_len);
    memcpy(&eeprom[i], fruid.product.mfg, len);
    i += len;

    eeprom[i++] = fruid.product.name_type_len;
    len = FIELD_LEN(fruid.product.name_type_len);
    memcpy(&eeprom[i], fruid.product.name, len);
    i += len;

    eeprom[i++] = fruid.product.part_type_len;
    len = FIELD_LEN(fruid.product.part_type_len);
    memcpy(&eeprom[i], fruid.product.part, len);
    i += len;

    eeprom[i++] = fruid.product.version_type_len;
    len = FIELD_LEN(fruid.product.version_type_len);
    memcpy(&eeprom[i], fruid.product.version, len);
    i += len;

    eeprom[i++] = fruid.product.serial_type_len;
    len = FIELD_LEN(fruid.product.serial_type_len);
    memcpy(&eeprom[i], fruid.product.serial, len);
    i += len;

    eeprom[i++] = fruid.product.asset_tag_type_len;
    len = FIELD_LEN(fruid.product.asset_tag_type_len);
    memcpy(&eeprom[i], fruid.product.asset_tag, len);
    i += len;

    eeprom[i++] = fruid.product.fruid_type_len;
    len = FIELD_LEN(fruid.product.fruid_type_len);
    memcpy(&eeprom[i], fruid.product.fruid, len);
    i += len;

    eeprom[i++] = fruid.product.custom1_type_len;
    if (fruid.product.custom1_type_len == NO_MORE_DATA_BYTE)
      goto product_chksum;
    len = FIELD_LEN(fruid.product.custom1_type_len);
    memcpy(&eeprom[i], fruid.product.custom1, len);
    i += len;

    eeprom[i++] = fruid.product.custom2_type_len;
    if (fruid.product.custom2_type_len == NO_MORE_DATA_BYTE)
      goto product_chksum;
    len = FIELD_LEN(fruid.product.custom2_type_len);
    memcpy(&eeprom[i], fruid.product.custom2, len);
    i += len;

    eeprom[i++] = fruid.product.custom3_type_len;
    if (fruid.product.custom3_type_len == NO_MORE_DATA_BYTE)
      goto product_chksum;
    len = FIELD_LEN(fruid.product.custom3_type_len);
    memcpy(&eeprom[i], fruid.product.custom3, len);
    i += len;

    eeprom[i++] = fruid.product.custom4_type_len;
    if (fruid.product.custom4_type_len == NO_MORE_DATA_BYTE)
      goto product_chksum;
    len = FIELD_LEN(fruid.product.custom4_type_len);
    memcpy(&eeprom[i], fruid.product.custom4, len);
    i += len;

    eeprom[i++] = fruid.product.custom5_type_len;
    if (fruid.product.custom5_type_len == NO_MORE_DATA_BYTE)
      goto product_chksum;
    len = FIELD_LEN(fruid.product.custom5_type_len);
    memcpy(&eeprom[i], fruid.product.custom5, len);
    i += len;

    eeprom[i++] = fruid.product.custom6_type_len;
    if (fruid.product.custom6_type_len == NO_MORE_DATA_BYTE)
      goto product_chksum;
    len = FIELD_LEN(fruid.product.custom6_type_len);
    memcpy(&eeprom[i], fruid.product.custom6, len);
    i += len;

  product_chksum:
    if (eeprom[i - 1] != NO_MORE_DATA_BYTE) {
      eeprom[i++] = NO_MORE_DATA_BYTE;
    }
    i += (7 - (i % 8));
    fruid.product.area_len = i - start + 1;
    eeprom[start + 1] = fruid.product.area_len / FRUID_AREA_LEN_MULTIPLIER;
    ret = calculate_chksum(&eeprom[start], fruid.product.area_len);
    if (ret < 0) {
      printf("Update product checksum fail!\n");
      goto error_exit;
    }
    i++;
  } else {
    fruid.product.area_len = 0;
  }

  // update header
  int offset = 1;
  eeprom[0] = 0x01; // format version
  if (fruid.chassis.flag == 1) {
    eeprom[2] = offset; // chassis area offset
  }
  if (fruid.board.flag == 1) {
    offset += fruid.chassis.area_len / FRUID_AREA_LEN_MULTIPLIER; // board area offset
    eeprom[3] = offset;
  }
  if (fruid.product.flag == 1) {
    offset += fruid.board.area_len / FRUID_AREA_LEN_MULTIPLIER; // product area offset
    eeprom[4] = offset;
  }

  //  update header checksum
  ret = calculate_chksum(&eeprom[0], 8);
  if (ret < 0) {
    printf("Update header checksum fail!\n");
    goto error_exit;
  }

  /* Open the FRUID binary file */
  fruid_fd = fopen(new_bin, "wb");
  if (!fruid_fd) {
#ifdef DEBUG
    syslog(LOG_ERR, "%s: unable to open the file %s", __func__, new_bin);
#endif
    return ENOENT;
  }

  /* Write the binary file */
  fwrite(eeprom, sizeof(uint8_t), i, fruid_fd);

  /* Close the FRUID binary file */
  fclose(fruid_fd);
 
error_exit:
  /* Free the eeprom malloced memory */
  free(eeprom);
  /* Free the malloced memory for the fruid information */
  free_fruid_info(&fruid);
  return ret;
}

