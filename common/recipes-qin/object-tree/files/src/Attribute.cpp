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

#include <string>
#include <unordered_map>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include "Attribute.h"

namespace openbmc {
namespace qin {

std::unordered_map<unsigned int, const std::string> Attribute::modesStringMap =
  {
    {RO, "RO"},
    {WO, "WO"},
    {RW, "RW"}
  };

std::unordered_map<std::string, const unsigned int> Attribute::stringModesMap =
  {
    {"RO", RO},
    {"WO", WO},
    {"RW", RW}
  };

nlohmann::json Attribute::dumpToJson() const {
  LOG(INFO) << "Dumpping the info for Attribute \"" << name_ << "\"";
  nlohmann::json dump;
  dump["name"] = name_;
  dump["value"] = value_;
  dump["modes"] = modesStringMap.at(modes_);
  return dump;
}

} // namespace qin
} // namespace openbmc
