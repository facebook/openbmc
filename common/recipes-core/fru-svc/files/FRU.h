/*
 * FRU.h
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
#include <object-tree/Object.h>
#include "FruIdAccessMechanism.h"

namespace openbmc {
namespace qin {

class FRU : public Object {
  private:
    std::vector<std::pair<std::string, std::string>> fruIdInfoList_; // To store fruId information
    std::unique_ptr<FruIdAccessMechanism> fruIdAccess_;

  public:
    /*
    * Contructor
    */
    FRU(const std::string &name, Object* parent, std::unique_ptr<FruIdAccessMechanism> fruIdAccess)
       : Object(name, parent){
      fruIdAccess_ = std::move(fruIdAccess);
      fruIdInfoList_ = fruIdAccess_.get()->getFruIdInfoList();
    }


    /*
     * Returns fruIdInfoList_
     */
    const std::vector<std::pair<std::string, std::string>> & getFruIdInfoList(){
      return fruIdInfoList_;
    }

    /*
     * Write fruId binary data from binFilePath
     * Update fruIdInfoList_ if write operation is successful
     * Returns status of operation
     */
    bool fruIdWriteBinaryData(const std::string & binFilePath) {
      if (fruIdAccess_.get()->writeBinaryData(binFilePath)){
        fruIdInfoList_ = fruIdAccess_.get()->getFruIdInfoList();
        return true;
      }
      else {
        return false;
      }
    }

    /*
     * Dumps fruId binary data at destFilePath
     * Returns status of operation
     */
    bool fruIdDumpBinaryData(const std::string & destFilePath) {
      return fruIdAccess_.get()->dumpBinaryData(destFilePath);
    }
};

} // namespace qin
} // namespace openbmc
