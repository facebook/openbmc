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

#include <errno.h>
#include <string>
#include <string.h>
#include <system_error>
#include <stdexcept>
#include <fstream>
#include <glog/logging.h>
#include "SensorAttribute.h"
#include "SensorSysfsApi.h"
#include "SensorObject.h"

namespace openbmc {
namespace qin {

const std::string SensorSysfsApi::readValue(const Object          &object,
                                            const SensorAttribute &attr)
    const {
  std::string path = fsPath_ + std::string("/") + attr.getAddr();
  std::fstream fs;
  LOG(INFO) << "Reading value from path " << path;
  fs.open(path, std::fstream::in);
  if (!fs.is_open()) {
    LOG(ERROR) << "Path " << path << " cannot be opened";
    throw std::system_error(errno, std::system_category(), strerror(errno));
  }
  std::string str;
  std::getline(fs, str);
  fs.close();
  return str;
}

void SensorSysfsApi::writeValue(const Object          &object,
                                const SensorAttribute &attr,
                                const std::string     &value) {
  std::string path = fsPath_ + std::string("/") + attr.getAddr();
  std::fstream fs;
  LOG(INFO) << "Writing value " << value <<" to path " << path;
  fs.open(path, std::fstream::out);
  if (!fs.is_open()) {
    LOG(ERROR) << "Path " << path << " cannot be opened";
    throw std::system_error(errno, std::system_category(), strerror(errno));
  }
  fs << value;
  fs.close();
}

} // namespace qin
} // namespace openbmc
