/*
 * FruJsonParser.h
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

#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include "FruObjectTree.h"
#include "FruIdAccessI2CEEPROM.h"

namespace openbmc {
namespace qin {

/**
 * Json parser to parse the json file containing the information for
 * building the fru object tree. It is based on nlohmann json.
 * The instace of FruObjectTree should be passed in for the building
 * process.
 */
class FruJsonParser {
  public:

    /**
     * A helper function that adds the specified FRU into the
     * fru tree.
     */
    static void parseFRU(const nlohmann::json &jObject,
                         FruObjectTree     &fruTree,
                         const std::string    &parentPath) {
      const std::string &name = jObject.at("objectName");
      LOG(INFO) << "Parsing the FRU \"" << name <<
        "\" under the parent path \"" << parentPath << "\"";

      Object* object = nullptr;

      if (jObject.find("fruIdAccess") == jObject.end()) {
        LOG(WARNING) << "FruId Access Mechanism for " << name << " not defined";
        throw std::runtime_error("FruId Access Mechanism for " + name + " not defined");
      }
      else {
        // Decode access mechanism
        const nlohmann::json &fruIdAccess = jObject.at("fruIdAccess");
        const std::string &type = fruIdAccess.at("type");

        if (type.compare("I2CEEPROM") == 0) {
          const std::string &eepromPath = fruIdAccess.at("eepromPath");
          std::unique_ptr<FruIdAccessMechanism> upFruIdAccess(new FruIdAccessI2CEEPROM(eepromPath));
          object = fruTree.addFRU(name, parentPath, std::move(upFruIdAccess));
        } else {
          LOG(ERROR) << "Specified fruid access mechanism " << type;
          throw std::invalid_argument("Invalid fruid access mechanism");
        }
      }

      if (object == nullptr) {
        throwObjectJsonConfliction(name, parentPath);
      }
    }

  private:

    static void throwObjectJsonConfliction(const std::string &name,
                                           const std::string &parentPath) {
      // there must be duplicate object
      LOG(ERROR) << "Object \"" << name << "\" cannot be added "
        "under parent object at \"" << parentPath << "\"";
      throw std::invalid_argument("JSON conflicts with the fru tree");
    }
};

} // namespace qin
} // namespace openbmc
