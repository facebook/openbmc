/*
 * FruIdAccessI2CEEPROM.h
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
#include "FruIdAccessMechanism.h"

namespace openbmc {
namespace qin {

class FruIdAccessI2CEEPROM : public FruIdAccessMechanism {
  private:
    std::string eepromPath_;               //path for eeprom file
    static const int FRUID_SIZE = 512;     //FRUID size in eeprom file

  public:
    /*
     * Constructor
     */
    FruIdAccessI2CEEPROM(const std::string &eepromPath) {
      this->eepromPath_ = eepromPath;
    }

    /*
     * Parses FruId information from eeprom file at eepromPath_
     * and returns vector represensation FruId information
     */
    std::vector<std::pair<std::string, std::string>> getFruIdInfoList() override;

    /*
     * Validate FruId information at binFilePath
     * On successful validation update binary data at eepromPath_
     * Returns status of operation
     */
    virtual bool writeBinaryData(const std::string & binFilePath) override;

    /*
     * Dump FruId information from eeprom file at eepromPath_ to destFilePath
     * Returns status of operation
     */
    virtual bool dumpBinaryData(const std::string & destFilePath) override;
};

} // namespace qin
} // namespace openbmc
