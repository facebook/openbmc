#include "rackmon.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
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
    devices[addr] = std::make_unique<ModbusDevice>(iface, addr, rmap);
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

std::vector<uint8_t> Rackmon::inspect_dormant() {
  time_t curr = std::time(0);
  std::vector<uint8_t> ret{};
  std::shared_lock lock(devices_mutex);
  for (const auto& it : devices) {
    if (it.second->is_active())
      continue;
    // If its more than 300s since last activity, start probing it.
    // change to something larger if required.
    if ((it.second->last_active() + dormant_min_inactive_time) < curr) {
      RegisterMap& rmap = regmap_db.at(it.first);
      uint16_t probe = rmap.probe_register;
      std::vector<uint16_t> v(1);
      try {
        uint8_t addr = it.first;
        it.second->ReadHoldingRegisters(probe, v);
        ret.push_back(addr);
      } catch (...) {
        continue;
      }
    }
  }
  return ret;
}

void Rackmon::recover_dormant() {
  std::vector<uint8_t> candidates = inspect_dormant();
  for (auto& addr : candidates) {
    std::unique_lock lock(devices_mutex);
    devices.at(addr)->set_active();
  }
}

void Rackmon::monitor(void) {
  std::shared_lock lock(devices_mutex);
  for (const auto& dev_it : devices) {
    if (!dev_it.second->is_active())
      continue;
    try {
      dev_it.second->monitor();
    } catch (std::exception& e) {
      std::cout << "Caught: " << e.what() << std::endl;
    }
  }
  last_monitor_time = std::time(0);
}

bool Rackmon::is_device_known(uint8_t addr) {
  std::shared_lock lk(devices_mutex);
  return devices.find(addr) != devices.end();
}

void Rackmon::scan_all() {
  std::cout << "Starting scan of all devices" << std::endl;
  for (auto& addr : possible_dev_addrs) {
    if (is_device_known(addr))
      continue;
    probe(addr);
  }
}

void Rackmon::scan() {
  // Circular iterator.
  static auto it = possible_dev_addrs.begin();
  if (force_scan.load()) {
    scan_all();
    force_scan = false;
    return;
  }

  // Probe for the address only if we already dont know it.
  if (!is_device_known(*it)) {
    probe(*it);
    last_scan_time = std::time(0);
  }

  // Try and recover dormant devices
  recover_dormant();
  if (++it == possible_dev_addrs.end())
    it = possible_dev_addrs.begin();
}

void Rackmon::start() {
  auto start_thread = [this](auto func) {
    threads.emplace_back(
        std::make_unique<PollThread<Rackmon>>(func, this, 2min));
    threads.back()->start();
  };
  if (threads.size() != 0)
    throw std::runtime_error("Already running");
  start_thread(&Rackmon::scan);
  start_thread(&Rackmon::monitor);
}

void Rackmon::stop() {
  // TODO We probably need a timer to ensure we
  // are not waiting here forever.
  while (threads.size() > 0) {
    threads.back()->stop();
    threads.pop_back();
  }
}

void Rackmon::rawCmd(Msg& req, Msg& resp, modbus_time timeout) {
  uint8_t addr = req.addr;
  std::shared_lock lock(devices_mutex);
  if (!devices.at(addr)->is_active()) {
    throw std::exception();
  }
  devices.at(addr)->command(req, resp, timeout);
}

void Rackmon::get_monitor_data(std::vector<ModbusDeviceMonitorData>& ret) {
  ret.clear();
  std::shared_lock lock(devices_mutex);
  std::transform(
      devices.begin(), devices.end(), std::back_inserter(ret), [](auto& kv) {
        return kv.second->get_monitor_data();
      });
}

void Rackmon::get_monitor_status(RackmonStatus& ret) {
  ret.started = threads.size() > 0;
  ret.last_scan = last_scan_time;
  ret.last_monitor = last_monitor_time;

  std::shared_lock lock(devices_mutex);
  ret.devices.clear();
  std::transform(
      devices.begin(),
      devices.end(),
      std::back_inserter(ret.devices),
      [](auto& kv) { return kv.second->get_status(); });
}

void Rackmon::get_monitor_data_formatted(
    std::vector<ModbusDeviceFormattedData>& ret) {
  ret.clear();
  std::shared_lock lock(devices_mutex);
  std::transform(
      devices.begin(), devices.end(), std::back_inserter(ret), [](auto& kv) {
        return kv.second->get_formatted_data();
      });
}

void to_json(json& j, const RackmonStatus& m) {
  j["running_status"] = m.started;
  j["last_scan"] = m.last_scan;
  j["devices"] = m.devices;
}
