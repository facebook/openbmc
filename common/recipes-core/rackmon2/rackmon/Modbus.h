// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include "Msg.h"
#include "UARTDevice.h"

namespace rackmon {

using ModbusTime = std::chrono::milliseconds;
class Modbus {
  std::string devicePath_{};
  std::unique_ptr<UARTDevice> device_ = nullptr;
  std::mutex deviceMutex_{};
  std::set<uint8_t> ignoredAddrs_ = {};
  uint32_t defaultBaudrate_ = 0;
  ModbusTime defaultTimeout_ = ModbusTime::zero();
  ModbusTime minDelay_ = ModbusTime::zero();
  std::chrono::seconds interfaceRetryTime_ = std::chrono::seconds(600);
  bool deviceValid_ = false;
  size_t openTries_ = 0;
  const size_t maxOpenTries_ = 12;
  std::ostream& profileStore_;

  void probePresence();
 public:
  explicit Modbus(std::ostream& prof) : profileStore_(prof) {}
  virtual ~Modbus() {
    if (device_) {
      device_->close();
    }
  }

  uint32_t getDefaultBaudrate() const {
    return defaultBaudrate_;
  }
  const std::string& name() const {
    return devicePath_;
  }

  virtual std::unique_ptr<UARTDevice> makeDevice(
      const std::string& deviceType,
      const std::string& devicePath,
      uint32_t baudrate);

  virtual void initialize(const nlohmann::json& j);

  virtual void command(
      Msg& req,
      Msg& resp,
      uint32_t baudrate = 0,
      ModbusTime timeout = ModbusTime::zero(),
      ModbusTime settleTime = ModbusTime::zero());

  virtual bool isPresent() {
    return deviceValid_;
  }
};

void from_json(const nlohmann::json& j, Modbus& m);
} // namespace rackmon
