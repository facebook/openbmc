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
#include <span>
#include <string>
#include <vector>

namespace prot {

constexpr auto LEVEL0_PUBKEY_LENGTH = 64;

struct BOOT_STATUS_ACK_PAYLOAD {
  uint8_t SPI_A; // 0 Active 1 Recovery 2 Dirty 3 Update Stage 4 Decommission
  uint8_t SPI_B;
  uint8_t ActiveVerificationStatus; // 0 Success 1 Failure.
  uint8_t RecoveryVerificationStatus;
  uint8_t SPIABiosVersionNumber[4];
  uint8_t SPIBBiosVersionNumber[4];
  uint8_t Checksum;
};

struct PROT_VERSION {
  uint8_t signature[8];
  uint8_t XFRVersion[8];
  uint8_t BuildDate[8];
  uint8_t Time[8];
  uint8_t SFBVersion[8];
  uint8_t CFGVersion[8];
  uint8_t WorkSpaceVersion[8];
  uint8_t OEMVersion[8];
  uint8_t ODMVersion[8];
  uint8_t reserved[8];
};

struct XFR_VERSION_READ_ACK_PAYLOAD {
  PROT_VERSION Active;
  PROT_VERSION Recovery;
  unsigned short int SiliconRBP;
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
  DevStatus protReadXfrVersion(XFR_VERSION_READ_ACK_PAYLOAD& prot_ver);
  DevStatus protLevel0PubKeyRead(std::vector<uint8_t>& pubkey);

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

class updateProperty {
 public:
  uint8_t target = 0;
  bool isPfrUpdate = true;
  updateProperty(
      const BOOT_STATUS_ACK_PAYLOAD& boot_status,
      uint8_t spiA,
      uint8_t spiB);

 private:
  uint8_t mSpiA = 0;
  uint8_t mSpiB = 1;
};

std::string spiStatusString(uint8_t status_val);
std::string spiVerifyString(uint8_t verify_val);
}; // namespace ProtSpiInfo

namespace ProtUpdateStatus {
std::string updateStatusString(uint8_t status_val);
}

namespace ProtVersion {
std::string getVerString(const std::span<uint8_t> ver);
std::string getDateString(const std::span<uint8_t, 8> date);
std::string getTimeString(const std::span<uint8_t, 8> time);
}; // namespace ProtVersion

} // end of namespace prot
