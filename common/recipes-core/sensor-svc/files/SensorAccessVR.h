/*
 * SensorAccessVR.h
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
#include <string>
#include "SensorAccessMechanism.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <glog/logging.h>
#include <openbmc/obmc-i2c.h>

namespace openbmc {
namespace qin {

#define MAX_READ_RETRY 10
#define VR_UPDATE_IN_PROGRESS "/tmp/stop_monitor_vr"
#define VR_TIMEOUT 500 * 4 // 4 including temp, current, power, volt
#define VR_TELEMETRY_VOLT 0x1A
#define VR_TELEMETRY_CURR 0x15
#define VR_TELEMETRY_POWER 0x2D
#define VR_TELEMETRY_TEMP 0x29



class SensorAccessVR : public SensorAccessMechanism {
  private:
    uint8_t busId_;
    uint8_t loop_;
    uint8_t reg_;
    uint8_t slaveAddr_;

  public:
    SensorAccessVR(uint8_t busId,
                   uint8_t loop,
                   uint8_t reg,
                   uint8_t slaveAddr) {
      this->busId_ = busId;
      this->loop_ = loop;
      this->reg_ = reg;
      this->slaveAddr_ = slaveAddr;
    }

    bool preRawRead(Sensor* s, float* value) override;

    void rawRead(Sensor* s, float *value) override{
      int fd;
      char fn[32];
      unsigned int retry = MAX_READ_RETRY;
      int ret;
      uint8_t tcount, rcount;
      uint8_t tbuf[16] = {0};
      uint8_t rbuf[16] = {0};

      readResult_ = READING_NA;

      static uint16_t vrUpdateInProgressCount = 0;
      if ( access(VR_UPDATE_IN_PROGRESS, F_OK) == 0 )
      {
        //Avoid sensord unmonitoring vr sensors
        //due to unexpected condition happen during vr_update
        if ( vrUpdateInProgressCount > VR_TIMEOUT)
        {
          remove(VR_UPDATE_IN_PROGRESS);
          vrUpdateInProgressCount = 0;
        }

        syslog(LOG_WARNING,
               "[%d]Stop Monitor VR Volt due to VR update is in progress\n",
               vrUpdateInProgressCount++);
        LOG(INFO) << "VR update in progress ";
        return;
      }
      else {
        vrUpdateInProgressCount = 0;
      }

      snprintf(fn, sizeof(fn), "/dev/i2c-%d", busId_);
      while (retry) {
        fd = open(fn, O_RDWR);
        if (fd < 0) {
          retry--;
          LOG(WARNING) << "i2c_io failed for bus -1 ";
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
          break;
        }

        if(retry == 0)
          goto error_exit;
      }

      // Set the page for read
      tbuf[0] = 0x00;
      tbuf[1] = loop_;

      tcount = 2;
      rcount = 0;

      retry = MAX_READ_RETRY;
      while (retry) {
        ret = i2c_rdwr_msg_transfer(fd, slaveAddr_, tbuf, tcount, rbuf, rcount);
        if (ret) {
          retry--;
          LOG(WARNING) << "i2c_io failed for bus ";
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
          break;
        }

        if(retry == 0)
          goto error_exit;
      }

      // Read 2 bytes from register
      tbuf[0] = reg_;

      tcount = 1;
      rcount = 2;

      retry = MAX_READ_RETRY;
      while(retry) {
        ret = i2c_rdwr_msg_transfer(fd, slaveAddr_, tbuf, tcount, rbuf, rcount);
        if (ret) {
          LOG(WARNING) << "i2c_io failed for bus 2";
          retry--;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
          break;
        }

        if (retry == 0)
          goto error_exit;
      }

      readResult_ = READING_SUCCESS;

      switch (reg_) {
        case VR_TELEMETRY_VOLT: {
          *value = ((rbuf[1] & 0x0F) * 256 + rbuf[0] ) * 1.25;
          *value /= 1000;
          break;
        }

        case VR_TELEMETRY_CURR: {
          if (rbuf[1] < 0x40) {
            // Positive value (sign at bit6)
            *value = ((rbuf[1] & 0x7F) * 256 + rbuf[0] ) * 62.5;
            *value /= 1000;
          } else {
            // Negative value 2's complement
            uint16_t temp = ((rbuf[1] & 0x7F) << 8) | rbuf[0];
            temp = 0x7fff - temp + 1;

            *value = (((temp >> 8) & 0x7F) * 256 + (temp & 0xFF) ) * -62.5;
            *value /= 1000;
          }
          break;
        }

        case VR_TELEMETRY_TEMP: {
          // AN-E1610B-034B: temp[11:0]
          // Calculate Temp
          int16_t temp = (rbuf[1] << 8) | rbuf[0];
          if ((rbuf[1] & 0x08))
            temp |= 0xF000; // If negative, sign extend temp.
          *value = (float)temp * 0.125;
          break;
        }

        case VR_TELEMETRY_POWER: {
          // Calculate Power
          *value = ((rbuf[1] & 0x3F) * 256 + rbuf[0] ) * 0.04;
          break;
        }
      }

    error_exit:
      if (fd > 0) {
        close(fd);
      }
    }
};

} // namespace qin
} // namespace openbmc
