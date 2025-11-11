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

#include <stdint.h>
#include <syslog.h>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#include "Cfg.h"
#include "FbossEepromParser.h"
#include "WeutilInterface.h"

using namespace weutil;
using namespace facebook::fboss::platform;

static std::unique_ptr<WeCfg> cfg = std::make_unique<WeCfg>();

std::vector<std::pair<std::string, std::string>> eepromParseNew(
    const std::string& eepromDeviceName) {
  uint16_t off = 0;
  std::string ePath = cfg->eepromNameToPath(eepromDeviceName).value_or("");
  std::string eepromFmt = cfg->eFormat(ePath);
  off = (ARISTA_PREFDL == eepromFmt) ? ARISTA_EEPROM_OFFSET : 0;
  return FbossEepromParser(ePath, off).getContents();
}

std::map<std::string, std::string> listEepromDevices() {
  return cfg->listEepromDevices();
}

/*
 *  return epprom sysfs path.
 */
std::optional<std::string> eepromNameToPath(const std::string& devName) {
  return cfg->eepromNameToPath(devName);
}

std::string eepromFormat(std::string& eName) {
  std::string ePath = cfg->eepromNameToPath(eName).value_or("");
  if (ePath != "")
    return cfg->eFormat(ePath);
  else
    return "invalid eeprom device name";
}
