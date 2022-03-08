// Copyright 2021-present Facebook. All Rights Reserved.
#include "Modbus.h"
#include <fstream>
#include <thread>
#include "Log.h"

using nlohmann::json;

namespace rackmon {

void Modbus::command(
    Msg& req,
    Msg& resp,
    uint32_t baudrate,
    ModbusTime timeout,
    ModbusTime settleTime) {
  if (!deviceValid_) {
    throw std::runtime_error("Uninitialized");
  }
  RACKMON_PROFILE_SCOPE(
      modbusCommand, "modbus::" + std::to_string(int(req.addr)), profileStore_);
  if (timeout == ModbusTime::zero()) {
    timeout = defaultTimeout_;
  }
  if (baudrate == 0) {
    baudrate = defaultBaudrate_;
  }
  req.encode();
  std::lock_guard<std::mutex> lck(deviceMutex_);
  device_->setBaudrate(baudrate);
  device_->write(req.raw.data(), req.len);
  resp.len = device_->read(resp.raw.data(), resp.len, timeout.count());
  resp.decode();
  if (settleTime != ModbusTime::zero()) {
    // If the bus needs to be idle after each transaction for
    // a given period of time, sleep here.
    // sleep override
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
  } else if (deviceType == "AspeedRS485") {
    ret = std::make_unique<AspeedRS485Device>(devicePath, baudrate);
  } else if (deviceType == "LocalEcho") {
    ret = std::make_unique<LocalEchoUARTDevice>(devicePath, baudrate);
  } else {
    throw std::runtime_error("Unknown device type: " + deviceType);
  }
  return ret;
}

void Modbus::probePresence() {
  if (!deviceValid_) {
    if (openTries_ > 0) {
      std::this_thread::sleep_for(std::chrono::seconds(interfaceRetryTime_));
    }
    try {
      openTries_++;
      device_->open();
      deviceValid_ = true;
    } catch (std::exception& ex) {
      // Log only once.
      if (openTries_ == 1) {
        logError << "Interface open failed: " << ex.what() << std::endl;
      }
      if (openTries_ < maxOpenTries_) {
        auto tid = std::thread(&Modbus::probePresence, this);
        tid.detach();
      }
    }
  }
}

void Modbus::initialize(const json& j) {
  j.at("device_path").get_to(devicePath_);
  j.at("baudrate").get_to(defaultBaudrate_);
  std::string deviceType = j.value("device_type", "default");

  defaultTimeout_ = ModbusTime(j.value("default_timeout", 300));
  minDelay_ = ModbusTime(j.value("min_delay", 0));
  ignoredAddrs_ = j.value("ignored_addrs", std::set<uint8_t>{});
  device_ = makeDevice(deviceType, devicePath_, defaultBaudrate_);
  probePresence();
}

void from_json(const json& j, Modbus& m) {
  m.initialize(j);
}

} // namespace rackmon
