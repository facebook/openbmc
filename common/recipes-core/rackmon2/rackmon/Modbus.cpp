// Copyright 2021-present Facebook. All Rights Reserved.
#include "Modbus.hpp"
#include <fstream>
#include <thread>
#include "Log.hpp"

using nlohmann::json;
using namespace rackmon;

void Modbus::command(
    Msg& req,
    Msg& resp,
    uint32_t baudrate,
    ModbusTime timeout,
    ModbusTime settleTime) {
  RACKMON_PROFILE_SCOPE(
      modbusCommand, "modbus::" + std::to_string(int(req.addr)), profileStore_);
  if (timeout == ModbusTime::zero())
    timeout = defaultTimeout_;
  if (baudrate == 0)
    baudrate = defaultBaudrate_;
  req.encode();
  std::lock_guard<std::mutex> lck(deviceMutex_);
  device_->setBaudrate(baudrate);
  device_->write(req.raw.data(), req.len);
  device_->read(resp.raw.data(), resp.len, timeout.count());
  resp.decode();
  if (settleTime != ModbusTime::zero()) {
    std::this_thread::sleep_for(settleTime);
  }
}

std::unique_ptr<UARTDevice> Modbus::makeDevice(
    const std::string& deviceType,
    const std::string& devicePath,
    uint32_t baudrate) {
  std::unique_ptr<UARTDevice> ret;
  if (deviceType == "default") {
    ret = std::make_unique<UARTDevice>(devicePath, baudrate);
  } else if (deviceType == "aspeed_rs485") {
    ret = std::make_unique<AspeedRS485Device>(devicePath, baudrate);
  } else {
    throw std::runtime_error("Unknown device type: " + deviceType);
  }
  return ret;
}

void Modbus::initialize(const json& j) {
  j.at("device_path").get_to(devicePath_);
  j.at("baudrate").get_to(defaultBaudrate_);
  std::string deviceType = j.value("device_type", "default");

  defaultTimeout_ = ModbusTime(j.value("default_timeout", 300));
  minDelay_ = ModbusTime(j.value("min_delay", 0));
  ignoredAddrs_ = j.value("ignored_addrs", std::set<uint8_t>{});
  device_ = makeDevice(deviceType, devicePath_, defaultBaudrate_);
  device_->open();
}

void from_json(const json& j, Modbus& m) {
  m.initialize(j);
}
