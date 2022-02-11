/*
 * FruIdAccessMechanism.h
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
#include <vector>

namespace openbmc {
namespace qin {

/* FruIdAccessMechanism : abstract class for FruIdAccess
 */
class FruIdAccessMechanism {
  public:

    /*
     * Returns vector represensation FruId information
     */
    virtual std::vector<std::pair<std::string, std::string>> getFruIdInfoList() = 0;

    /*
     * Write fruId binary data from binFilePath
     * Returns status of operation
     */
    virtual bool writeBinaryData(const std::string & binFilePath) = 0;

    /*
     * Dump FruId information to destFilePath
     * Returns status of operation
     */
    virtual bool dumpBinaryData(const std::string & destFilePath) = 0;
};

} // namespace qin
} // namespace openbmc
