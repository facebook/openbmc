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
 */
#ifndef FBW_EEPROM_H
#define FBW_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FBW_EEPROM_F_MAGIC 2
#define FBW_EEPROM_F_VERSION 1
#define FBW_EEPROM_F_PRODUCT_NAME 12
#define FBW_EEPROM_F_PRODUCT_NUMBER 8
#define FBW_EEPROM_F_ASSEMBLY_NUMBER 12
#define FBW_EEPROM_F_FACEBOOK_PCB_NUMBER 12
#define FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER 12
#define FBW_EEPROM_F_ODM_PCB_NUMBER 13
#define FBW_EEPROM_F_ODM_PCB_SERIAL 13
#define FBW_EEPROM_F_PRODUCT_STATE 1
#define FBW_EEPROM_F_PRODUCT_VERSION 1
#define FBW_EEPROM_F_PRODUCT_SUBVERSION 1
#define FBW_EEPROM_F_PRODUCT_SERIAL 13
#define FBW_EEPROM_F_PRODUCT_ASSET 12
#define FBW_EEPROM_F_SYSTEM_MANUFACTURER 8
#define FBW_EEPROM_F_SYSTEM_MANU_DATE 4
#define FBW_EEPROM_F_PCB_MANUFACTURER 8
#define FBW_EEPROM_F_ASSEMBLED 8
#define FBW_EEPROM_F_LOCAL_MAC 12
#define FBW_EEPROM_F_EXT_MAC_BASE 12
#define FBW_EEPROM_F_EXT_MAC_SIZE 2
#define FBW_EEPROM_F_LOCATION 8
#define FBW_EEPROM_F_CRC8 1

struct wedge_eeprom_st {
  /* version number of the eeprom. Must be the first element */
  uint16_t fbw_magic;

  /* version number of the eeprom. Must be the first element */
  uint8_t fbw_version;

  /* Product Name */
  char fbw_product_name[FBW_EEPROM_F_PRODUCT_NAME];

  /* Top Level 20 - Product Part Number: XX-XXXXXX */
  char fbw_product_number[FBW_EEPROM_F_PRODUCT_NUMBER];

  /* System Assembly Part Number XXX-XXXXXX-XX */
  char fbw_assembly_number[FBW_EEPROM_F_ASSEMBLY_NUMBER];

  /* Galaxy100 PCB Part Number: XXX-XXXXXXX-XX */
  char fbw_facebook_pcba_number[FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER];

  /* Galaxy100 PCB Part Number: XXX-XXXXXXX-XX */
  char fbw_facebook_pcb_number[FBW_EEPROM_F_FACEBOOK_PCB_NUMBER];

  /* ODM PCB Part Number: XXXXXXXXXXXX */
  char fbw_odm_pcb_number[FBW_EEPROM_F_ODM_PCB_NUMBER];

  /* ODM PCB Serial Number: XXXXXXXXXXXX */
  char fbw_odm_pcb_serial[FBW_EEPROM_F_ODM_PCB_SERIAL];

  /* Product Production State */
  uint8_t fbw_production_state;

  /* Product Version */
  uint8_t fbw_product_version;

  /* Product Sub Version */
  uint8_t fbw_product_subversion;

  /* Product Serial Number: XXXXXXXX */
  char fbw_product_serial[FBW_EEPROM_F_PRODUCT_SERIAL];

  /* Product Asset Tag: XXXXXXXX */
  char fbw_product_asset[FBW_EEPROM_F_PRODUCT_ASSET];

  /* System Manufacturer: XXXXXXXX */
  char fbw_system_manufacturer[FBW_EEPROM_F_SYSTEM_MANUFACTURER];

  /* System Manufacturing Date: mm-dd-yy */
  //char fbw_system_manufacturing_date[FBW_EEPROM_F_SYSTEM_MANU_DATE];
  //uint32_t fbw_system_manufacturing_date;
  uint16_t fbw_system_manufacturing_year;
  uint8_t fbw_system_manufacturing_month;
  uint8_t fbw_system_manufacturing_day;

  /* PCB Manufacturer: XXXXXXXXX */
  char fbw_pcb_manufacturer[FBW_EEPROM_F_PCB_MANUFACTURER];

  /* Assembled At: XXXXXXXX */
  char fbw_assembled[FBW_EEPROM_F_ASSEMBLED];

  /* Local MAC Address */
  uint8_t fbw_local_mac[FBW_EEPROM_F_LOCAL_MAC];

  /* Extended MAC Address */
  uint8_t fbw_mac_base[FBW_EEPROM_F_EXT_MAC_BASE];

  /* Extended MAC Address Size */
  uint16_t fbw_mac_size;

  /* Location on Fabric: "LEFT"/"RIGHT", "WEDGE", "LC" */
  char fbw_location[FBW_EEPROM_F_LOCATION];

  /* CRC8 */
  uint8_t fbw_crc8;
}__attribute__ ((__packed__));

//int wedge_eeprom_parse(const char *fn, struct wedge_eeprom_st *eeprom);
void dump_eeprom(struct wedge_eeprom_st *eeprom);
void parse_command(char* cmd, char* c);
int read_text(char* in);
int write_binary(char* out);
#ifdef __cplusplus
}
#endif

#endif


