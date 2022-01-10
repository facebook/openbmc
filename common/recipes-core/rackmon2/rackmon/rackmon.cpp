#include "rackmon.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include "log.hpp"

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
    interfaces.push_back(make_interface());
    interfaces.back()->initialize(iface_conf);
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
  next_dev_it = possible_dev_addrs.begin();
}

bool Rackmon::probe(Modbus& iface, uint8_t addr) {
  const RegisterMap& rmap = regmap_db.at(addr);
  std::vector<uint16_t> v(1);
  try {
    ReadHoldingRegistersReq req(addr, rmap.probe_register, v.size());
    ReadHoldingRegistersResp resp(addr, v);
    iface.command(req, resp, rmap.default_baudrate, probe_timeout);
    std::unique_lock lock(devices_mutex);
    devices[addr] = std::make_unique<ModbusDevice>(iface, addr, rmap);
    logInfo << std::hex << std::setw(2) << std::setfill('0') << "Found "
             << int(addr) << " on " << iface.name() << std::endl;
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
      const RegisterMap& rmap = regmap_db.at(it.first);
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
    dev_it.second->monitor();
  }
  last_monitor_time = std::time(0);
}

bool Rackmon::is_device_known(uint8_t addr) {
  std::shared_lock lk(devices_mutex);
  return devices.find(addr) != devices.end();
}

void Rackmon::scan_all() {
  logInfo << "Starting scan of all devices" << std::endl;
  for (auto& addr : possible_dev_addrs) {
    if (is_device_known(addr))
      continue;
    probe(addr);
  }
}

void Rackmon::scan() {
  // Circular iterator.
  if (force_scan.load()) {
    scan_all();
    force_scan = false;
    return;
  }

  // Probe for the address only if we already dont know it.
  if (!is_device_known(*next_dev_it)) {
    probe(*next_dev_it);
    last_scan_time = std::time(0);
  }

  // Try and recover dormant devices
  recover_dormant();
  if (++next_dev_it == possible_dev_addrs.end())
    next_dev_it = possible_dev_addrs.begin();
}

void Rackmon::start(poll_interval interval) {
  auto start_thread = [this](auto func, auto intr) {
    threads.emplace_back(
        std::make_unique<PollThread<Rackmon>>(func, this, intr));
    threads.back()->start();
  };
  if (threads.size() != 0)
    throw std::runtime_error("Already running");
  start_thread(&Rackmon::scan, interval);
  start_thread(&Rackmon::monitor, interval);
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
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "rawcmd::" + std::to_string(int(req.addr)), profile_store);
  std::shared_lock lock(devices_mutex);
  if (!devices.at(addr)->is_active()) {
    throw std::exception();
  }
  devices.at(addr)->command(req, resp, timeout);
  // Add back the CRC removed by validate.
  resp.len += 2;
}

void Rackmon::ReadHoldingRegisters(
    uint8_t addr,
    uint16_t reg_off,
    std::vector<uint16_t>& regs) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "readRegs::" + std::to_string(int(addr)), profile_store);
  std::shared_lock lock(devices_mutex);
  if (!devices.at(addr)->is_active()) {
    throw std::exception();
  }
  devices.at(addr)->ReadHoldingRegisters(reg_off, regs);
}

void Rackmon::WriteSingleRegister(
    uint8_t addr,
    uint16_t reg_off,
    uint16_t value) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "writeReg::" + std::to_string(int(addr)), profile_store);
  std::shared_lock lock(devices_mutex);
  if (!devices.at(addr)->is_active()) {
    throw std::exception();
  }
  devices.at(addr)->WriteSingleRegister(reg_off, value);
}

void Rackmon::WriteMultipleRegisters(
    uint8_t addr,
    uint16_t reg_off,
    std::vector<uint16_t>& values) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "writeRegs::" + std::to_string(int(addr)), profile_store);
  std::shared_lock lock(devices_mutex);
  if (!devices.at(addr)->is_active()) {
    throw std::exception();
  }
  devices.at(addr)->WriteMultipleRegisters(reg_off, values);
}

void Rackmon::ReadFileRecord(uint8_t addr, std::vector<FileRecord>& records) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "ReadFile::" + std::to_string(int(addr)), profile_store);
  std::shared_lock lock(devices_mutex);
  if (!devices.at(addr)->is_active()) {
    throw std::exception();
  }
  devices.at(addr)->ReadFileRecord(records);
}

std::vector<ModbusDeviceStatus> Rackmon::list_devices() {
  std::shared_lock lock(devices_mutex);
  std::vector<ModbusDeviceStatus> ret;
  std::transform(
      devices.begin(), devices.end(), std::back_inserter(ret), [](auto& kv) {
        return kv.second->get_status();
      });
  return ret;
}

void Rackmon::get_raw_data(std::vector<ModbusDeviceRawData>& ret) {
  ret.clear();
  std::shared_lock lock(devices_mutex);
  std::transform(
      devices.begin(), devices.end(), std::back_inserter(ret), [](auto& kv) {
        return kv.second->get_raw_data();
      });
}

void Rackmon::get_fmt_data(std::vector<ModbusDeviceFmtData>& ret) {
  ret.clear();
  std::shared_lock lock(devices_mutex);
  std::transform(
      devices.begin(), devices.end(), std::back_inserter(ret), [](auto& kv) {
        return kv.second->get_fmt_data();
      });
}

void Rackmon::get_value_data(std::vector<ModbusDeviceValueData>& ret) {
  ret.clear();
  std::shared_lock lock(devices_mutex);
  std::transform(
      devices.begin(), devices.end(), std::back_inserter(ret), [](auto& kv) {
        return kv.second->get_value_data();
      });
}

std::string Rackmon::get_profile_data() {
  std::stringstream ss;
  profile_store.swap(ss);
  return ss.str();
}
