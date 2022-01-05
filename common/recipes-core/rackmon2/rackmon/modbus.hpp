#pragma once
#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include "msg.hpp"
#include "uart.hpp"

using modbus_time = std::chrono::milliseconds;
class Modbus {
  std::string device_path{};
  std::unique_ptr<UARTDevice> dev = nullptr;
  std::mutex mutex{};
  std::set<uint8_t> ignored_addrs = {};
  uint32_t default_baudrate = 0;
  modbus_time default_timeout = modbus_time::zero();
  modbus_time min_delay = modbus_time::zero();
  std::ostream& profile_store;

 public:
  explicit Modbus(std::ostream& prof) : profile_store(prof) {}
  virtual ~Modbus() {}

  uint32_t get_default_baudrate() const {
    return default_baudrate;
  }
  const std::string& name() const {
    return device_path;
  }

  virtual std::unique_ptr<UARTDevice> make_device(
      const std::string& device_type,
      const std::string& device_path,
      uint32_t baud);

  virtual void initialize(const nlohmann::json& j);

  virtual void command(
      Msg& req,
      Msg& resp,
      uint32_t baud = 0,
      modbus_time timeout = modbus_time::zero(),
      modbus_time settle_time = modbus_time::zero());
};

void from_json(const nlohmann::json& j, Modbus& m);
