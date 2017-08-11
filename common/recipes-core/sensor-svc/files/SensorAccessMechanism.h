/*
 * SensorAccessMechanism.h
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
#include <cstdint>

namespace openbmc {
namespace qin {

//Acccess conditions
#define ACCESS_COND_CHECK_FRUPOWERON (1 << 0)
#define ACCESS_COND_CHECK_FRUPOWERON_5SEC (1 << 1)


enum ReadResult {
  READING_NA = -2,
  READING_SKIP = 1,
  READING_SUCCESS = 0
};

class Sensor;     //Forward declaration of class Sensor

class SensorAccessMechanism {

protected:

  int8_t maxNofRetry_ = -1; // -1 if no limit
  uint8_t totalRetry_ = 0;
  ReadResult readResult_ = READING_NA;
  uint8_t accessCondition_ = 0; // Allways accessible

  virtual void rawRead(Sensor* s, float *value);
  virtual bool preRawRead(Sensor* s, float* value) {
    return true;
  }
  virtual void postRawRead(Sensor* s, float* value) {}
  bool checkAccessConditions(Sensor* s);

public:
  bool setAccessConditions(uint8_t accessCondition) {
    this->accessCondition_ = accessCondition;
  }

  ReadResult sensorRawRead(Sensor* s, float *value) {

    if (checkAccessConditions(s) == false) {
      readResult_ = READING_NA;
      return readResult_;
    }

    if (preRawRead(s, value)){
      rawRead(s, value);
    }
    else {
      readResult_ = READING_NA;
    }

    postRawRead(s, value);

    if (readResult_ == READING_NA) {
      *value = 0;
    }

    //Max retry logic
    if (maxNofRetry_ > 0) {
      if (readResult_ == READING_SUCCESS) {
        maxNofRetry_ = 0;
      }
      else if (maxNofRetry_ > maxNofRetry_){
        maxNofRetry_++;
      }
      else {
        readResult_ =  READING_SKIP;
      }
    }

    return readResult_;
  }

  ReadResult getLastReadResult() {
    return readResult_;
  }

  void setmaxNofRetry (uint8_t maxNofRetry) {
    this->maxNofRetry_ = maxNofRetry;
  }
};

} // namespace qin
} // namespace openbmc
