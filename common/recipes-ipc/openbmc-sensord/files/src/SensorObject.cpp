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
#include <system_error>
#include <glog/logging.h>
#include <object-tree/Attribute.h>
#include "SensorObject.h"
#include "SensorApi.h"
#include "SensorAttribute.h"

namespace openbmc {
namespace ipc {

SensorAttribute* SensorObject::addAttribute(const std::string &name,
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
  std::unique_ptr<SensorAttribute> upAttr(new SensorAttribute(name, type));
  SensorAttribute* attr = upAttr.get();
  attrMap->insert(std::make_pair(name, std::move(upAttr)));
  return attr;
}

const std::string& SensorObject::readAttrValue(
    const std::string &attrName,
    const std::string &attrType) const {
  LOG(INFO) << "Reading the value of Attribute name " << attrName << " type "
    << attrType;
  SensorAttribute* attr;
  if ((attr = getSensorAttribute(attrName, attrType)) == nullptr) {
    LOG(ERROR) << "Attribute of type " << attrType << " name " << attrName
      << " not found";
    throw std::invalid_argument("Attribute not found");
  }
  if (!attr->isReadable()) {
    LOG(ERROR) << "Attribute of type " << attrType << " name " << attrName
      << " does not support read in modes";
    throw std::system_error(EPERM,
                            std::system_category(),
                            "Attribute read not supported");
  }
  if (attr->isAccessible()) {
    LOG(INFO) << "Attribute name " << attrName << " type " << attrType
      << " is accessible. Reading value through SensorApi.";
    attr->setValue(sensorApi_.get()->readValue(*this, *attr));
  }
  return attr->getValue();
}

void SensorObject::writeAttrValue(const std::string &value,
                                  const std::string &attrName,
                                  const std::string &attrType) {
  LOG(INFO) << "Writing the value of Attribute name " << attrName << " type "
    << attrType;
  SensorAttribute* attr;
  if ((attr = getSensorAttribute(attrName, attrType)) == nullptr) {
    LOG(ERROR) << "Attribute of type " << attrType << " name " << attrName
      << " not found";
    throw std::invalid_argument("Attribute not found");
  }
  if (!attr->isWritable()) {
    LOG(ERROR) << "Attribute of type " << attrType << " name " << attrName
      << " does not support write in modes";
    throw std::system_error(EPERM,
                            std::system_category(),
                            "Attribute write not supported");
  }
  if (attr->isAccessible()) {
    LOG(INFO) << "Attribute name " << attrName << " type " << attrType
      << " is accessible. Writing value through SensorApi.";
    sensorApi_.get()->writeValue(value, *this, *attr);
  }
  attr->setValue(value);
}

} // namespace ipc
} // namepsace openbmc
