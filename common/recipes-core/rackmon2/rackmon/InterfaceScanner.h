// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <atomic>
#include "DeviceLocationIterator.h"
#include "ModbusDeviceInventory.h"
#include "PollThread.h"
#include "Register.h"

namespace rackmon {
class InterfaceScanner {
 protected:
  static constexpr int kScanNumRetry = 3;
  static constexpr ModbusTime kProbeTimeout = std::chrono::milliseconds(70);

  std::shared_ptr<Modbus> interface_;

  ModbusDeviceInventory& deviceInventory_;
  const RegisterMapDatabase& registerMapDB_;
  std::unique_ptr<DeviceLocationIterator> nextDeviceToProbe_;

  virtual time_t getTime() {
    return std::time(nullptr);
  }

 private:
  // Thread that periodically scans the interface for new devices
  // and recovers dormant ones.
  PollThread<InterfaceScanner> deviceScanner_;

  // Thread that updates the registers of all active devices on the interface
  PollThread<InterfaceScanner> registerMonitor_;
  // As an optimization, devices are normally scanned one by one
  // This allows someone to initiate a forced full scan.
  // This mimicks a restart of rackmond.
  std::atomic<bool> reqForceScan_ = true;

 public:
  InterfaceScanner(
      std::shared_ptr<Modbus> interface,
      ModbusDeviceInventory& deviceInventory,
      const RegisterMapDatabase& registerMapDB,
      PollThreadTime interval)
      : interface_(std::move(interface)),
        deviceInventory_(deviceInventory),
        registerMapDB_(registerMapDB),
        nextDeviceToProbe_(
            std::make_unique<DeviceLocationIterator>(
                registerMapDB,
                interface_)),
        deviceScanner_(
            PollThread<InterfaceScanner>(
                &InterfaceScanner::scan,
                this,
                interval)),
        registerMonitor_(
            PollThread<InterfaceScanner>(
                &InterfaceScanner::updateDeviceRegisters,
                this,
                std::chrono::seconds(registerMapDB_.minMonitorInterval()))) {
    deviceScanner_.start();
    registerMonitor_.start();
  }

  virtual ~InterfaceScanner() {
    registerMonitor_.stop();
    deviceScanner_.stop();
  }

  // If there is a forced scan ongoing this will end it before it continues
  // to the next item
  void endForceScan() {
    reqForceScan_ = false;
  }

  // Scan all possible devices. Skips active/dormant devices.
  void fullScan();

  // Probe an interface for the presence of the address.
  bool probe(const DeviceLocation&);

  void runDeviceScanner(bool nonBlocking, bool forceScan) {
    if (forceScan) {
      reqForceScan_ = true;
    }
    deviceScanner_.tick(nonBlocking);
  }

  // Scan loop. Blocks forever as long as req_stop is true.
  void scan();

  // Iterates over all devices on this interface and updates registers of active
  // devices
  void updateDeviceRegisters() {
    auto allDevices = deviceInventory_.getAllModbusDevices();
    for (const auto& device : allDevices) {
      if (&device->getInterface() == interface_.get() && device->isActive()) {
        device->reloadAllRegisters();
      }
    }
  }

  // Explicitly disabling copying and moving
  InterfaceScanner(const InterfaceScanner&) = delete;
  InterfaceScanner& operator=(const InterfaceScanner&) = delete;
  InterfaceScanner(InterfaceScanner&&) = delete;
  InterfaceScanner& operator=(InterfaceScanner&&) = delete;

  // Exposed for testing - asynchronously updates registers of active devices
  void runRegisterMonitor() {
    registerMonitor_.tick();
  }
};

} // namespace rackmon
