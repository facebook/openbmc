/*
 * HotPlugDetectionFile.h
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
#include <fstream>
#include <iostream>
#include <cstdio>

class HotPlugDetectionFile {
  private:
    std::string fileName_;              //HotPlug status is stored in this file

  public:
    /**
     * Constructor, Create Empty File
     */
    HotPlugDetectionFile(const std::string & fileName) {
      fileName_ = fileName;
      //Create empty file
      std::ofstream hpDetectFile(fileName_, std::ios::out | std::ios::trunc);
      hpDetectFile.close();
    }

    /**
     * Destructor, delete file
     */
    ~HotPlugDetectionFile() {
      //Remove File
      std::remove(fileName_.c_str());
    }

    /**
     * Write hot plug status to file
     */
    void writeHotPlugStatusToFile(int status) {
      std::ofstream hpDetectFile(fileName_, std::ios::out | std::ios::trunc);
      hpDetectFile << status;
      hpDetectFile.close();
    }

    /**
     * Return fileName
     */
    const std::string & getFileName() {
      return fileName_;
    }
};
