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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "WeutilInterface.h"

#define EEPROM_READ_SZ 2048

namespace weutil {
class Eeprom {
 public:
  explicit Eeprom(const std::string&, const uint16_t);
  virtual ~Eeprom() = default;
  virtual std::map<fieldId, std::pair<std::string, std::string>>
  getEepromData() = 0;

 protected:
  enum vFormat { VT_INT8, VT_INT16, VT_INT32, VT_STRMAC, VT_HEXMAC, VT_STR };

  std::tuple<int, int, std::string> nextTlv(std::map<int, vFormat>&);
  int readInt8();
  int readInt16();
  int readInt32();
  std::string readStr(const int& len);
  std::string readHexMac(const int& len);
  std::string readStrMac(const int& len);

  template <typename T>
  std::string int2HexStr(T i) {
    std::stringstream stream;
    stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex
           << i;
    return stream.str();
  }

  std::vector<uint8_t> buf_;
  uint16_t pos_; // the index of buf_[].
  int bufSz_;
  uint16_t eepromDataSz_; // valid eeprom data size, except the CRC16 field.
};

} // namespace weutil
