// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <atomic>
#include <set>
#include <shared_mutex>
#include <thread>
#include "DeviceLocationFilter.h"
#include "DeviceLocationIterator.h"
#include "InterfaceScanner.h"
#include "Modbus.h"
#include "ModbusDevice.h"
#include "ModbusDeviceInventory.h"
#include "PollThread.h"

namespace rackmon {

struct ModbusDeviceFilter {
  std::optional<DeviceLocationFilter> locationFilter{};
  std::optional<std::set<std::string>> typeFilter{};
  bool contains(const ModbusDevice& dev) const;
};

class Rackmon {
  std::shared_mutex threadMutex_{};
  std::vector<std::unique_ptr<InterfaceScanner>> scanners_;
  // Has to be before defining active or dormant devices
  // to ensure users get destroyed before the interface.
  std::vector<std::shared_ptr<Modbus>> interfaces_{};

  void assertNotStarted(const std::string& error) const {
    if (!scanners_.empty()) {
      throw std::runtime_error(error);
    }
  }

  // --------- Private Methods --------
 protected:
  RegisterMapDatabase registerMapDB_{};
  ModbusDeviceInventory deviceInventory_;

  void triggerScanThreads() const {
    for (const auto& st : scanners_) {
      st->runDeviceScanner(false, false);
    }
  }

  virtual std::unique_ptr<Modbus> makeInterface() {
    return std::make_unique<Modbus>();
  }
  const RegisterMapDatabase& getRegisterMapDatabase() const {
    return registerMapDB_;
  }

 public:
  virtual ~Rackmon() {
    stop();
  }

  // Load Interface configuration.
  void loadInterface(const nlohmann::json& config);

  // Load a register map into the internal database.
  void loadRegisterMap(const nlohmann::json& config);

  // Load configuration, preferable before starting, but can be
  // done at any time, but this is a one time only.
  void load(const std::string& confPath, const std::string& regmapDir);

  // Start the monitoring/scanning loops
  void start(PollThreadTime interval = std::chrono::minutes(3));
  // Stop the monitoring/scanning loops
  void stop(bool forceStop = true);

  // Force rackmond to do a full scan on the next scan loop.
  void forceScan();

  // Executes the Raw command. Throws an exception on error.
  void rawCmd(
      Request& req,
      std::optional<uint16_t> uniqueDevAddr,
      Response& resp,
      ModbusTime timeout);

  // Read registers
  void readHoldingRegisters(
      uint8_t deviceAddress,
      std::optional<uint8_t> port,
      uint16_t registerOffset,
      std::vector<uint16_t>& registerContents,
      ModbusTime timeout = ModbusTime::zero());

  // Write Single Register
  void writeSingleRegister(
      uint8_t deviceAddress,
      std::optional<uint8_t> port,
      uint16_t registerOffset,
      uint16_t value,
      ModbusTime timeout = ModbusTime::zero());

  // Write multiple registers
  void writeMultipleRegisters(
      uint8_t deviceAddress,
      std::optional<uint8_t> port,
      uint16_t registerOffset,
      std::vector<uint16_t>& values,
      ModbusTime timeout = ModbusTime::zero());

  // Read File Record
  void readFileRecord(
      uint8_t deviceAddress,
      std::optional<uint8_t> port,
      std::vector<FileRecord>& records,
      ModbusTime timeout = ModbusTime::zero());

  // Get status of devices
  std::vector<ModbusDeviceInfo> listDevices() const;

  // Get monitored data
  void getRawData(std::vector<ModbusDeviceRawData>& data) const;

  // Get value data
  void getValueData(
      std::vector<ModbusDeviceValueData>& data,
      const ModbusDeviceFilter& devFilter = {},
      const ModbusRegisterFilter& regFilter = {},
      bool latestValueOnly = false) const;

  void reload(
      const ModbusDeviceFilter& devFilter,
      const ModbusRegisterFilter& regFilter);

  // For interface-specific scanning and monitoring
  void triggerMonitorThreads() {
    std::shared_lock lk(threadMutex_);
    for (const auto& st : scanners_) {
      st->runRegisterMonitor();
    }
  }

  virtual std::unique_ptr<InterfaceScanner> createInterfaceScanner(
      std::shared_ptr<Modbus> interface,
      PollThreadTime interval) {
    return std::make_unique<InterfaceScanner>(
        interface, deviceInventory_, registerMapDB_, interval);
  }
};

} // namespace rackmon
