#include "modbus.hpp"
#include <fstream>
#include <thread>

using nlohmann::json;

void Modbus::command(
    Msg& req,
    Msg& resp,
    uint32_t baud,
    modbus_time timeout,
    modbus_time settle_time) {
  if (timeout == modbus_time::zero())
    timeout = default_timeout;
  if (baud == 0)
    baud = default_baudrate;
  req.encode();
  std::lock_guard<std::mutex> lck(mutex);
  dev->set_baudrate(baud);
  dev->write(req.raw.data(), req.len);
  dev->read(resp.raw.data(), resp.len, timeout.count());
  resp.decode();
  if (settle_time != modbus_time::zero()) {
    std::this_thread::sleep_for(settle_time);
  }
}

void Modbus::initialize(const json& j) {
  j.at("device_path").get_to(device_path);
  j.at("baudrate").get_to(default_baudrate);
  std::string device_type = j.value("device_type", "default");

  default_timeout = modbus_time(j.value("default_timeout", 300));
  min_delay = modbus_time(j.value("min_delay", 0));
  ignored_addrs = j.value("ignored_addrs", std::set<uint8_t>{});
  if (device_type == "default") {
    dev = std::make_unique<UARTDevice>(device_path, default_baudrate);
  } else if (device_type == "aspeed_rs485") {
    dev = std::make_unique<AspeedRS485Device>(device_path, default_baudrate);
  } else {
    throw std::runtime_error("Unknown device type: " + device_type);
  }
  dev->open();
}

void from_json(const json& j, Modbus& m) {
  m.initialize(j);
}
