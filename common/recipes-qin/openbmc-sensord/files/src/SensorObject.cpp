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
#include <glog/logging.h>
#include "SensorObject.h"
#include "SensorDevice.h"
#include "SensorAttribute.h"

namespace openbmc {
namespace qin {

SensorAttribute* SensorObject::addAttribute(const std::string &name) {
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

const std::string& SensorObject::readAttrValue(const std::string &name)
    const {
  LOG(INFO) << "Reading the value of Attribute \n" << name << "\"";
  SensorAttribute* attr =
      static_cast<SensorAttribute*>(getReadableAttribute(name));
  return static_cast<SensorDevice*>(parent_)->readAttrValue(*this, *attr);
}

void SensorObject::writeAttrValue(const std::string &name,
                                  const std::string &value) {
  LOG(INFO) << "Writing the value of Attribute \"" << name << "\"";
  SensorAttribute* attr =
      static_cast<SensorAttribute*>(getWritableAttribute(name));
  static_cast<SensorDevice*>(parent_)->writeAttrValue(*this, *attr, value);
}

} // namespace qin
} // namepsace openbmc
