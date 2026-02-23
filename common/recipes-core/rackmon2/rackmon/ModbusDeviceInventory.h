// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <map>
#include <shared_mutex>
#include <vector>
#include "DeviceLocationFilter.h"
#include "DeviceLocationIterator.h"
#include "ModbusDevice.h"

namespace rackmon {
class ModbusDeviceInventory {
 private:
  static constexpr time_t kDormantMinInactiveTime = 300;

  // probe dormant devices and return recovered devices.
  std::vector<DeviceLocation> inspectDormant() const;

  mutable std::shared_mutex devicesMutex_{};

  // These devices discovered on actively monitored buses
  std::map<DeviceLocation, std::shared_ptr<ModbusDevice>> devices_{};

  virtual time_t getTime() const {
    return std::time(nullptr);
  }

 public:
  void addDevice(DeviceLocation key, const RegisterMap& rmap);
  std::vector<std::shared_ptr<ModbusDevice>> getAllModbusDevices() const;

  // Return the device given location and optional port.
  std::shared_ptr<ModbusDevice> getModbusDevice(
      uint8_t addr,
      std::optional<uint8_t> port);

  // Try and recover dormant devices.
  void recoverDormant();

  bool isDeviceKnown(DeviceLocation);

  void setExclusiveModeForAll(bool enable);

  virtual ~ModbusDeviceInventory() = default;
};
} // namespace rackmon
