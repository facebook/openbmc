/*
 * FruIdAccessI2CEEPROM.cpp
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

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <glog/logging.h>
#include <openbmc/fruid.h>
#include "FruIdAccessI2CEEPROM.h"

namespace openbmc {
namespace qin {

std::vector<std::pair<std::string, std::string>> FruIdAccessI2CEEPROM::getFruIdInfoList() {
  std::vector<std::pair<std::string, std::string>> fruIdInfoList;
  std::ifstream eepromFile;

  //open eeprom file
  eepromFile.open(eepromPath_, std::ios::in | std::ios::binary | std::ios::ate);

  if (eepromFile.is_open()) {
    //Get size of eepromFile
    int size = eepromFile.tellg();

    if (size >= FRUID_SIZE) {
      unsigned char fruIdData[FRUID_SIZE] = {0};
      fruid_info_t fruid;

      //Get binary data from eepromFile
      eepromFile.seekg (0, std::ios::beg);
      eepromFile.read ((char*)fruIdData, FRUID_SIZE);

      // parse fruId from eepromFile dump
      fruid_parse_eeprom(fruIdData, FRUID_SIZE, &fruid);

      //decode struct fruid and stored it in map
      if (fruid.chassis.flag == 1) {
        fruIdInfoList.push_back({"Chassis Type", std::string(fruid.chassis.type_str)});
        fruIdInfoList.push_back({"Chassis Part Number", std::string(fruid.chassis.part)});
        fruIdInfoList.push_back({"Chassis Serial Number", std::string(fruid.chassis.serial)});
        if (fruid.chassis.custom1 != nullptr) {
          fruIdInfoList.push_back({"Chassis Custom Data 1", std::string(fruid.chassis.custom1)});
        }
        if (fruid.chassis.custom2 != nullptr) {
          fruIdInfoList.push_back({"Chassis Custom Data 2", std::string(fruid.chassis.custom2)});
        }
        if (fruid.chassis.custom3 != nullptr) {
          fruIdInfoList.push_back({"Chassis Custom Data 3", std::string(fruid.chassis.custom3)});
        }
        if (fruid.chassis.custom4 != nullptr) {
          fruIdInfoList.push_back({"Chassis Custom Data 4", std::string(fruid.chassis.custom4)});
        }
      }
      else {
        LOG(INFO) << "Chassis Info not set";
      }

      if (fruid.board.flag == 1) {
        fruIdInfoList.push_back({"Board Mfg Date", std::string(fruid.board.mfg_time_str)});
        fruIdInfoList.push_back({"Board Manufacturer", std::string(fruid.board.mfg)});
        fruIdInfoList.push_back({"Board Product", std::string(fruid.board.name)});
        fruIdInfoList.push_back({"Board Serial", std::string(fruid.board.serial)});
        fruIdInfoList.push_back({"Board Part Number", std::string(fruid.board.part)});
        fruIdInfoList.push_back({"Board Fru Id", std::string(fruid.board.fruid)});
        if (fruid.board.custom1 != nullptr) {
          fruIdInfoList.push_back({"Board Custom Data 1", std::string(fruid.board.custom1)});
        }
        if (fruid.board.custom2 != nullptr) {
          fruIdInfoList.push_back({"Board Custom Data 2", std::string(fruid.board.custom2)});
        }
        if (fruid.board.custom3 != nullptr) {
          fruIdInfoList.push_back({"Board Custom Data 3", std::string(fruid.board.custom3)});
        }
        if (fruid.board.custom4 != nullptr) {
          fruIdInfoList.push_back({"Board Custom Data 4", std::string(fruid.board.custom4)});
        }

      }
      else {
        LOG(INFO) << "Board Info not set";
      }

      if (fruid.product.flag == 1) {
        fruIdInfoList.push_back({"Product Manufacturer", std::string(fruid.product.mfg)});
        fruIdInfoList.push_back({"Product Name", std::string(fruid.product.name)});
        fruIdInfoList.push_back({"Product Part Number", std::string(fruid.product.part)});
        fruIdInfoList.push_back({"Product Version", std::string(fruid.product.version)});
        fruIdInfoList.push_back({"Product Serial", std::string(fruid.product.serial)});
        fruIdInfoList.push_back({"Product Asset Tag", std::string(fruid.product.asset_tag)});
        fruIdInfoList.push_back({"Product Fru Id", std::string(fruid.product.fruid)});
        if (fruid.product.custom1 != nullptr) {
          fruIdInfoList.push_back({"Product Custom Data 1", std::string(fruid.product.custom1)});
        }
        if (fruid.product.custom2 != nullptr) {
          fruIdInfoList.push_back({"Product Custom Data 2", std::string(fruid.product.custom2)});
        }
        if (fruid.product.custom3 != nullptr) {
          fruIdInfoList.push_back({"Product Custom Data 3", std::string(fruid.product.custom3)});
        }
        if (fruid.product.custom4 != nullptr) {
          fruIdInfoList.push_back({"Product Custom Data 4", std::string(fruid.product.custom4)});
        }
      }
      else {
        LOG(INFO) << "Product Info not set";
      }
    }
    else {
      LOG(ERROR) << "File " << eepromPath_ << " size " << size << " is less than " << FRUID_SIZE;
    }

    //close eepromFile
    eepromFile.close();
  }
  else {
    LOG(ERROR) << "File " << eepromPath_ << " does not exists";
  }

  return fruIdInfoList;
}

bool FruIdAccessI2CEEPROM::writeBinaryData(const std::string & binFilePath){
  std::ifstream binFile;
  binFile.open(binFilePath, std::ios::in | std::ios::binary | std::ios::ate);

  if (binFile.is_open()) {
    //Get size of binFile
    int size = binFile.tellg();

    if (size >= FRUID_SIZE) {
      unsigned char fruIdData[FRUID_SIZE] = {0};
      fruid_info_t fruid;

      //Get binary data from binFilePath
      binFile.seekg (0, std::ios::beg);
      binFile.read ((char*)fruIdData, FRUID_SIZE);

      // parse fruId from binFilePath dump and check if it is successful
      if (fruid_parse_eeprom(fruIdData, FRUID_SIZE, &fruid) == 0){
        //Parse successful -> binFilePath is verified
        //Write to eepromPath_
        std::string command = "dd if=" + binFilePath + " of=" + eepromPath_ + " bs=" + std::to_string(FRUID_SIZE) + " count=1";
        if (system(command.c_str()) == EXIT_SUCCESS) {
          return true;
        }
        else{
          LOG(ERROR) << "Command failed: " << command;
        }
      }
      else{
        LOG(ERROR) << "FRUID checksum failed for input FRUID bin " << binFilePath;
      }
    }
    else {
      LOG(ERROR) << "File " << binFilePath << " size " << size << " is less than " << FRUID_SIZE;
    }

    binFile.close();
  }
  else{
    LOG(ERROR) << "Unable to open file " << binFilePath;
  }

  return false;
}

bool FruIdAccessI2CEEPROM::dumpBinaryData(const std::string & destFilePath){
  std::ifstream eepromFile;
  std::ofstream outFile;
  eepromFile.open(eepromPath_, std::ios::in | std::ios::binary | std::ios::ate);

  if (eepromFile.is_open()) {
    //Get size of eepromFile
    int size = eepromFile.tellg();
    if (size >= FRUID_SIZE) {
      unsigned char fruIdData[FRUID_SIZE] = {0};

      //Get binary data from eepromFile
      eepromFile.seekg (0, std::ios::beg);
      eepromFile.read ((char*)fruIdData, FRUID_SIZE);

      //write data to destFilePath
      outFile.open(destFilePath, std::ios::binary);
      if(outFile.is_open()) {
        outFile.write((char*)fruIdData, FRUID_SIZE);
        outFile.close();
        return true;
      }
      else {
        LOG(ERROR) << "Unable to create file " << destFilePath;
      }
    }
    else {
      LOG(ERROR) << "File " << eepromPath_ << " size " << size << " is less than " << FRUID_SIZE;
    }

    //close eepromFile
    eepromFile.close();
  }
  else {
    LOG(ERROR) << "File " << eepromPath_ << " does not exists";
  }

  return false;
}

} // namespace qin
} // namespace openbmc
