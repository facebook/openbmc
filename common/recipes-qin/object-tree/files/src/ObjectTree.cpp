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
namespace qin {

ObjectTree::ObjectTree(const std::shared_ptr<Ipc> &ipc,
                       const std::string          &rootName) {
  if (ipc == nullptr) {
    LOG(ERROR) << "ipc cannot be nullptr";
    throw std::invalid_argument("Invalid ipc");
  }
  ipc_ = ipc;

  const std::string rootPath = getPath("", rootName);

  LOG(INFO) << "Adding root \"" << rootName << "\"";
  std::unique_ptr<Object> upObj(new Object(rootName));
  root_ = addObjectByPath(std::move(upObj), rootPath);

  LOG(INFO) << "Setting up the callbacks";
  ipc_.get()->onConnAcquired = onConnAcquiredCallBack;
  ipc_.get()->onConnLost     = onConnLostCallBack;
}

Object* ObjectTree::addObject(const std::string &name,
                              const std::string &parentPath) {
  LOG(INFO) << "Adding object \"" << name << "\" under path \""
    << parentPath << "\"";
  Object* parent = getParent(parentPath, name);
  const std::string path = getPath(parentPath, name);
  std::unique_ptr<Object> upObj(new Object(name, parent));
  return addObjectByPath(std::move(upObj), path);
}

Object* ObjectTree::addObject(std::unique_ptr<Object> upObj,
                              const std::string       &parentPath) {
  checkObject(upObj.get());
  const std::string &name = upObj.get()->getName();
  const std::string path = getPath(parentPath, name);
  LOG(INFO) << "Adding object \"" << name << "\" under path \""
    << parentPath << "\"";
  Object* parent = getParent(parentPath, name);
  parent->addChildObject(*(upObj.get()));
  return addObjectByPath(std::move(upObj), path);
}

void ObjectTree::deleteObjectByPath(const std::string &path) {
  ObjectMap::const_iterator it;
  if ((it = objectMap_.find(path)) == objectMap_.end()) {
    LOG(ERROR) << "Object to be deleted cannot be found at path \""
      << path << "\"";
    throw std::invalid_argument("Object not found");
  }
  Object* object = it->second.get();
  if (object == root_) {
    LOG(ERROR) << "Cannot delete root \"" << root_->getName() << "\"";
    throw std::invalid_argument("Error deleting root");
  }
  Object* parent = object->getParent();
  object = parent->removeChildObject(object->getName());
  if (object == nullptr) {
    LOG(ERROR) << "Failed to delete the object at \"" << path << "\"";
    throw std::invalid_argument("Error deleting object");
  }
  objectMap_.erase(it);
  ipc_.get()->unregisterObject(path);
}

Object* ObjectTree::getParent(const std::string &parentPath,
                              const std::string &name) const {
  Object* parent = getObject(parentPath);
  if (parent == nullptr) {
    LOG(ERROR) << "Parent object not found at \"" << parentPath << "\"";
    throw std::invalid_argument("Parent not found");
  }
  if (parent->getChildObject(name) != nullptr) {
    LOG(ERROR) << "Parent object has a child with the same name as \""
      << name << "\"";
    throw std::invalid_argument("Duplicated object name");
  }
  return parent;
}

void ObjectTree::checkObject(const Object* object) const {
  LOG(INFO) << "Checking validality of Object";
  if (object == nullptr) {
    LOG(ERROR) << "Empty object";
    throw std::invalid_argument("Empty object");
  }
  if (object->getChildCount() != 0) {
    LOG(ERROR) << "Non-empty children of the object \""
      << object->getName() << "\"";
    throw std::invalid_argument("Nonempty children");
  }
}

} // namespace qin
} // namespace openbmc
