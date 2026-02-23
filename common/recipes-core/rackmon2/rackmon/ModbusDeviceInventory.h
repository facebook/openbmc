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
  std::vector<DeviceLocation> inspectDormant(time_t curr) const;

  mutable std::shared_mutex devicesMutex_{};

  // These devices discovered on actively monitored buses
  std::map<DeviceLocation, std::shared_ptr<ModbusDevice>> devices_{};

 public:
  void addDevice(DeviceLocation key, const RegisterMap& rmap);
  std::vector<std::shared_ptr<ModbusDevice>> getAllModbusDevices() const;

  // Return the device given location and optional port.
  std::shared_ptr<ModbusDevice> getModbusDevice(
      uint8_t addr,
      std::optional<uint8_t> port);

  // Try and recover dormant devices.
  void recoverDormant(time_t now);

  bool isDeviceKnown(DeviceLocation);

  // Monitor loop. Blocks forever as long as req_stop is true.
  void monitor();

  void setExclusiveModeForAll(bool enable);
};
} // namespace rackmon
