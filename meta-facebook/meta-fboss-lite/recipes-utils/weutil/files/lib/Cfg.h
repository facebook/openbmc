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
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define CFG_FILE "/etc/weutil/eeprom.json"
#define META_EEPROM_V4 "META-FBOSS EEPROM Version 4"
#define ARISTA_PREFDL "Arista Rackhawk Prefdl"
#define ARISTA_EEPROM_OFFSET 15 * 1024

namespace weutil {

class WeCfg {
 public:
  WeCfg();
  std::map<std::string, std::string> listEepromDevices();
  std::optional<std::string> eepromNameToPath(const std::string& name);
  std::string eFormat(const std::string& ePath);

 private:
  // mapping:  name --> sysfs_path
  std::map<std::string, std::string> weCfgTbl_;
};

} // namespace weutil
