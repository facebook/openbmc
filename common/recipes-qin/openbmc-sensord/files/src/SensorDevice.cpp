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
#include "SensorDevice.h"
#include "SensorApi.h"
#include "SensorAttribute.h"

namespace openbmc {
namespace qin {

SensorAttribute* SensorDevice::addAttribute(const std::string &name) {
  LOG(INFO) << "Adding Attribute \"" << name << "\" to object \"" << name_
    << "\"";
  if (getAttribute(name) != nullptr) {
    LOG(ERROR) << "Adding duplicated Attribute \"" << name << "\"";
    throw std::invalid_argument("Duplicated Attribute");
  }
  std::unique_ptr<SensorAttribute> upAttr(new SensorAttribute(name));
  SensorAttribute* attr = upAttr.get();
  attrMap_.insert(std::make_pair(name, std::move(upAttr)));
  return attr;
}

const std::string& SensorDevice::readAttrValue(const Object    &object,
                                               SensorAttribute &attr) const {
  LOG(INFO) << "SensorDevice \"" << name_ << "\" reading Attribute " << "\""
    << attr.getName() << "\" value of Object \"" << object.getName() << "\"";
  DCHECK(attr.isReadable()) << "SensorAttribute \"" << attr.getName()
    << "\" is not readable";
  attr.setValue(sensorApi_.get()->readValue(object, attr));
  return attr.getValue();
}

const std::string& SensorDevice::readAttrValue(
                                   const std::string &name) const {
  LOG(INFO) << "Reading the value of Attribute \"" << name << "\"";
  SensorAttribute* attr =
      static_cast<SensorAttribute*>(getReadableAttribute(name));
  return readAttrValue(*this, *attr);
}

void SensorDevice::writeAttrValue(const Object      &object,
                                  SensorAttribute   &attr,
                                  const std::string &value) {
  LOG(INFO) << "SensorDevice \"" << name_ << "\" writing Attribute " << "\""
    << attr.getName() << "\" value \"" << value << "\" of Object \""
    << object.getName() << "\"";
  DCHECK(attr.isWritable()) << "SensorAttribute \"" << attr.getName()
    << "\" is not writable";
  if (attr.isAccessible()) {
    sensorApi_.get()->writeValue(object, attr, value);
  }
  attr.setValue(value);
}

void SensorDevice::writeAttrValue(const std::string &name,
                                  const std::string &value) {
  LOG(INFO) << "Writing the value of Attribute \"" << name << "\"";
  SensorAttribute* attr =
      static_cast<SensorAttribute*>(getWritableAttribute(name));
  writeAttrValue(*this, *attr, value);
}


} // namepsace openbmc
} // namespace qin
