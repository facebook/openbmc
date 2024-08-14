/*
 * Copyright 2024-present Facebook. All Rights Reserved.
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

#pragma once

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace cper
{

constexpr auto CPER_DUMP_PATH = "/mnt/data/faultlog/cper/";
constexpr auto CPER_CONVERT = "/usr/bin/cper-convert";
constexpr auto CPER_DUMP_MAX_LIMIT = 1024;

constexpr auto INVALID_ID = 0xFF;
constexpr auto INVALID_ERROR_DESC = "None";

using SectionHandler = int(*)(const nlohmann::ordered_json&, const nlohmann::ordered_json&, std::string&);
using SectionHandlerMap = std::unordered_map<std::string, SectionHandler>;

enum CperHandleCodes
{
  CPER_HANDLE_SUCCESS = 0,
  CPER_HANDLE_FAIL
};

enum UnifiedErrorType
{
  UNIFIED_PCIE_ERR  = 0x20,
  UNIFIED_MEM_ERR   = 0x21,
  UNIFIED_PROC_ERR  = 0x2F,
};

std::string formatHex(uint32_t value, int width = 2);
int addOemSectionHandlerMap(SectionHandlerMap& sectionHandlerMap);
int parseCperFile(const std::string& file, std::vector<std::string>& results);
int createCperDumpEntry(const uint8_t& payloadId, 
                        const uint8_t* data, const size_t& dataSize);

} // namespace cper