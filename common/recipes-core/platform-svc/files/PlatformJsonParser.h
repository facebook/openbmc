/*
 * PlatformJsonParser.h
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
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include "PlatformObjectTree.h"

namespace openbmc {
namespace qin {

/**
 * Json parser to parse the json file containing the information for
 * building the sensor object tree. It is based on nlohmann json.
 * The instace of PlatformObjectTree should be passed in for the building
 * process.
 */
class PlatformJsonParser {
  public:
    typedef std::function<void(const nlohmann::json&,
                               PlatformObjectTree&,
                               const std::string&)> ObjectParser;

    // a mapping from the strings of object types defined in JSON schema
    // to the corresponding object parser
    static std::unordered_map<std::string, ObjectParser> objectParserMap;

    /**
     * Parse the json file and build up a branch of PlatformObjectTree
     * into the specified tree under the specified parent object at the
     * parentPath. Notice that the Sensor will be treated as the
     * leaf and that it should not have children.
     */
    static void parse(const std::string   &jsonFile,
                      PlatformObjectTree  &platformTree,
                      const std::string   &parentPath);

    /**
     * Parse the specified json object into the platformTree under the object at
     * parentPath. The json object is assumed to contain the declaration of
     * ONE object to be added under parentPath
     */
    static void parseObject(const nlohmann::json &jObject,
                            PlatformObjectTree   &platformTree,
                            const std::string    &parentPath);

  private:

    static void throwObjectJsonConfliction(const std::string &name,
                                           const std::string &parentPath) {
      // there must be duplicate object
      LOG(ERROR) << "Object \"" << name << "\" cannot be added "
        "under parent object at \"" << parentPath << "\"";
      throw std::invalid_argument("JSON conflicts with the sensor tree");
    }

    static void throwAttrJsonConfliction(const std::string &attrName,
                                         const std::string &objectName) {
      // there must be duplicate attribute
      LOG(ERROR) << "Attribute \"" << attrName
        << "\" cannot be added into Object \"" << objectName << "\"";
      throw std::invalid_argument("JSON conflicts with the sensor tree");
    }

    /**
     * A helper function that adds the Platform Service into the
     * platform tree and that proceeds to add the attributes under it.
     */
     static void parsePlatformService(const nlohmann::json &jObject,
                                      PlatformObjectTree   &platformTree,
                                      const std::string    &parentPath);

    /**
     * A helper function that adds the specified FRU into the
     * platform tree.
     */
    static void parseFRU(const nlohmann::json &jObject,
                         PlatformObjectTree   &platformTree,
                         const std::string    &parentPath);

    /**
     * A helper function that adds the specified Sensor into the
     * platform tree
     */
    static void parseSensor(const nlohmann::json &jObject,
                            PlatformObjectTree   &platformTree,
                            const std::string    &parentPath);
};

} // namespace qin
} // namespace openbmc
