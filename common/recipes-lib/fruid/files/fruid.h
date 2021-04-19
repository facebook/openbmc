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

#ifndef __FRUID_H__
#define __FRUID_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <openbmc/ipmi.h>

#define FRUID_FORMAT_VER           0x01
#define FRUID_OFFSET_MULTIPLIER    8
#define FRUID_AREA_LEN_MULTIPLIER  8

#define FRUID_OFFSET_AREA_INTERNAL        0
#define FRUID_OFFSET_AREA_CHASSIS         1
#define FRUID_OFFSET_AREA_BOARD           2
#define FRUID_OFFSET_AREA_PRODUCT         3
#define FRUID_OFFSET_AREA_MULTIRECORD     4

#define FRUID_CHASSIS_TYPECODE_MIN        0
#define FRUID_CHASSIS_TYPECODE_MAX        31

#define MULTIRECORD_FORMAT_VER       0x02
#define MULTIRECORD_FORMAT_VER_MASK  0x0F
#define MULTIRECORD_LAST_RECORED_BIT (1 << 7)
/*----------------------------------------
 OEM Record ID
 ----------------------------------------*/
enum {
  OEM_RECORD_ID_OCP_NIC_3_0 = 0xC0,
  OEM_RECORD_ID_SMART_FAN = 0xFB,
};

/*
 * SMART FAN OEM Record ID Specification
 */
#define SMART_FAN_RECORD_ID          OEM_RECORD_ID_SMART_FAN
#define MANUFACTURER_ID_DATA_LENGTH  3
#define SMART_FAN_VERSION_LENGTH     4
#define SMART_FAN_FW_VERSION_LENGTH  4
#define SMART_FAN_MFG_LINE_LENGTH    8
#define SMART_FAN_CLEI_CODE_LENGTH   10
#define SMART_FAN_VOL_DATA_LENGTH    2
#define SMART_FAN_CUR_DATA_LENGTH    2
#define SMART_FAN_VOL_CUR_MULTIPLIER 10
#define SMART_FAN_RPM_DATA_LENGTH    3
#define MFG_DATE_TIME_LENGTH         3

/* To hold the common header information. */
typedef struct fruid_header_t {
  uint8_t format_ver : 4;
  struct {
    uint8_t internal;
    uint8_t chassis;
    uint8_t board;
    uint8_t product;
    uint8_t multirecord;
  } offset_area;
  uint8_t pad;
  uint8_t chksum;
} fruid_header_t;

/* To hold the Chassis area information. */
typedef struct fruid_area_chassis_t {
  uint8_t format_ver : 4;
  uint8_t area_len;
  uint8_t type;
  char * type_str;
  uint8_t part_type_len;
  char * part;
  uint8_t serial_type_len;
  char * serial;
  uint8_t custom1_type_len;
  char * custom1;
  uint8_t custom2_type_len;
  char * custom2;
  uint8_t custom3_type_len;
  char * custom3;
  uint8_t custom4_type_len;
  char * custom4;
  uint8_t custom5_type_len;
  char * custom5;
  uint8_t custom6_type_len;
  char * custom6;
  uint8_t chksum;
} fruid_area_chassis_t;

/* To hold the Board area information. */
typedef struct fruid_area_board_t {
  uint8_t format_ver : 4;
  uint8_t area_len;
  uint8_t lang_code;
  uint8_t * mfg_time;
  char * mfg_time_str;
  uint8_t mfg_type_len;
  char * mfg;
  uint8_t name_type_len;
  char * name;
  uint8_t serial_type_len;
  char * serial;
  uint8_t part_type_len;
  char * part;
  uint8_t fruid_type_len;
  char * fruid;
  uint8_t custom1_type_len;
  char * custom1;
  uint8_t custom2_type_len;
  char * custom2;
  uint8_t custom3_type_len;
  char * custom3;
  uint8_t custom4_type_len;
  char * custom4;
  uint8_t custom5_type_len;
  char * custom5;
  uint8_t custom6_type_len;
  char * custom6;
  uint8_t chksum;
} fruid_area_board_t;

/* To hold the Product area information. */
typedef struct fruid_area_product_t {
  uint8_t format_ver : 4;
  uint8_t area_len;
  uint8_t lang_code;
  uint8_t mfg_type_len;
  char * mfg;
  uint8_t name_type_len;
  char * name;
  uint8_t part_type_len;
  char * part;
  uint8_t version_type_len;
  char * version;
  uint8_t serial_type_len;
  char * serial;
  uint8_t asset_tag_type_len;
  char * asset_tag;
  uint8_t fruid_type_len;
  char * fruid;
  uint8_t custom1_type_len;
  char * custom1;
  uint8_t custom2_type_len;
  char * custom2;
  uint8_t custom3_type_len;
  char * custom3;
  uint8_t custom4_type_len;
  char * custom4;
  uint8_t custom5_type_len;
  char * custom5;
  uint8_t custom6_type_len;
  char * custom6;
  uint8_t chksum;
} fruid_area_product_t;

/* To hold the Multirecord area information. */
typedef struct fruid_area_multirecord_header_t {
  uint8_t type_id;
  uint8_t format_ver;
  uint8_t area_len;
  uint8_t record_chksum;
  uint8_t header_chksum;
} fruid_area_multirecord_header_t;

typedef struct fruid_area_multirecord_smart_fan_t {
  uint32_t manufacturer_id;
  char * smart_fan_ver;
  char * fw_ver;
  uint8_t * mfg_time;
  char * mfg_time_str;
  char * mfg_line;
  char * clei_code;
  uint32_t voltage;
  uint32_t current;
  uint32_t rpm_front;
  uint32_t rpm_rear;
} fruid_area_multirecord_smart_fan_t;

/* To hold all the fruid information */
typedef struct fruid_info_t {
  struct {
    uint8_t flag;
    uint8_t format_ver : 4;
    uint8_t area_len;
    uint8_t type;
    char * type_str;
    uint8_t part_type_len;
    char * part;
    uint8_t serial_type_len;
    char * serial;
    uint8_t custom1_type_len;
    char * custom1;
    uint8_t custom2_type_len;
    char * custom2;
    uint8_t custom3_type_len;
    char * custom3;
    uint8_t custom4_type_len;
    char * custom4;
    uint8_t custom5_type_len;
    char * custom5;
    uint8_t custom6_type_len;
    char * custom6;
    uint8_t chksum;
  } chassis;
  struct {
    uint8_t flag;
    uint8_t format_ver : 4;
    uint8_t area_len;
    uint8_t lang_code;
    uint8_t * mfg_time;
    char * mfg_time_str;
    uint8_t mfg_type_len;
    char * mfg;
    uint8_t name_type_len;
    char * name;
    uint8_t serial_type_len;
    char * serial;
    uint8_t part_type_len;
    char * part;
    uint8_t fruid_type_len;
    char * fruid;
    uint8_t custom1_type_len;
    char * custom1;
    uint8_t custom2_type_len;
    char * custom2;
    uint8_t custom3_type_len;
    char * custom3;
    uint8_t custom4_type_len;
    char * custom4;
    uint8_t custom5_type_len;
    char * custom5;
    uint8_t custom6_type_len;
    char * custom6;
    uint8_t chksum;
  } board;
  struct {
    uint8_t flag;
    uint8_t format_ver : 4;
    uint8_t area_len;
    uint8_t lang_code;
    uint8_t mfg_type_len;
    char * mfg;
    uint8_t name_type_len;
    char * name;
    uint8_t part_type_len;
    char * part;
    uint8_t version_type_len;
    char * version;
    uint8_t serial_type_len;
    char * serial;
    uint8_t asset_tag_type_len;
    char * asset_tag;
    uint8_t fruid_type_len;
    char * fruid;
    uint8_t custom1_type_len;
    char * custom1;
    uint8_t custom2_type_len;
    char * custom2;
    uint8_t custom3_type_len;
    char * custom3;
    uint8_t custom4_type_len;
    char * custom4;
    uint8_t custom5_type_len;
    char * custom5;
    uint8_t custom6_type_len;
    char * custom6;
    uint8_t chksum;
  } product;
  struct {
    uint8_t flag;
    uint32_t manufacturer_id;
    char * smart_fan_ver;
    char * fw_ver;
    uint8_t * mfg_time;
    char * mfg_time_str;
    char * mfg_line;
    char * clei_code;
    uint32_t voltage;
    uint32_t current;
    uint32_t rpm_front;
    uint32_t rpm_rear;
  } multirecord_smart_fan;
} fruid_info_t;

/* To hold the different area offsets. */
typedef struct fruid_eeprom_t {
  uint8_t * header;
  uint8_t * chassis;
  uint8_t * board;
  uint8_t * product;
  uint8_t * multirecord;
} fruid_eeprom_t;

/* List of all the Chassis types. */
static const char * fruid_chassis_type [] = {
  "Other",                    /* 0x01 */
  "Unknown",                  /* 0x02 */
  "Desktop",                  /* 0x03 */
  "Low Profile Desktop",      /* 0x04 */
  "Pizza Box",                /* 0x05 */
  "Mini Tower",               /* 0x06 */
  "Tower",                    /* 0x07 */
  "Portable",                 /* 0x08 */
  "Laptop",                   /* 0x09 */
  "Notebook",                 /* 0x0A */
  "Hand Held",                /* 0x0B */
  "Docking Station",          /* 0x0C */
  "All in One",               /* 0x0D */
  "Sub Notebook",             /* 0x0E */
  "Space-saving",             /* 0x0F */
  "Lunch Box",                /* 0x10 */
  "Main Server Chassis",      /* 0x11 */
  "Expansion Chassis",        /* 0x12 */
  "SubChassis",               /* 0x13 */
  "Bus Expansion Chassis",    /* 0x14 */
  "Peripheral Chassis",       /* 0x15 */
  "RAID Chassis",             /* 0x16 */
  "Rack Mount Chassis",       /* 0x17 */
  "Sealed-case PC",           /* 0x18 */
  "Multi-system Chassis",     /* 0x19 */
  "Compact PCI",              /* 0x1A */
  "Advanced TCA",             /* 0x1B */
  "Blade",                    /* 0x1C */
  "Blade Enclosure",          /* 0x1D */
  "Tablet",                   /* 0x1E */
  "Convertible",              /* 0x1F */
  "Detachable"                /* 0x20 */
};

static const char * fruid_field_all_opt[] = {
  "--CPN",
  "--CSN",
  "--CCD1",
  "--CCD2",
  "--CCD3",
  "--CCD4",
  "--CCD5",
  "--CCD6",
  "--BMD",
  "--BM",
  "--BP",
  "--BSN",
  "--BPN",
  "--BFI",
  "--BCD1",
  "--BCD2",
  "--BCD3",
  "--BCD4",
  "--BCD5",
  "--BCD6",
  "--PM",
  "--PN",
  "--PPN",
  "--PV",
  "--PSN",
  "--PAT",
  "--PFI",
  "--PCD1",
  "--PCD2",
  "--PCD3",
  "--PCD4",
  "--PCD5",
  "--PCD6"
};

enum {
  CPN,
  CSN,
  CCD1,
  CCD2,
  CCD3,
  CCD4,
  CCD5,
  CCD6,
  BMD,
  BM,
  BP,
  BSN,
  BPN,
  BFI,
  BCD1,
  BCD2,
  BCD3,
  BCD4,
  BCD5,
  BCD6,
  PM,
  PN,
  PPN,
  PV,
  PSN,
  PAT,
  PFI,
  PCD1,
  PCD2,
  PCD3,
  PCD4,
  PCD5,
  PCD6
};

int fruid_parse(const char * bin, fruid_info_t * fruid);
int fruid_parse_eeprom(const uint8_t * eeprom, int eeprom_len, fruid_info_t * fruid);
void free_fruid_info(fruid_info_t * fruid);
int fruid_modify(const char * cur_bin, const char * new_bin, const char * field, const char * content);

#ifdef __cplusplus
}
#endif

#endif /* __FRUID_H__ */
