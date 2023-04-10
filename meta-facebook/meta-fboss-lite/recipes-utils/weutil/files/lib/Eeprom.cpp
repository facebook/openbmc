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

#include "Eeprom.h"

using namespace std;

namespace weutil {

Eeprom::Eeprom(const std::string& eFile, const uint16_t off) : pos_(0) {
  std::ifstream file(eFile, std::ios::binary);
  // get eeprom file size:
  file.seekg(0, std::ios::end);
  bufSz_ = file.tellg();
  assert(bufSz_ - off > 0);
  // meta-eeprom data size
  bufSz_ = bufSz_ - off;
  file.seekg(off, std::ios::beg);

  // read the data:
  buf_.resize(bufSz_);
  file.read((char*)&buf_[0], bufSz_);
  file.close();
};

bool Eeprom::runOutBuf() {
  return pos_ >= bufSz_;
}

/*
 * TODO: take care of endianness
 * lucky, both armv7 and X86 are little-endian.
 */
int Eeprom::readInt32() {
  int res = 0;
  if (pos_ + 4 > bufSz_) {
    throw std::runtime_error(
        "32 over the boundary of eeprom buf size " + std::to_string(pos_ + 4) +
        "/" + std::to_string(bufSz_));
  }
  res = readInt8();
  res = readInt8() << 8 | res;
  res = readInt8() << 16 | res;
  res = readInt8() << 24 | res;
  return res;
};

/*
 * TODO: take care of endianness
 * lucky, both armv7 and X86 are little-endian.
 */
int Eeprom::readInt16() {
  int res = 0;
  if (pos_ + 2 > bufSz_) {
    throw std::runtime_error(
        "16 over the boundary of eeprom buf size " + std::to_string(pos_ + 4) +
        "/" + std::to_string(bufSz_));
  }
  res = readInt8();
  res = readInt8() << 8 | res;
  return res & 0xffff;
};

int Eeprom::readInt8() {
  int res = 0;
  if (pos_ + 1 > bufSz_) {
    throw std::runtime_error(
        "8 over the boundary of eeprom buf size " + std::to_string(pos_ + 4) +
        "/" + std::to_string(bufSz_));
  }
  res = buf_[pos_++];
  return res & 0xff;
};

std::string Eeprom::readHexMac(const int& len) {
  std::string macStr;

  if (len != 6) {
    throw std::runtime_error(
        "invalid MAC address length: " + std::to_string(len));
  }

  for (int i = 0; i < 6; i++) {
    int a = readInt8();
    int hv = (a & 0xf0) >> 4;
    int lv = a & 0xf;
    macStr += (hv < 10) ? '0' + hv : 'A' + hv - 10;
    macStr += (lv < 10) ? '0' + lv : 'A' + lv - 10;
    macStr += ':';
  }
  macStr.pop_back();
  return macStr;
}

std::string Eeprom::readStrMac(const int& len) {
  std::string mac;
  mac = readStr(len);
  return mac.substr(0, 2) + ":" + mac.substr(2, 2) + ":" + mac.substr(4, 2) +
      ":" + mac.substr(6, 2) + ":" + mac.substr(8, 2) + ":" + mac.substr(10, 2);
}

std::string Eeprom::readStr(const int& len) {
  int i = 0;
  std::string res;

  if (len <= 0) {
    throw std::runtime_error(
        "str over the boundary of eeprom buf size " + std::to_string(pos_ + 4) +
        "/" + std::to_string(bufSz_));
  }

  for (i = 0; i < len && pos_ < bufSz_; i++) {
    res = res + (char)buf_[pos_++];
  }

  if (pos_ == bufSz_ && i < len) {
    throw std::runtime_error("run out eeprom buf size ");
  }
  return res;
};

std::tuple<int, int, std::string> Eeprom::nextTlv(
    std::map<int, vFormat>& valFmt) {
  int t, l;
  std::string v;
  std::stringstream stream;
  t = readInt8();
  l = readInt8();

  if (valFmt.find(t) == valFmt.end()) {
    throw std::runtime_error(
        "TLV type (" + std::to_string(t) + " )'s valueFotmat is NOT defined.");
  };
  switch (valFmt[t]) {
    case VT_INT8:
      v = std::to_string(readInt8());
      break;
    case VT_INT16:
      stream << readInt16();
      v = stream.str();
      break;
    case VT_INT32:
      stream << readInt32();
      v = stream.str();
      break;
    case VT_STRMAC:
      v = readStrMac(l);
      break;
    case VT_HEXMAC:
      v = readHexMac(l);
      break;
    case VT_STR:
      v = readStr(l);
      break;
  }
  return make_tuple(t, l, v);
};

} // namespace weutil
