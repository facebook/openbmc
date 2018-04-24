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
#ifndef FBW_EEPROM_H
#define FBW_EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FBW_EEPROM_F_MAGIC 2
#define FBW_EEPROM_F_VERSION 1
#define FBW_EEPROM_F_PRODUCT_NAME 12
#define FBW_EEPROM_F_PRODUCT_NAME_V3 20
#define FBW_EEPROM_F_PRODUCT_NUMBER 8
#define FBW_EEPROM_F_ASSEMBLY_NUMBER 12
#define FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER 12
#define FBW_EEPROM_F_FACEBOOK_PCB_NUMBER 12
#define FBW_EEPROM_F_ODM_PCBA_NUMBER 13
#define FBW_EEPROM_F_ODM_PCBA_SERIAL 12
#define FBW_EEPROM_F_ODM_PCBA_SERIAL_V2 13
#define FBW_EEPROM_F_PRODUCT_STATE 1
#define FBW_EEPROM_F_PRODUCT_VERSION 1
#define FBW_EEPROM_F_PRODUCT_SUBVERSION 1
#define FBW_EEPROM_F_PRODUCT_SERIAL 12
#define FBW_EEPROM_F_PRODUCT_SERIAL_V2 13
#define FBW_EEPROM_F_PRODUCT_ASSET 12
#define FBW_EEPROM_F_SYSTEM_MANUFACTURER 8
#define FBW_EEPROM_F_SYSTEM_MANU_DATE 4
#define FBW_EEPROM_F_PCB_MANUFACTURER 8
#define FBW_EEPROM_F_ASSEMBLED 8
#define FBW_EEPROM_F_LOCAL_MAC 12
#define FBW_EEPROM_F_EXT_MAC_BASE 12
#define FBW_EEPROM_F_EXT_MAC_SIZE 2
#define FBW_EEPROM_F_LOCATION 8
#define FBW_EEPROM_F_LOCATION_V3 20
#define FBW_EEPROM_F_CRC8 1

#define __MAX(a, b) (((a) > (b)) ? (a) : (b))

struct wedge_eeprom_st {
  /* version number of the eeprom. Must be the first element */
  uint8_t fbw_version;

  /* Product Name */
  char fbw_product_name[__MAX(FBW_EEPROM_F_PRODUCT_NAME,
                              FBW_EEPROM_F_PRODUCT_NAME_V3)
                        + 1];

  /* Top Level 20 - Product Part Number: XX-XXXXXX */
  char fbw_product_number[FBW_EEPROM_F_PRODUCT_NUMBER + 2];

  /* System Assembly Part Number XXX-XXXXXX-XX */
  char fbw_assembly_number[FBW_EEPROM_F_ASSEMBLY_NUMBER + 3];

  /* Facebook PCBA Part Number: XXX-XXXXXXX-XX */
  char fbw_facebook_pcba_number[FBW_EEPROM_F_FACEBOOK_PCBA_NUMBER + 3];

  /* Facebook PCB Part Number: XXX-XXXXXXX-XX */
  char fbw_facebook_pcb_number[FBW_EEPROM_F_FACEBOOK_PCB_NUMBER + 3];

  /* ODM PCBA Part Number: XXXXXXXXXXXX */
  char fbw_odm_pcba_number[FBW_EEPROM_F_ODM_PCBA_NUMBER + 1];

  /* ODM PCBA Serial Number: XXXXXXXXXXXX */
  char fbw_odm_pcba_serial[__MAX(FBW_EEPROM_F_ODM_PCBA_SERIAL,
                                 FBW_EEPROM_F_ODM_PCBA_SERIAL_V2)
                           + 1];

  /* Product Production State */
  uint8_t fbw_production_state;

  /* Product Version */
  uint8_t fbw_product_version;

  /* Product Sub Version */
  uint8_t fbw_product_subversion;

  /* Product Serial Number: XXXXXXXX */
  char fbw_product_serial[__MAX(FBW_EEPROM_F_PRODUCT_SERIAL,
                                FBW_EEPROM_F_PRODUCT_SERIAL_V2)
                          + 1];

  /* Product Asset Tag: XXXXXXXX */
  char fbw_product_asset[FBW_EEPROM_F_PRODUCT_ASSET + 1];

  /* System Manufacturer: XXXXXXXX */
  char fbw_system_manufacturer[FBW_EEPROM_F_SYSTEM_MANUFACTURER + 1];

  /* System Manufacturing Date: mm-dd-yy */
  uint8_t fbw_system_manufacturing_date[10];

  /* PCB Manufacturer: XXXXXXXXX */
  char fbw_pcb_manufacturer[FBW_EEPROM_F_PCB_MANUFACTURER + 1];

  /* Assembled At: XXXXXXXX */
  char fbw_assembled[FBW_EEPROM_F_ASSEMBLED + 1];

  /* Local MAC Address */
  uint8_t fbw_local_mac[6];

  /* Extended MAC Address */
  uint8_t fbw_mac_base[6];

  /* Extended MAC Address Size */
  uint16_t fbw_mac_size;

  /* Location on Fabric: "LEFT"/"RIGHT", "WEDGE", "LC" */
  char fbw_location[__MAX(FBW_EEPROM_F_LOCATION,
                         FBW_EEPROM_F_LOCATION_V3)
                    + 1];

  /* CRC8 */
  uint8_t fbw_crc8;
};

int wedge_eeprom_parse(const char *fn, struct wedge_eeprom_st *eeprom);

#ifdef __cplusplus
}
#endif

#endif
