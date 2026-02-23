// Copyright 2021-present Facebook. All Rights Reserved.
#include "ModbusDeviceInventory.h"
#include "Log.h"
namespace rackmon {

void ModbusDeviceInventory::addDevice(
    DeviceLocation key,
    const RegisterMap& rmap) {
  std::unique_lock lock(devicesMutex_);
  devices_[key] = std::make_shared<ModbusDevice>(key.interface, key.addr, rmap);
}

std::vector<DeviceLocation> ModbusDeviceInventory::inspectDormant(
    time_t curr) const {
  std::vector<DeviceLocation> ret{};
  std::shared_lock lock(devicesMutex_);
  for (const auto& it : devices_) {
    if (it.second->isActive()) {
      continue;
    }
    // If its more than 300s since last activity, start probing it.
    // change to something larger if required.
    if ((it.second->lastActive() + kDormantMinInactiveTime) < curr) {
      const RegisterMap& rmap = it.second->getRegisterMap();
      uint16_t probe = rmap.probeRegister;
      std::vector<uint16_t> v(1);
      try {
        it.second->readHoldingRegisters(probe, v);
        ret.push_back(it.first);
      } catch (...) {
        continue;
      }
    }
  }
  return ret;
}

void ModbusDeviceInventory::recoverDormant(time_t now) {
  const std::vector<DeviceLocation> dormant = inspectDormant(now);
  for (const auto& key : dormant) {
    std::shared_lock lock(devicesMutex_);
    devices_.at(key)->setActive();
  }
}

void ModbusDeviceInventory::monitor() {
  std::shared_lock lock(devicesMutex_);
  for (const auto& dev_it : devices_) {
    if (!dev_it.second->isActive()) {
      continue;
    }
    dev_it.second->reloadAllRegisters();
  }
}

bool ModbusDeviceInventory::isDeviceKnown(DeviceLocation key) {
  std::shared_lock lk(devicesMutex_);
  return devices_.find(key) != devices_.end();
}

std::vector<std::shared_ptr<ModbusDevice>>
ModbusDeviceInventory::getAllModbusDevices() const {
  std::shared_lock lock(devicesMutex_);
  std::vector<std::shared_ptr<ModbusDevice>> result;
  result.reserve(devices_.size());
  std::transform(
      devices_.begin(),
      devices_.end(),
      std::back_inserter(result),
      [](const auto& kv) { return kv.second; });
  return result;
}
std::shared_ptr<ModbusDevice> ModbusDeviceInventory::getModbusDevice(
    uint8_t addr,
    std::optional<uint8_t> port) {
  std::shared_ptr<ModbusDevice> d = nullptr;
  std::stringstream err;
  std::stringstream ss;
  ss << "0x" << std::hex << +addr << " port: ";
  if (port.has_value()) {
    ss << std::dec << port.value();
  } else {
    ss << "NULL";
  }
  std::string location = ss.str();
  const DeviceLocationFilter filter({{port, addr}});
  std::shared_lock lock(devicesMutex_);
  for (auto device = devices_.begin(); device != devices_.end(); ++device) {
    const DeviceLocation& devLocation = device->first;

    if (filter.contains(devLocation.interface.getPort(), devLocation.addr)) {
      // Addresses match and either the ports match or (for backwards
      // compatibility) no port is specified
      if (d == nullptr) {
        d = device->second;
      } else {
        err << "Multiple devices found at " << location
            << " during probe sequence";
        throw std::runtime_error(err.str());
      }
    }
  }

  if (d != nullptr) {
    if (!d->isActive()) {
      err << "Device at " << location << " is not active";
      throw std::runtime_error(err.str());
    }
    return d;
  }

  err << "No device found at " << location << " during probe sequence";
  throw std::out_of_range(err.str());
}

void ModbusDeviceInventory::setExclusiveModeForAll(bool enable) {
  std::shared_lock lk(devicesMutex_);
  for (const auto& [key, device] : devices_) {
    device->setExclusiveMode(enable);
  }
}
}; // namespace rackmon
