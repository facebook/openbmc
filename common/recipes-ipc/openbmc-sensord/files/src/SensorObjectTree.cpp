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

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <memory>
#include <glog/logging.h>
#include "SensorObjectTree.h"

namespace openbmc {
namespace ipc {


SensorObject* SensorObjectTree::addSensorObject(
                                      const std::string &name,
                                      const std::string &parentPath,
                                      std::unique_ptr<SensorApi> uSensorApi) {
  LOG(INFO) << "Adding object \"" << name << "\" under path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);
  std::unique_ptr<SensorObject> upObj(
      new SensorObject(name, std::move(uSensorApi), parent));
  return static_cast<SensorObject*>(addObjectByPath(std::move(upObj), path));
}

} // namespace ipc
} // namespace openbmc
