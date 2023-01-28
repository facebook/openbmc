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

#include "ProtDevice.hpp"

#include <fmt/format.h>
#include <openbmc/AmiSmbusInterfaceSrcLib.h>
#include <openbmc/obmc-i2c.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace prot {

bool ProtDevice::s_verbose = false;

bool ProtDevice::getVerbose() {
  return s_verbose;
}
void ProtDevice::setVerbose(bool verbose) {
  s_verbose = verbose;
  EnableAmiSmbusInterfaceDebug(s_verbose);
}

ProtDevice::ProtDevice(uint8_t fru, uint8_t bus, uint8_t addr)
    : m_fruid(fru), m_bus(bus), m_addr(addr) {
  openDev();
}

ProtDevice::~ProtDevice() {
  closeDev();
}

ProtDevice::DevStatus ProtDevice::openDev() {
  if (isDevOpen()) {
    return DevStatus::SUCCESS;
  }

  auto fd = i2c_cdev_slave_open(m_bus, m_addr, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    auto e = errno;
    std::cout << "i2c device open failed (" << e << "), bus: " << m_bus
              << ", addr: " << m_addr << ", fd: " << fd << std::endl;
    return DevStatus::OPEN_FAILED;
  }
  m_fd = fd;
  return DevStatus::SUCCESS;
}

bool ProtDevice::isDevOpen() const {
  return !(m_fd < 0);
}

void ProtDevice::closeDev() {
  if (!isDevOpen())
    return;

  close(m_fd);
  m_fd = -1;
}

ProtDevice::DevStatus ProtDevice::portGetBootStatus(
    BOOT_STATUS_ACK_PAYLOAD& boot_status) {
  DATA_LAYER_PACKET_ACK data_layer_ack{};

  auto rc = protSendRecvDataPacket(CMD_BOOT_STATUS, nullptr, 0, data_layer_ack);
  if (rc != DevStatus::SUCCESS) {
    return rc;
  }

  memcpy(
      &boot_status,
      &data_layer_ack.PayLoad[0],
      sizeof(BOOT_STATUS_ACK_PAYLOAD));

  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::portRequestRecoverySpiUnlock() {
  DATA_LAYER_PACKET_ACK data_layer_ack{};

  auto rc = protSendRecvDataPacket(
      CMD_RECOVERY_SPI_UNLOCK_REQUEST, nullptr, 0, data_layer_ack);
  if (rc != DevStatus::SUCCESS) {
    return rc;
  }

  std::cout << fmt::format(
                   "Receive status code: {:#04x}", data_layer_ack.PayLoad[0])
            << std::endl;
  if (data_layer_ack.PayLoad[0]) {
    std::cout << "PCH have control of SPI, please try later..." << std::endl;
    return DevStatus::PROT_DEV_BUSY;
  }
  std::cout << "Granted" << std::endl;

  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::protGetUpdateStatus(uint8_t& status) {
  DATA_LAYER_PACKET_ACK data_layer_ack{};

  auto rc =
      protSendRecvDataPacket(CMD_UPDATE_STATUS, nullptr, 0, data_layer_ack);
  if (rc != DevStatus::SUCCESS) {
    return rc;
  }

  status = data_layer_ack.PayLoad[0];
  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::protFwUpdateIntent() {
  DATA_LAYER_PACKET_ACK data_layer_ack{};

  auto rc = protSendRecvDataPacket(
      CMD_FW_UPDATE_INTENT_REQUEST, nullptr, 0, data_layer_ack);

  if (rc != DevStatus::SUCCESS) {
    return rc;
  }

  // this command require extra 5 seconds delay to prevent error return...
  // 0x04 0x85 0x02 0x75
  sleep(5);
  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::protUpdateComplete() {
  DATA_LAYER_PACKET_ACK data_layer_ack{};

  return protSendRecvDataPacket(
      CMD_UPDATE_COMPLETE, nullptr, 0, data_layer_ack);
}

ProtDevice::DevStatus ProtDevice::protUfmLogReadoutEntry(size_t& log_count) {
  log_count = 0;
  DATA_LAYER_PACKET_ACK data_layer_ack{};

  auto rc = protSendRecvDataPacket(
      CMD_UFM_LOG_READOUT_ENTRY, nullptr, 0, data_layer_ack);
  if (rc != DevStatus::SUCCESS) {
    return rc;
  }

  log_count = static_cast<size_t>(data_layer_ack.PayLoad[0]);
  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::protGetLogData(size_t index, PROT_LOG& log) {
  DATA_LAYER_PACKET_ACK data_layer_ack{};

  uint8_t i = index % 0xFF;
  auto rc = protSendRecvDataPacket(CMD_LOG_DATA_LAYER, &i, 1, data_layer_ack);
  if (rc != DevStatus::SUCCESS) {
    return rc;
  }

  if (data_layer_ack.Length != 14) {
    std::cout << fmt::format(
        "the log data length is invalid, length: {:d}\n",
        data_layer_ack.Length);
    return DevStatus::PROT_DATA_RECV_FAILED;
  }

  memcpy(&log, &data_layer_ack.PayLoad[0], sizeof(PROT_LOG));
  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::protSendDataPacket(
    std::vector<uint8_t> data_raw) {
  return protSendDataPacket(
      data_raw[0], (data_raw.data() + 1), (data_raw.size() - 1));
}

ProtDevice::DevStatus
ProtDevice::protSendDataPacket(uint8_t cmd, uint8_t* payload, size_t length) {
  if (!isDevOpen()) {
    std::cout << "device not opened, fd: " << m_fd << std::endl;
    return DevStatus::OPEN_FAILED;
  }

  auto ret = sent_data_packet(m_fruid, m_fd, cmd, payload, length);
  if (ret) {
    std::cout << fmt::format(
                     "sent_data_packet fail, cmd: {:#04x}, ret: {}", cmd, ret)
              << std::endl;
    return DevStatus::PROT_CMD_FAILED;
  }

  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::protRecvDataPacket(
    DATA_LAYER_PACKET_ACK& data_layer_ack) {
  auto ret = DataLayerReceiveFromPlatFire(
      m_fruid, m_fd, (unsigned char*)&data_layer_ack);
  if (ret) {
    std::cout << "DataLayerReceiveFromPlatFire fail, ret: " << ret << std::endl;
    return DevStatus::PROT_DATA_RECV_FAILED;
  }

  return DevStatus::SUCCESS;
}

ProtDevice::DevStatus ProtDevice::protSendRecvDataPacket(
    uint8_t cmd,
    uint8_t* payload,
    size_t length,
    DATA_LAYER_PACKET_ACK& data_layer_ack) {
  {
    auto rc = protSendDataPacket(cmd, payload, length);
    if (rc != DevStatus::SUCCESS) {
      return rc;
    }
  }

  sleep(1); // most commands need 1 second delay between send and receive

  {
    auto rc = protRecvDataPacket(data_layer_ack);
    if (rc != DevStatus::SUCCESS) {
      return rc;
    }
  }

  return DevStatus::SUCCESS;
}

namespace ProtSpiInfo {
std::string spiStatusString(uint8_t status_val) {
  const static std::vector<std::string> status_str_vec = {
      "active",
      "recovery",
      "dirty",
      "update staged",
      "decommission",
  };

  if (status_val >= status_str_vec.size()) {
    return "undefined";
  }

  return status_str_vec[status_val];
}

std::string spiVerifyString(uint8_t verify_val) {
  const static std::vector<std::string> verify_str_vec = {
      "successful",
      "fail",
  };

  if (verify_val >= verify_str_vec.size()) {
    return "undefined";
  }

  return verify_str_vec[verify_val];
}

} // namespace ProtSpiInfo

} // namespace prot
