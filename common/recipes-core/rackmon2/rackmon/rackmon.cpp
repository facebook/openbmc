#include "rackmon.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <iomanip>

using nlohmann::json;
using namespace std::literals;

void Rackmon::load(
    const std::string& conf_path,
    const std::string& regmap_dir) {
  // TODO: Catch parse exceptions and print a pretty
  // message on exactly which configuration fail
  // was bad/missing.
  std::ifstream ifs(conf_path);
  json j;
  ifs >> j;
  for (const auto& iface_conf : j["interfaces"]) {
    std::unique_ptr<Modbus> iface = std::make_unique<Modbus>();
    iface->initialize(iface_conf);
    interfaces.push_back(std::move(iface));
  }
  regmap_db.load(regmap_dir);

  // Precomputing this makes our scan soooo much easier.
  // its 256 bytes wasted. but worth it.
  for (uint16_t addr = 0; addr <= 0xff; addr++) {
    try {
      regmap_db.at(uint8_t(addr));
      possible_dev_addrs.push_back(uint8_t(addr));
    } catch (std::out_of_range& e) {
      continue;
    }
  }
}

bool Rackmon::probe(Modbus& iface, uint8_t addr) {
  RegisterMap& rmap = regmap_db.at(addr);
  std::vector<uint16_t> v(1);
  try {
    ReadHoldingRegistersReq req(addr, rmap.probe_register, v.size());
    ReadHoldingRegistersResp resp(v);
    iface.command(req, resp, rmap.default_baudrate, probe_timeout);
    std::unique_lock lock(devices_mutex);
    active_devices[addr] = std::make_unique<ModbusDevice>(iface, addr, rmap);
    std::cout << std::hex << std::setw(2) << std::setfill('0');
    std::cout << "Found " << int(addr) << " on " << iface.name() << std::endl;
    return true;
  } catch (std::exception& e) {
    return false;
  }
}

void Rackmon::probe(uint8_t addr) {
  // We do not support the same address
  // on multiple interfaces.
  for (auto& iface : interfaces)
    if (probe(*iface, addr))
      break;
}

void Rackmon::mark_active(uint8_t addr) {
  std::cout << std::hex << std::setw(2) << std::setfill('0');
  std::cout << "Device marked as active: " << int(addr) << std::endl;
  std::unique_lock lock(devices_mutex);
  std::unique_ptr<ModbusDevice> dev = std::move(dormant_devices.at(addr));
  dormant_devices.erase(addr);
  active_devices[addr] = std::move(dev);
}

void Rackmon::mark_dormant(uint8_t addr) {
  std::cout << std::hex << std::setw(2) << std::setfill('0');
  std::cout << "Device marked as dormant: " << int(addr) << std::endl;
  std::unique_lock lock(devices_mutex);
  std::unique_ptr<ModbusDevice> dev = std::move(active_devices.at(addr));
  active_devices.erase(addr);
  dormant_devices[addr] = std::move(dev);
}

std::vector<uint8_t> Rackmon::inspect_dormant() {
  time_t curr = std::time(0);
  std::vector<uint8_t> ret{};
  std::shared_lock lock(devices_mutex);
  for (auto it = dormant_devices.begin(); it != dormant_devices.end();) {
    // If its more than 300s since last activity, start probing it.
    // change to something larger if required.
    if ((it->second->last_active() + dormant_min_inactive_time) < curr) {
      RegisterMap& rmap = regmap_db.at(it->first);
      uint16_t probe = rmap.probe_register;
      std::vector<uint16_t> v(1);
      try {
        uint8_t addr = it->first;
        it->second->ReadHoldingRegisters(probe, v);
        ret.push_back(addr);
      } catch (...) {
        it++;
      }
    }
  }
  return ret;
}
void Rackmon::recover_dormant() {
  std::vector<uint8_t> candidates = inspect_dormant();
  for (auto& addr : candidates) {
    mark_active(addr);
  }
}

std::vector<uint8_t> Rackmon::monitor_active() {
  std::vector<uint8_t> ret{};
  std::shared_lock lock(devices_mutex);
  for (auto dev_it = active_devices.begin(); dev_it != active_devices.end();) {
    try {
      dev_it->second->monitor();
    } catch (...) {
      if (dev_it->second->is_flaky()) {
        uint8_t addr = dev_it->first;
        ret.push_back(addr);
      }
    }
  }
  return ret;
}

void Rackmon::monitor(void) {
  while (!req_stop.load()) {
    if (scan_active.load()) {
      std::vector<uint8_t> dormant = monitor_active();
      for (auto& addr : dormant) {
        mark_dormant(addr);
      }
    }
    last_monitor_time = std::time(0);
    std::this_thread::sleep_for(2min);
  }
}

void Rackmon::scan_all() {
  std::cout << "Starting scan of all devices" << std::endl;
  for (auto& addr : possible_dev_addrs) {
    if (active_devices.find(addr) != active_devices.end())
      continue;
    if (dormant_devices.find(addr) != dormant_devices.end())
      continue;
    probe(addr);
  }
}

void Rackmon::scan() {
  auto it = possible_dev_addrs.begin();
  // circular iterator
  auto next_it = [this](auto& it) {
    it++;
    if (it == possible_dev_addrs.end()) {
      it = possible_dev_addrs.begin();
    }
    return it;
  };

  while (!req_stop.load()) {
    if (force_scan.load()) {
      scan_all();
      force_scan = false;
      continue;
    }

    // Try and recover dormant devices
    recover_dormant();

    // Dont try to waste time scanning known
    // active/dormant devices.
    if (active_devices.find(*it) != active_devices.end() ||
        dormant_devices.find(*it) != dormant_devices.end()) {
      it = next_it(it);
      continue;
    }
    probe(*it);
    it = next_it(it);
    last_scan_time = std::time(0);
    std::this_thread::sleep_for(2min);
  }
}

void Rackmon::start() {
  if (threads.size() != 0)
    throw std::runtime_error("Already running");
  req_stop = false;
  threads.push_back(std::thread(&Rackmon::scan, this));
  threads.push_back(std::thread(&Rackmon::monitor, this));
}

void Rackmon::stop() {
  req_stop = true;
  while (threads.size()) {
    threads.back().join();
    threads.pop_back();
  }
}

void Rackmon::rawCmd(Msg& req, Msg& resp, modbus_time timeout) {
  uint8_t addr = req.addr;
  active_devices.at(addr)->command(req, resp, timeout);
}

void Rackmon::get_monitor_data(std::vector<ModbusDeviceMonitorData>& ret) {
  ret.clear();
  std::shared_lock lock(devices_mutex);
  std::transform(
      active_devices.begin(),
      active_devices.end(),
      std::back_inserter(ret),
      [](auto& kv) { return kv.second->get_monitor_data(); });
}

void Rackmon::get_monitor_status(RackmonStatus& ret) {
  ret.started = threads.size() > 0;
  ret.last_scan = last_scan_time;
  ret.last_monitor = last_monitor_time;

  std::shared_lock lock(devices_mutex);
  ret.active_devices.clear();
  ret.dormant_devices.clear();
  std::transform(
      active_devices.begin(),
      active_devices.end(),
      std::back_inserter(ret.active_devices),
      [](auto& kv) { return kv.second->get_status(); });
  std::transform(
      dormant_devices.begin(),
      dormant_devices.end(),
      std::back_inserter(ret.dormant_devices),
      [](auto& kv) { return kv.second->get_status(); });
}

void to_json(json& j, const RackmonStatus& m) {
  j["running_status"] = m.started;
  j["last_scan"] = m.last_scan;
  j["active_devices"] = m.active_devices;
  j["dormant_devices"] = m.dormant_devices;
}
