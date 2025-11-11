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
#include <optional>
#include <string>
#include <vector>

#define DEFAULT_EEPROM_NAME "chassis_eeprom"

std::map<std::string, std::string> listEepromDevices();
std::optional<std::string> eepromNameToPath(const std::string& devName);
std::vector<std::pair<std::string, std::string>> eepromParseNew(
    const std::string& eepromDeviceName);
std::string eepromFormat(std::string& eName);
