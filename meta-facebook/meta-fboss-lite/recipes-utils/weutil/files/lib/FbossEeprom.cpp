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

#include "FbossEeprom.h"
#include "WeutilInterface.h"

namespace weutil {

/**
 *	EepromFboss::EepromFboss -derived class of eeprom.
 *	  customized for META-FBOSS eeprom V4 format data processing.
 *	  all fields definition and eeprom sysfs path are stored in this class
 *    object.
 */
EepromFboss::EepromFboss(const std::string& eFile, const uint16_t off)
    : Eeprom(eFile, off), ePath_(eFile) {
  parseFixedField();
  while (parseTlvField())
    ;
  if (!crc16Check(std::stoi(dict_[mappingTbl_[CRC16]]))) {
    throw std::runtime_error("eeprom checksum error!");
  }
}

void EepromFboss::parseFixedField() {
  magic_ = readInt16();
  version_ = readInt8();
  reserved_ = readInt8();

  if (magic_ != 0xfbfb) {
    throw std::runtime_error(
        "invalid eeprom magic number " + std::to_string(magic_));
  }
  if (version_ != 0x04) {
    throw std::runtime_error(
        "invalid eeprom version number " + std::to_string(version_) +
        " expected Version : 4");
  }
  dict_[FF_ID_OFFSET + 1] = std::to_string(version_);
}

/**
 *	eeprom::parseTlvField:
 *	   - parse the next TLV data and return it as tuple format: <int, int,
 *string>
 *
 *	return:  true if parsed the next TLV tuple succeed.
 *	         false either read the last TLV element or run out of buf_ data.
 *
 *  note:  various error checking is performed and throwing an exception
 *         if verification failed.
 */
bool EepromFboss::parseTlvField() {
  std::tuple<int, int, std::string> tlv = nextTlv(valFmtTbl_);
  auto id = std::get<0>(tlv);
  auto len = std::get<1>(tlv);
  auto val = std::get<2>(tlv);

  auto it = tlvTbl_.find(id);
  if (it != tlvTbl_.end()) {
    if (it->second.second != -1 && it->second.second != len) {
      throw std::runtime_error(
          "<" + it->second.first +
          "> TLV length mismatching.  read/expected: " + std::to_string(len) +
          "/" + std::to_string(it->second.second));
    }
    dict_[id] = val;

    if (id == mappingTbl_[CRC16] || runOutBuf()) {
      return false;
    }
  } else {
    throw std::runtime_error("undefined TLV field type " + std::to_string(id));
  }
  return true;
}

/**
 *	crc16Check - check the CRC16 value based on the CRC-CCITT
 *               algorithm for the data buffer
 *	@crcVal: the CRC16 value read from the eeprom field.
 *
 *	return:  true if the calaulated CRC value is the same as crcVal, false
 *           flase otherwise.
 */
bool EepromFboss::crc16Check(uint16_t crcVal) {
  uint16_t crc;
  /* the CRC16 calculation covers all eeprom data except the CRC16 TLV field. */
  crc = crc16_ccitt(0xffff, &buf_[0], bufSz_ - 4);
  crc = ~crc;
  if (crcVal != crc) {
    throw std::runtime_error(
        "CRC16 field value: " + int2HexStr<uint16_t>(crcVal) +
        ",  expected value: " + int2HexStr<uint16_t>(crc));
  }
  return crcVal == crc;
};

/**
 *	eeprom::getEepromData -	returns all eeprom fields data
 *	        (including fields	not present in the eeprom device).
 *   the field data are listed in fieldId's code order to make the output
 *consistency for all eeprom devices.
 *
 *	fieldId is defined in the WeutilInterface.h
 *
 */
std::map<fieldId, std::pair<std::string, std::string>>
EepromFboss::getEepromData() {
  std::map<fieldId, std::pair<std::string, std::string>> res;
  int iCode;

  if (dict_.empty()) {
    throw std::runtime_error("failed to read eeprom data for " + ePath_);
  }

  for (auto it = mappingTbl_.begin(); it != mappingTbl_.end(); it++) {
    iCode = it->second;
    if (dict_.find(iCode) != dict_.end()) {
      if (tlvField(iCode)) {
        if (iCode == mappingTbl_[CRC16]) {
          std::string hexStr = int2HexStr<uint16_t>(std::stoi(dict_[iCode]));
          res[it->first] = make_pair(tlvTbl_[iCode].first, hexStr);
        } else {
          res[it->first] = make_pair(tlvTbl_[iCode].first, dict_[iCode]);
        }
      } else {
        res[it->first] = make_pair(ffTbl_[iCode].first, dict_[iCode]);
      }
    }
  }
  return res;
}

bool EepromFboss::tlvField(int iCode) {
  return (((iCode & FF_ID_OFFSET) ^ FF_ID_OFFSET) == 0) ? false : true;
}

} // namespace weutil
