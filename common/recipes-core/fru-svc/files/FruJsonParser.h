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
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include "FruObjectTree.h"
using namespace openbmc::qin;

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

      std::string id;

      //todo: add parsing of fruId access mechanism
      try {
        id = jObject.at("id");
      }
      catch (const std::out_of_range& oor) {
        //Ignore if id is not mentioned
      }

      Object* object = fruTree.addFRU(name, parentPath, id);

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
