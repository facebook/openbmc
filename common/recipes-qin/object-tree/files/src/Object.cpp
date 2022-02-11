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
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <glog/logging.h>
#include "Attribute.h"
#include "Object.h"

namespace openbmc {
namespace qin {

Attribute* Object::getAttribute(const std::string &name) const {
  AttrMap::const_iterator it;
  if ((it = attrMap_.find(name)) == attrMap_.end()) {
    return nullptr;
  }
  return it->second.get();
}

const std::string& Object::readAttrValue(const std::string &name) const {
  LOG(INFO) << "Reading the value of Attribute \n" << name << "\"";
  Attribute* attr = getReadableAttribute(name);
  return attr->getValue();
}

void Object::writeAttrValue(const std::string &name,
                            const std::string &value) {
  LOG(INFO) << "Writing the value of Attribute \"" << name << "\"";
  Attribute* attr = getWritableAttribute(name);
  attr->setValue(value);
}

Attribute* Object::addAttribute(const std::string &name) {
  LOG(INFO) << "Adding Attribute \"" << name << "\" to object \"" << name_
    << "\"";
  if (getAttribute(name) != nullptr) {
    LOG(ERROR) << "Adding duplicated Attribute \"" << name << "\"";
    throw std::invalid_argument("Duplicated Attribute");
  }
  std::unique_ptr<Attribute> upAttr(new Attribute(name));
  Attribute* attr = upAttr.get();
  attrMap_.insert(std::make_pair(name, std::move(upAttr)));
  return attr;
}

void Object::deleteAttribute(const std::string &name) {
  const Attribute* attr = getAttribute(name);
  LOG(INFO) << "Deleting Attribute \"" << name << "\" from object \n"
    << name_ << "\n";
  if (attr == nullptr) {
    LOG(ERROR) << "Attribute \"" << name << "\" not found";
    throw std::invalid_argument("Attribute not found");
  }
  attrMap_.erase(name);
}

void Object::addChildObject(Object &child) {
  LOG(INFO) << "Adding child object \"" << child.getName() << "\"";
  if (getChildObject(child.getName()) != nullptr) {
    LOG(ERROR) << "Adding duplicated Child";
    throw std::invalid_argument("Duplicated child object");
  }
  if (child.getParent() != nullptr && child.getParent() != this) {
    LOG(ERROR) << "Child has a different parent";
    throw std::invalid_argument("Child has non-null parent");
  }
  childMap_.insert({child.getName(), &child});
  child.setParent(this);
}

Object* Object::removeChildObject(const std::string &name) {
  LOG(INFO) << "Removing child object \"" << name << "\"";
  Object* child = getChildObject(name);
  if (child == nullptr) {
    LOG(ERROR) << "Object with name " << name << " not found";
    throw std::invalid_argument("Child object not found");
  }
  if (child->getChildCount() != 0) {
    LOG(ERROR) << "Object to be deleted has non-empty children";
    throw std::invalid_argument("Object has non-empty children");
  }
  childMap_.erase(name);
  child->setParent(nullptr);
  return child;
}

std::string Object::getObjectPath() const {
  if (parent_ == nullptr){
    return "/" + name_;
  }
  else {
    return parent_->getObjectPath() + "/" + name_;
  }
}

nlohmann::json Object::dumpToJson() const {
  LOG(INFO) << "Dump object with name " << name_ << " into json";
  nlohmann::json dump = Object::dump();
  for (auto cit = childMap_.begin(); cit != childMap_.end(); cit++) {
    dump["childObjectNames"].push_back(cit->first);
  }
  dump["childObjectCount"] = getChildCount();
  return dump;
}

nlohmann::json Object::dumpToJsonRecursive() const {
  LOG(INFO) << "Dump object with name " << name_ << " into json";
  nlohmann::json dump = Object::dump();
  for (auto cit = childMap_.begin(); cit != childMap_.end(); cit++) {
    dump["childObjects"].push_back(cit->second->dumpToJsonRecursive());
  }
  dump["childObjectCount"] = getChildCount();
  return dump;
}

nlohmann::json Object::dump() const {
  nlohmann::json dump;
  dump["objectName"] = name_;
  dump["objectType"] = "Generic";
  if (parent_ == nullptr) {
    dump["parentName"] = nullptr;
  } else {
    dump["parentName"] = parent_->getName();
  }

  for (auto &it : attrMap_) {
    Attribute* attr = it.second.get();
    dump["attributes"].push_back(attr->dumpToJson());
  }
  dump["attrCount"] = getAttrCount();
  return dump;
}

} // namespace qin
} // namepsace openbmc
