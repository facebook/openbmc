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

#include "Cfg.h"

using json = nlohmann::json;

namespace weutil {
WeCfg::WeCfg() {
  std::ifstream f(CFG_FILE);
  json jCfg = json::parse(f);
  json jdev = jCfg["eeprom devices"];
  //  json pltName = jCfg["platform"];

  for (json::iterator it = jdev.begin(); it != jdev.end(); ++it) {
    assert(it->find("name") != it->end());
    assert(it->find("sysfs_path") != it->end());
    weCfgTbl_[(*it)["name"]] = (*it)["sysfs_path"];
  }
}

std::map<std::string, std::string> WeCfg::listEepromDevices() {
  std::map<std::string, std::string> res = weCfgTbl_;
  return res;
}

std::string WeCfg::eFormat(const std::string& ePath) {
  uint8_t header[4];
  std::ifstream file(ePath, std::ios::binary);
  size_t eepromSysfsSz;

  // check eeprom file size:
  file.seekg(0, std::ios::end);
  eepromSysfsSz = file.tellg();
  if (eepromSysfsSz <= 4) {
    throw std::runtime_error(
        "unrecongized eeorom format, data size is too small!");
  }
  file.seekg(0, std::ios::beg);
  file.read((char*)&header[0], 4);
  file.close();

  if (header[0] == 0xfb && header[1] == 0xfb && header[2] == 0x04) {
    return META_EEPROM_V4;
  } else if (header[0] == 0xfb && header[1] == 0xfb && header[2] == 0x05) {
    return META_EEPROM_V5;
  } else if (header[0] == 0xfb && header[1] == 0xfb && header[2] == 0x03) {
    return META_EEPROM_V3;
  } else if (
      (header[0] == '0' && header[1] == '0' && header[2] == '0' &&
      (header[3] == '2' || header[3] == '3')) || (header[0] == 0 &&
      header[1] == 0 && header[2] == 0 && header[3] == 3)) {
    assert(eepromSysfsSz > 15 * 1024);
    return ARISTA_PREFDL;
  }
  return "unrecognized eeprom data format";
}

std::optional<std::string> WeCfg::eepromNameToPath(const std::string& name) {
  auto it = weCfgTbl_.find(name);
  if (it == weCfgTbl_.end()) {
    return {};
  }
  return weCfgTbl_[name];
}
} // namespace weutil
