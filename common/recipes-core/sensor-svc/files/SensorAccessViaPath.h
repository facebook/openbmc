/*
 * SensorAccessViaPath.h
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
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstring>
#include <syslog.h>
#include "SensorAccessMechanism.h"

#define LARGEST_DEVICE_NAME 120
#define GPIO_BAT_SENSE_EN_N 46
#define NIC_MAX_TEMP 125

class SensorAccessViaPath : public SensorAccessMechanism {
  private:
    std::string path;          //sensor path
    float unitDiv = 1;         //divisor for value read from path

  public:
    SensorAccessViaPath(std::string path){
      this->path = path;
    }

    SensorAccessViaPath(std::string path, float unitDiv) {
      this->path = path;
      this->unitDiv = unitDiv;
    }

    void setUnitDiv(float unitDiv) {
      this->unitDiv = unitDiv;
    }

    void rawRead(Sensor* s, float *value) override {
      int pos = path.find('*');
      while (pos != std::string::npos){
        //need to resolve path
        char temp[LARGEST_DEVICE_NAME];
        std::ostringstream out;
        out << "cd " << path.substr(0, pos + 1) << "; pwd";

        // Get current working directory
        FILE* fp = popen(out.str().c_str(), "r");
        if (fp != NULL){
          fgets(temp, LARGEST_DEVICE_NAME, fp);
          pclose(fp);

          //Remove newline char at end
          int size = strlen(temp);
          temp[size-1] = '\0';

          path = std::string(temp) + path.substr(pos + 1);
        }

        pos = path.find('*');
      }

      std::ifstream sensorFile (path.c_str());
      if (sensorFile.is_open())
      {
        sensorFile >> *value;
        sensorFile.close();
        *value = (*value) / unitDiv;
        readResult_ = READING_SUCCESS;
      }
      else {
        syslog(LOG_INFO, "Could not open sensor file at %s ", path.c_str());
        readResult_ = READING_NA;
      }
    }

    bool preRawRead(Sensor* s, float* value) override;
    void postRawRead(Sensor* s, float* value) override;
};
