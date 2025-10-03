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
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "CrcLib.h"
#include "Eeprom.h"
#include "WeutilInterface.h"

#define FF_ID_OFFSET 0x1000

namespace weutil {

class EepromFboss : public Eeprom, public CrcLib {
 public:
  explicit EepromFboss(const std::string& eFile, const uint16_t off = 0);
  std::map<fieldId, std::pair<std::string, std::string>> getEepromData()
      override;

 private:
  void parseFixedField();
  bool parseTlvField();
  bool tlvField(int);
  bool crc16Check(uint16_t crcVal);

  // the first 4 bytes of fixed eeprom data.
  int magic_;
  int version_;
  int reserved_;
  uint16_t crc16_;
  std::string ePath_;
  std::map<int, std::string> dict_;

  std::map<fieldId, int> mappingTbl_{
      {VERSION, FF_ID_OFFSET + 1},
      {PRODUCT_NAME, 0x01},
      {PRODUCU_PART_NUM, 0x02},
      {SYS_ASSEM_PART_NUM, 0x03},
      {PCBA_PART_NUM, 0x04},
      {PCB_PART_NUM, 0x05},
      {VENDOR_PCBA_PART_NUM, 0x06},
      {VENDOR_PCBA_SERIAL_NUM, 0x07},
      {PRODUCT_PRODUCTION_STATE, 0x08},
      {PRODUCT_VER, 0x09},
      {PRODUCT_SUB_VER, 0x0a},
      {PRODUCT_SERIAL_NUM, 0x0b},
      {SYSTEM_MANUFACTURER, 0x0c},
      {SYSTEM_MANUFACT_DATA, 0x0d},
      {PCB_MANUFACTURER, 0x0e},
      {ASSEMBLED_AT, 0x0f},
      {LOCAL_MAC, 0x10},
      {EXTENDED_MAC_BASE, 0x11},
      {EXTENDED_MAC_ADDR_SIZE, 0x12},
      {EEPROM_LOCATRION_ON_FABRIC, 0x13},
      {CRC16, 0xfa},
  };

  //
  // Fixed Field Table:  all fixed  fields are defined here.
  // key in ffTbl[] started from 0x1000 to differentiate from tvlTbl_[]'s key.
  //
  std::map<int, std::pair<std::string, int>> ffTbl_{
      {FF_ID_OFFSET, std::make_pair("Magic", 2)},
      {FF_ID_OFFSET + 1, std::make_pair("Version", 1)},
      {FF_ID_OFFSET + 2, std::make_pair("Reserved", 1)},
  };

  // TLV Table: [tlv-id: [field-name, expected-lenth]].
  // length == -1 means variable.
  std::map<int, std::pair<std::string, int>> tlvTbl_{
      {0x01, std::make_pair("Product Name", -1)},
      {0x02, std::make_pair("Product Part Number", -1)},
      {0x03, std::make_pair("System Assembly Part Number", 8)},
      {0x04, std::make_pair("Meta PCBA Part Number", 12)},
      {0x05, std::make_pair("Meta PCB Part Number", 12)},
      {0x06, std::make_pair("ODM/JDM PCBA Part Number", -1)},
      {0x07, std::make_pair("ODM/JDM PCBA Serial Number", -1)},
      {0x08, std::make_pair("Product Production State", 1)},
      {0x09, std::make_pair("Product Version", 1)},
      {0x0a, std::make_pair("Product Sub-Version", 1)},
      {0x0b, std::make_pair("Product Serial Number", -1)},
      {0x0c, std::make_pair("System Manufacturer", -1)},
      {0x0d, std::make_pair("System Manufacturing Date", 8)},
      {0x0e, std::make_pair("PCB Manufacturer", -1)},
      {0x0f, std::make_pair("Assembled at", -1)},
      {0x10, std::make_pair("Local MAC", 6)},
      {0x11, std::make_pair("Extended MAC Base", 6)},
      {0x12, std::make_pair("Extended MAC Address Size", 2)},
      {0x13, std::make_pair("EEPROM location on Fabric", -1)},
      {0xfa, std::make_pair("CRC16", 2)},
  };

  //
  // define each TLV field value's type.
  // it is required for some value type's special treatment.
  //
  std::map<int, vFormat> valFmtTbl_{
      {0x01, VT_STR},    {0x02, VT_STR},   {0x03, VT_STR}, {0x04, VT_STR},
      {0x05, VT_STR},    {0x06, VT_STR},   {0x07, VT_STR}, {0x08, VT_INT8},
      {0x09, VT_INT8},   {0x0a, VT_INT8},  {0x0b, VT_STR}, {0x0c, VT_STR},
      {0x0d, VT_STR},    {0x0e, VT_STR},   {0x0f, VT_STR}, {0x10, VT_HEXMAC},
      {0x11, VT_HEXMAC}, {0x12, VT_INT16}, {0x13, VT_STR}, {0xfa, VT_INT16}};
};
} // namespace weutil
