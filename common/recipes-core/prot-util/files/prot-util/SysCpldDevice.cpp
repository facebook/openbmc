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
 
#include "SysCpldDevice.hpp"

namespace sysCpld {

CpldDevice::CpldDevice(uint8_t fru, uint8_t bus, uint8_t addr)
    : m_fruid(fru), m_bus(bus), m_addr(addr) {
  openDev();
}

CpldDevice::~CpldDevice() {
  closeDev();
}

bool CpldDevice:: CpldDevice::openDev() {
  if (isDevOpen()) {
    return true;
  }

  auto fd = i2c_cdev_slave_open(m_bus, m_addr, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    auto e = errno;
    std::cerr << "i2c device open failed (" << e << "), bus: " << m_bus
              << ", addr: " << m_addr << ", fd: " << fd << std::endl;
    return false;
  }
  m_fd = fd;
  return true;
}

bool CpldDevice::isDevOpen() const {
  return !(m_fd < 0);
}

void CpldDevice::closeDev() {
  if (!isDevOpen())
    return;

  close(m_fd);
  m_fd = -1;
}

bool CpldDevice::isAuthComplete() {
  uint8_t value;

  value = readRegister(prot_seq_signals);

  return BIT(value, auth_complete_bit);
}

bool CpldDevice::isBootFail() {
  uint8_t value;

  value = readRegister(prot_ctrl_signals);

  return BIT(value, boot_fail_bit);
}

uint8_t CpldDevice::readRegister(uint8_t offset) {
  uint8_t tbuf[8] = {offset};
  uint8_t rbuf[8] = {0};

  if (!isDevOpen()) {
    throw std::runtime_error("Cpld Device not open");
  }

  if (retry_cond(!i2c_rdwr_msg_transfer(m_fd, (m_addr << 1), tbuf, 1, rbuf, 1),
        max_retry, retry_delay)) {
    throw std::runtime_error("Fail to read offset: " + offset);
  }
  
  return rbuf[0];
}

} // end of namespace sysCpld