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
#include "ObjectTree.h"
#include "Object.h"

namespace openbmc {
namespace ipc {

ObjectTree::ObjectTree(const std::shared_ptr<Ipc> &ipc,
                       const std::string          &rootName) {
  ipc_ = ipc;
  const std::string rootPath = ipc_.get()->getPath("", rootName);
  if (!ipc_.get()->isPathAllowed(rootPath)) {
    LOG(ERROR) << "Root path of " << rootPath
      << "does not match the ipc rule.";
    throw std::invalid_argument("Invalid object path");
  }

  LOG(INFO) << "Adding root with name " << rootName;
  std::unique_ptr<Object> upObj(new Object(rootName));
  root_ = upObj.get();
  ipc_.get()->registerObject(rootPath, root_);
  objectMap_.insert(std::make_pair(rootPath, std::move(upObj)));
}

Object* ObjectTree::addObject(const std::string &name,
                              const std::string &parentPath) {
  const std::string path = ipc_.get()->getPath(parentPath, name);
  if (!ipc_.get()->isPathAllowed(path)) {
    LOG(ERROR) << "Object path of " << path << "does not match the ipc rule.";
    throw std::invalid_argument("Invalid object path");
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

  std::unique_ptr<Object> upObj(new Object(name, parent));
  Object* object = upObj.get();
  ipc_.get()->registerObject(path, object);
  objectMap_.insert(std::make_pair(path, std::move(upObj)));
  return object;
}

void ObjectTree::deleteObjectByPath(const std::string &path) {
  ObjectMap::const_iterator it;
  if ((it = objectMap_.find(path)) == objectMap_.end()) {
    LOG(ERROR) << "Object to be deleted cannot be found at path "
               << path;
    return;
  }
  Object* object = it->second.get();
  if (object == root_) {
    LOG(ERROR) << "Cannot delete root.";
    return;
  }
  Object* parent = object->getParent();
  object = parent->removeChildObject(object->getName());
  if (object == nullptr) {
    // may not be removed since the child object could have children
    LOG(ERROR) << "Failed to delete the object with path " << path;
    return;
  }
  ipc_.get()->unregisterObject(path);
  objectMap_.erase(it);
}

} // namespace ipc
} // namespace openbmc
