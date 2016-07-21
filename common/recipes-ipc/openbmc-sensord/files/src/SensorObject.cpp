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

SensorAttribute* SensorObject::addAttribute(const std::string &name) {
  LOG(INFO) << "Adding Attribute \"" << name << "\" to object \"" << name_
    << "\"";
  if (getAttribute(name) != nullptr) {
    LOG(ERROR) << "Adding duplicated Attribute \"" << name << "\"";
    return nullptr;
  }
  std::unique_ptr<SensorAttribute> upAttr(new SensorAttribute(name));
  SensorAttribute* attr = upAttr.get();
  attrMap_.insert(std::make_pair(name, std::move(upAttr)));
  return attr;
}

const std::string& SensorObject::readAttrValue(
                                   const std::string &attrName) const {
  LOG(INFO) << "Reading the value of Attribute \"" << attrName << "\"";
  SensorAttribute* attr;
  if ((attr = getAttribute(attrName)) == nullptr) {
    LOG(ERROR) << "Attribute \"" << attrName << "\" not found";
    throw std::invalid_argument("Attribute not found");
  }
  if (!attr->isReadable()) {
    LOG(ERROR) << "Attribute \"" << attrName << "\" does not support read in modes";
    throw std::system_error(EPERM,
                            std::system_category(),
                            "Attribute read not supported");
  }
  if (attr->isAccessible()) {
    LOG(INFO) << "Attribute \"" << attrName << "\" is accessible. "
      << "Reading value through SensorApi.";
    attr->setValue(sensorApi_.get()->readValue(*this, *attr));
  }
  return attr->getValue();
}

void SensorObject::writeAttrValue(const std::string &attrName,
                                  const std::string &value) {
  LOG(INFO) << "Writing the value of Attribute \"" << attrName << "\"";
  SensorAttribute* attr;
  if ((attr = getAttribute(attrName)) == nullptr) {
    LOG(ERROR) << "Attribute \"" << attrName << "\" not found";
    throw std::invalid_argument("Attribute not found");
  }
  if (!attr->isWritable()) {
    LOG(ERROR) << "Attribute \"" << attrName << "\" does not support write "
      << "in modes";
    throw std::system_error(EPERM,
                            std::system_category(),
                            "Attribute write not supported");
  }
  if (attr->isAccessible()) {
    LOG(INFO) << "Attribute \"" << attrName << "\""
      << " is accessible. Writing value through SensorApi.";
    sensorApi_.get()->writeValue(*this, *attr, value);
  }
  attr->setValue(value);
}

} // namespace ipc
} // namepsace openbmc
