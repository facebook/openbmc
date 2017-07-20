/*
 * FruObjectTree.cpp
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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
#include <glog/logging.h>
#include "FruObjectTree.h"
#include "DBusFruInterface.h"
#include "DBusFruServiceInterface.h"

namespace openbmc {
namespace qin {

static DBusFruInterface fruInterface;
static DBusFruServiceInterface fruServiceInterface;

static int parseInt(std::string str) {
  if ((str.length() > 2) && (str.at(0) == '0' && str.at(1) == 'x')){
    return std::stoi (str,nullptr,16);
  }
  else {
    return std::stoi (str);
  }
}

FruService* FruObjectTree::addFruService(const std::string &name,
                                         const std::string &parentPath) {
  LOG(INFO) << "Adding FruService \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);
  std::unique_ptr<FruService> upDev(new FruService(name, parent));
  return static_cast<FruService*>(addObjectByPath(std::move(upDev), path, fruServiceInterface));
}

FRU* FruObjectTree::addFRU(const std::string                     &name,
                           const std::string                     &parentPath,
                           std::unique_ptr<FruIdAccessMechanism> fruIdAccess) {
  LOG(INFO) << "Adding FRU \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);

  std::unique_ptr<FRU> upDev(new FRU(name, parent, std::move(fruIdAccess)));
  return static_cast<FRU*>(addObjectByPath(std::move(upDev), path, fruInterface));
}

} // namespace qin
} // namespace openbmc
