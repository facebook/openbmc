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

class FruIdAccessI2CEEPROM : public FruIdAccessMechanism {
  private:
    std::string eepromPath_;               //path for eeprom file
    static const int FRUID_SIZE = 256;     //FRUID size in eeprom file

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
};
