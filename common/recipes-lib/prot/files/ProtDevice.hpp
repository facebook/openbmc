/*
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
 *
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

#include <openbmc/AmiSmbusInterfaceSrcLib.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace prot {

struct BOOT_STATUS_ACK_PAYLOAD {
  uint8_t SPI_A; // 0 Active 1 Recovery 2 Dirty 3 Update Stage 4 Decommission
  uint8_t SPI_B;
  uint8_t ActiveVerificationStatus; // 0 Success 1 Failure.
  uint8_t RecoveryVerificationStatus;
  uint32_t SPIABiosVersionNumber;
  uint32_t SPIBBiosVersionNumber;
  uint8_t Checksum;
};

class ProtDevice {
 public:
  enum class DevStatus {
    SUCCESS = 0,
    OPEN_FAILED = -1,
    PROT_CMD_FAILED = -2,
    PROT_DATA_RECV_FAILED = -3,
    PROT_DEV_BUSY = -4,
  };

  ProtDevice(uint8_t fru, uint8_t bus, uint8_t addr);
  ~ProtDevice();

  bool isDevOpen() const;

  uint8_t getFruId() const {
    return m_fruid;
  }
  uint8_t getBus() const {
    return m_bus;
  }
  uint8_t getAddr() const {
    return m_addr;
  }

  DevStatus portGetBootStatus(BOOT_STATUS_ACK_PAYLOAD& boot_status);
  DevStatus portRequestRecoverySpiUnlock();
  DevStatus protGetUpdateStatus(uint8_t& status);
  DevStatus protFwUpdateIntent();
  DevStatus protUpdateComplete();
  DevStatus protUfmLogReadoutEntry(size_t& log_count);
  DevStatus protGetLogData(size_t index, PROT_LOG& log);

  DevStatus protSendDataPacket(std::vector<uint8_t> data_raw);
  DevStatus protSendDataPacket(uint8_t cmd, uint8_t* payload, size_t length);
  DevStatus protRecvDataPacket(DATA_LAYER_PACKET_ACK& data_layer_ack);

  DevStatus protSendRecvDataPacket(
      uint8_t cmd,
      uint8_t* payload,
      size_t length,
      DATA_LAYER_PACKET_ACK& data_layer_ack);

  static bool getVerbose();
  static void setVerbose(bool verbose);

 private:
  DevStatus openDev();
  void closeDev();

  int m_fd = -1;
  uint8_t m_fruid = 0;
  uint8_t m_bus = 0;
  uint8_t m_addr = 0;

  static bool s_verbose;
};

namespace ProtSpiInfo {
enum class SpiStatus {
  ACTIVE = 0,
  RECOVERY,
  DIRTY,
  UPDATE_STAGED,
  DECOMMSION,
};

enum class SpiVerify {
  SUCCESS = 0,
  FAIL,
};

std::string spiStatusString(uint8_t status_val);
std::string spiVerifyString(uint8_t verify_val);
}; // namespace ProtSpiInfo

} // end of namespace prot
