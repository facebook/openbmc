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
#include "Eeprom.h"
#include "FbossEeprom.h"
#include "WeutilInterface.h"
#include "FbossEepromParser.h"

using namespace weutil;
static std::unique_ptr<WeCfg> cfg = std::make_unique<WeCfg>();

std::vector<std::pair<std::string, std::string>> eepromParseNew(
    const std::string& eepromDeviceName) {
  uint16_t off = 0;
  facebook::fboss::platform::FbossEepromParser newParser;
  std::string ePath = cfg->eepromNameToPath(eepromDeviceName).value_or("");
  std::string eepromFmt = cfg->eFormat(ePath);
  off = (ARISTA_PREFDL == eepromFmt) ? ARISTA_EEPROM_OFFSET : 0;
  return newParser.getEeprom(ePath, off);
}

std::map<fieldId, std::pair<std::string, std::string>> eepromParse(
    const std::string& eepromDeviceName) {
  uint16_t off = 0;
  std::map<fieldId, std::pair<std::string, std::string>> res;
  std::string ePath = cfg->eepromNameToPath(eepromDeviceName).value_or("");
  std::string eepromFmt = cfg->eFormat(ePath);

  if (META_EEPROM_V4 == eepromFmt || ARISTA_PREFDL == eepromFmt) {
    // they all use the meta-eeprom-v4 format, but has different offset value.
    off = (ARISTA_PREFDL == eepromFmt) ? ARISTA_EEPROM_OFFSET : 0;
    std::unique_ptr<EepromFboss> ep;
    ep = std::make_unique<EepromFboss>(ePath, off);
    res = ep->getEepromData();
  } else {
    throw std::runtime_error("unsupported eeprom format ");
  }
  return res;
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
