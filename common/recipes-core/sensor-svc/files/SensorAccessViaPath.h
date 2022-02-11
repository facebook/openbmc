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
#include <fstream>
#include <sstream>
#include <glog/logging.h>
#include "SensorAccessMechanism.h"

namespace openbmc {
namespace qin {

#define LARGEST_DEVICE_NAME 120
#define GPIO_BAT_SENSE_EN_N 46
#define NIC_MAX_TEMP 125

class SensorAccessViaPath : public SensorAccessMechanism {
  private:
    std::string path_;          //sensor path
    float unitDiv_ = 1;         //divisor for value read from path

  public:
    SensorAccessViaPath(std::string path)
      : path_(path) {}

    SensorAccessViaPath(std::string path, float unitDiv)
      : path_(path), unitDiv_(unitDiv) {}

    void rawRead(Sensor* s, float *value) override {
      int pos = path_.find('*');
      while (pos != std::string::npos){
        //need to resolve path_
        char temp[LARGEST_DEVICE_NAME];
        std::ostringstream out;
        out << "cd " << path_.substr(0, pos + 1) << "; pwd";

        // Get current working directory
        FILE* fp = popen(out.str().c_str(), "r");
        if (fp != NULL){
          fgets(temp, LARGEST_DEVICE_NAME, fp);
          pclose(fp);

          //Remove newline char at end
          int size = strlen(temp);
          temp[size-1] = '\0';

          path_ = std::string(temp) + path_.substr(pos + 1);
        }

        pos = path_.find('*');
      }

      std::ifstream sensorFile (path_.c_str());
      if (sensorFile.is_open())
      {
        sensorFile >> *value;
        sensorFile.close();
        *value = (*value) / unitDiv_;
        readResult_ = READING_SUCCESS;
      }
      else {
        LOG(INFO) << "Could not open sensor file at " << path_;
        readResult_ = READING_NA;
      }
    }

    bool preRawRead(Sensor* s, float* value) override;
    void postRawRead(Sensor* s, float* value) override;
};

} // namespace qin
} // namespace openbmc
