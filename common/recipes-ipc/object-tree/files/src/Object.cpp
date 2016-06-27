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
#include <memory>
#include <glog/logging.h>
#include "Attribute.h"
#include "Object.h"

namespace openbmc {
namespace ipc {

Attribute* Object::getAttribute(const std::string &name,
                                const std::string &type) const {
  if (typeMap_.find(type) == typeMap_.end()) {
    return nullptr;
  }
  AttrMap* attrMap = typeMap_.find(type)->second.get();
  if (attrMap->find(name) == attrMap->end()) {
    return nullptr;
  }
  return attrMap->find(name)->second.get();
}

Attribute* Object::addAttribute(const std::string &name,
                                const std::string &type) {
  if (getAttribute(name, type) != nullptr) {
    LOG(ERROR) << "Adding duplicated attribute with name " << name
               << " and type " << type;
    return nullptr;
  }
  if (typeMap_.find(type) == typeMap_.end()) {
    std::unique_ptr<AttrMap> upAttrMap(new AttrMap());
    typeMap_.insert(std::make_pair(type, std::move(upAttrMap)));
  }
  AttrMap* attrMap = typeMap_.find(type)->second.get();
  std::unique_ptr<Attribute> upAttr(new Attribute(name, type));
  Attribute* attr = upAttr.get();
  attrMap->insert(std::make_pair(name, std::move(upAttr)));
  return attr;
}

void Object::deleteAttribute(const std::string &name, const std::string &type) {
  const Attribute* attr = getAttribute(name, type);
  if (attr == nullptr) {
    LOG(ERROR) << "Attribute with name " << name << " and type " << type
               << " not found";
    return;
  }
  AttrMap* attrMap = typeMap_.find(type)->second.get();
  attrMap->erase(name);
  if (attrMap->size() == 0) {
    typeMap_.erase(type);
  }
}

void Object::addChildObject(Object &child) {
  if (getChildObject(child.getName()) != nullptr) {
    LOG(ERROR) << "Adding duplicated Child";
    return;
  }
  if (child.getParent() != nullptr && child.getParent() != this) {
    LOG(ERROR) << "Child has a different parent";
    return;
  }
  childMap_.insert({child.getName(), &child});
  child.setParent(this);
}

Object* Object::removeChildObject(const std::string &name) {
  Object* child = getChildObject(name);
  if (child == nullptr) {
    LOG(ERROR) << "Object with name " << name << " not found";
    return nullptr;
  }
  if (child->getChildNumber() != 0) {
    LOG(ERROR) << "Object to be deleted has nonempty children";
    return nullptr;
  }
  childMap_.erase(name);
  child->setParent(nullptr);
  return child;
}

} // namepsace openbmc
} // namespace ipc
