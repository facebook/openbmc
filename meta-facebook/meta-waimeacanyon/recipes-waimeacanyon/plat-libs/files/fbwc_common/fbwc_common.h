/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#ifndef __FBWC_COMMON_H__
#define __FBWC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#define FRU_COMPONENT_NEW_VER_KEY "%s_%s_new_ver" 
#define MAX_PATH_LEN 128
#define MAX_NAME_LEN 32
#define READING_NA    (-2)

#define MB_BIC_VR_BUS 9
#define RNS_VCCIN_ADDR 0xC0
#define RNS_VCCD_ADDR 0xC6
#define RNS_VCCINFAON_ADDR 0xC2

#define MAX_FRU_NAME_STR  16
#define MAX_FRU_DATA_SIZE            512
#define NIC_FRU_ADDR  0x50
#define BSM_FRU_ADDR  0x56
#define SCM_FRU_ADDR  0x50
#define EEPROM_PATH              "/sys/bus/i2c/devices/%d-00%X/eeprom"
#define COMMON_FRU_PATH          "/tmp/fruid_%s.bin"
#define COMMON_TMP_FRU_PATH      "/tmp/tfruid_%s.bin"


enum {
  //if not match fru_str_list, Segmentation fault
  FRU_ALL = 0,
  FRU_IOM,
  FRU_MB,
  FRU_SCB,
  FRU_BMC,
  FRU_SCM,
  FRU_BSM,
  FRU_HDBP,
  FRU_PDB,
  FRU_NIC,
  FRU_FAN0,
  FRU_FAN1,
  FRU_FAN2,
  FRU_FAN3,
};

enum {
  FW_MB_CPLD = 1,
  FW_MB_BIC,
  FW_ME,
  FW_BIOS,
  FW_VR_VCCIN,
  FW_VR_VCCD,
  FW_VR_VCCINFAON,
  FW_VR,
};

enum {
  FRU_ID_MB         = 0,
  FRU_ID_SCM        = 1,
  FRU_ID_BSM        = 2,
};

enum {
  FORCE_UPDATE_UNSET = 0x0,
  FORCE_UPDATE_SET,
};

//0'base bus
enum {
  I2C_BUS_IOC = 0,
  I2C_BUS_EXP = 1,
  I2C_BUS_MB_BIC = 2,
  I2C_BUS_PEER_BIC = 3,
  I2C_BUS_NIC = 4,  // NIC EEPROM
  I2C_BUS_E1S_DEV0 = 5,
  I2C_BUS_E1S_DEV1 = 6,
  I2C_BUS_SCM = 15, // SCM EEPROM, BSM EEPROM
};

char* get_component_name(uint8_t comp);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __FBWC_COMMON_H__ */
