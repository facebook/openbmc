#include "modbus.hpp"
#include <fstream>
#include <thread>
#include "log.hpp"

using nlohmann::json;

void Modbus::command(
    Msg& req,
    Msg& resp,
    uint32_t baud,
    modbus_time timeout,
    modbus_time settle_time) {
  RACKMON_PROFILE_SCOPE(
      modbus_command,
      "modbus::" + std::to_string(int(req.addr)),
      profile_store);
  if (timeout == modbus_time::zero())
    timeout = default_timeout;
  if (baud == 0)
    baud = default_baudrate;
  req.encode();
  std::lock_guard<std::mutex> lck(mutex);
  dev->setBaudrate(baud);
  dev->write(req.raw.data(), req.len);
  dev->read(resp.raw.data(), resp.len, timeout.count());
  resp.decode();
  if (settle_time != modbus_time::zero()) {
    std::this_thread::sleep_for(settle_time);
  }
}

std::unique_ptr<UARTDevice> Modbus::make_device(
    const std::string& device_type,
    const std::string& device_path,
    uint32_t baud) {
  std::unique_ptr<UARTDevice> ret;
  if (device_type == "default") {
    ret = std::make_unique<UARTDevice>(device_path, baud);
  } else if (device_type == "aspeed_rs485") {
    ret = std::make_unique<AspeedRS485Device>(device_path, baud);
  } else {
    throw std::runtime_error("Unknown device type: " + device_type);
  }
  return ret;
}

void Modbus::initialize(const json& j) {
  j.at("device_path").get_to(device_path);
  j.at("baudrate").get_to(default_baudrate);
  std::string device_type = j.value("device_type", "default");

  default_timeout = modbus_time(j.value("default_timeout", 300));
  min_delay = modbus_time(j.value("min_delay", 0));
  ignored_addrs = j.value("ignored_addrs", std::set<uint8_t>{});
  dev = make_device(device_type, device_path, default_baudrate);
  dev->open();
}

void from_json(const json& j, Modbus& m) {
  m.initialize(j);
}
