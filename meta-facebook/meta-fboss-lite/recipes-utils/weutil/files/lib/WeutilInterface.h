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

// TODO: change to chassis_eeprom
#define DEFAULT_EEPROM_NAME "dummy_eeprom"

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

std::map<std::string, std::string> listEepromDevices();
std::optional<std::string> eepromNameToPath(const std::string& devName);
std::map<fieldId, std::pair<std::string, std::string>> eepromParse(
    const std::string& eepromDeviceName);
std::string eepromFormat(std::string& eName);
