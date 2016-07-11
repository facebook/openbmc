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
  const std::string path = ipc_.get()->getPath(parentPath, name);
  if (!ipc_->isPathAllowed(path)) {
    LOG(ERROR) << "Sensor Object name of " << name
      << "does not match the ipc rule.";
    throw std::invalid_argument("Invalid object name");
  }

  Object* parent = getObject(parentPath);
  if (parent == nullptr) {
    LOG(WARNING) << "Parent object not found at path " << parentPath;
    return nullptr;
  }
  if (parent->getChildObject(name) != nullptr) {
    LOG(WARNING) << "Parent object has a child with the same name as" << name;
    return nullptr;
  }

  std::unique_ptr<SensorObject> upObj(new SensorObject(name,
                                                       std::move(uSensorApi),
                                                       parent));
  SensorObject* object = upObj.get();
  ipc_->registerObject(path, object);
  objectMap_.insert(std::make_pair(path, std::move(upObj)));
  return object;
}

} // namespace ipc
} // namespace openbmc
