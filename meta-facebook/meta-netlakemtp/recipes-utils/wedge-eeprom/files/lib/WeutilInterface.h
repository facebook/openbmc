/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include <optional>

#define DEFAULT_EEPROM_NAME "netlake-eeprom"
#define MIN_TYPE 0
#define MAX_TYPE 20
#define SYS_LEN 8
#define MAC_LEN 8
#define SYS_MANUFACTURING_DATE_LEN 8
#define PCB_PCBA_PART_LEN 12
#define PRODUCT_VER_STATE_LEN 1
#define MAC_ADR_LEN 5
#define MAC_ADR_BASE_LEN 1
#define HEADER_LEN 3
#define CRC_VALUE_H 1
#define CRC_VALUE_L 2
#define CRC_TYPE_ADR 0xFA
#define CRC_LEN_ADR 0x02
#define NULL_TYPE 0xFF
#define HEADER 0xFB
#define FORMAT_VERSION 0x05
#define CRC_INIT 0x1D0F //CRC-CCITT-AUG initial value
#define CHECK_HIGHEST_BIT 0x8000
#define TRUNCATED_POLYNOMIA 0x1021

//
//  META-FBOSS EEPROM V4 fields.
//
enum fieldId {
  VERSION = 0,
  PRODUCT_NAME,
  PRODUCU_PART_NUM,
  SYS_ASSEM_PART_NUM,
  PCBA_PART_NUM,
  PCB_PART_NUM,
  VENDOR_PCBA_PART_NUM,
  VENDOR_PCBA_SERIAL_NUM,
  PRODUCT_PRODUCTION_STATE,
  PRODUCT_VER,
  PRODUCT_SUB_VER,
  PRODUCT_SERIAL_NUM,
  SYSTEM_MANUFACTURER,
  SYSTEM_MANUFACT_DATA,
  PCB_MANUFACTURER,
  ASSEMBLED_AT,
  LOCAL_MAC,
  EXTENDED_MAC_BASE,
  EXTENDED_MAC_ADDR_SIZE,
  EEPROM_LOCATRION_ON_FABRIC,
  CRC16
};

//
//  META FBOSS EEPROM v5 Format TypeID.
//
enum typeId_v5 {
  ID_PRODUCT_NAME = 1,         //1,Product Name
  ID_PRODUCT_PART_NUM,         //2,Product Part Number
  ID_SYS_ASSEM_PART_NUM,       //3,System Assembly Part Number
  ID_PCBA_PART_NUM,            //4,Meta PCBA Part Number
  ID_PCB_PART_NUM,             //5,Meta PCB Part Number
  ID_ODM_JDM_PCBA_PART_NUM,    //6,ODM/JDM PCBA Part Number
  ID_ODM_JDM_PCBA_SERIAL_NUM,  //7,ODM/JDM PCBA Serial Number
  ID_PRODUCT_PRODUCTION_STATE, //8,Product Production State
  ID_PRODUCT_VER,              //9,Product Version
  ID_PRODUCT_SUB_VER,          //10,Product Sub-Version
  ID_PRODUCT_SERIAL_NUM,       //11,Product Serial Number
  ID_SYS_MANUFACTURER,         //12,System Manufacturer
  ID_SYS_MANUFACTURING_DATE,   //13,System Manufacturing Date
  ID_PCB_MANUFACTURER,         //14,PCB Manufacturer
  ID_ASSEMBLED_AT,             //15,Assembled at
  ID_EEPROM_LOC_FABRIC,        //16,EEPROM location on Fabric
  ID_X86_CPU_MAC,              //17,X86 CPU MAC Base + X86 CPU MAC Address Size
  ID_BMC_MAC,                  //18,BMC MAC Base + BMC MAC Address Size
  ID_SWITCH_ASIC_MAC,          //19,Switch ASIC MAC Base + Switch ASIC MAC Base
  ID_RSVD_MAC                  //20,META Reserved MAC Base + META Reserved MAC Address Size
};

std::map<std::string, std::string> listEepromDevices();
std::optional<std::string> eepromNameToPath(const std::string& devName);
std::map<fieldId, std::pair<std::string, std::string>> eepromParse(
    const std::string& eepromDeviceName);
std::vector<std::pair<std::string, std::string>> eepromParseNew(
    const std::string& eepromDeviceName);
std::string eepromFormat(std::string& eName);
