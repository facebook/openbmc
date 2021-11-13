#pragma once
#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include "msg.hpp"
#include "uart.hpp"

using modbus_time = std::chrono::milliseconds;
class Modbus {
  std::string device_path;
  std::unique_ptr<UARTDevice> dev = nullptr;
  std::mutex mutex{};
  std::set<uint8_t> ignored_addrs = {};
  uint32_t default_baudrate = 0;
  modbus_time default_timeout = modbus_time::zero();
  modbus_time min_delay = modbus_time::zero();

 public:
  Modbus() {}
  virtual ~Modbus() {}

  uint32_t get_default_baudrate() const {
    return default_baudrate;
  }

  void initialize(const nlohmann::json& j);

  void command(
      Msg& req,
      Msg& resp,
      uint32_t baud = 0,
      modbus_time timeout = modbus_time::zero(),
      modbus_time settle_time = modbus_time::zero());
};

void from_json(const nlohmann::json& j, Modbus& m);
