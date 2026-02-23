// Copyright 2021-present Facebook. All Rights Reserved.
#include "Rackmon.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <optional>
#include "DeviceLocationFilter.h"
#include "Log.h"

#if (defined(__llvm__) && (__clang_major__ < 9)) || \
    (!defined(__llvm__) && (__GNUC__ < 8))
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

using nlohmann::json;
using namespace std::literals;

namespace rackmon {

bool ModbusDeviceFilter::contains(const ModbusDevice& dev) const {
  // If neither is provided, its considered as a
  // shortcut of "all".
  if (!locationFilter && !typeFilter) {
    return true;
  }
  if (locationFilter &&
      locationFilter->contains(dev.getDevicePort(), dev.getDeviceAddress())) {
    return true;
  }
  if (typeFilter &&
      typeFilter->find(dev.getDeviceType()) != typeFilter->end()) {
    return true;
  }
  return false;
}

void Rackmon::loadInterface(const nlohmann::json& config) {
  std::shared_lock lk(threadMutex_);
  assertNotStarted("Cannot load configuration when started");

  if (!interfaces_.empty()) {
    throw std::runtime_error("Interfaces already loaded");
  }
  for (const auto& ifaceConf : config["interfaces"]) {
    interfaces_.push_back(makeInterface());
    interfaces_.back()->initialize(ifaceConf);
  }
}

void Rackmon::loadRegisterMap(const nlohmann::json& config) {
  std::shared_lock lk(threadMutex_);
  assertNotStarted("Cannot load configuration when started");

  registerMapDB_.load(config);
}

void Rackmon::load(const std::string& confPath, const std::string& regmapDir) {
  auto getJSON = [](const std::string& fileName) {
    std::ifstream ifs(fileName);
    json contents;
    try {
      ifs >> contents;
    } catch (const nlohmann::json::parse_error& ex) {
      logError << "Error loading: " << fileName << " byte: " << ex.byte
               << std::endl;
      throw;
    }
    ifs.close();
    return contents;
  };
  loadInterface(getJSON(confPath));

  for (auto const& dir_entry : std::filesystem::directory_iterator{regmapDir}) {
    loadRegisterMap(getJSON(dir_entry.path().string()));
  }
}

void Rackmon::start(PollThreadTime interval) {
  std::unique_lock lk(threadMutex_);
  logInfo << "Start was requested" << std::endl;
  assertNotStarted("Already running");

  deviceInventory_.setExclusiveModeForAll(false);
  std::transform(
      interfaces_.begin(),
      interfaces_.end(),
      std::back_inserter(scanners_),
      [this, interval](const auto& interface) {
        return this->createInterfaceScanner(interface, interval);
      });
}

void Rackmon::stop(bool forceStop) {
  std::unique_lock lk(threadMutex_);
  logInfo << "Stop was requested" << std::endl;
  deviceInventory_.setExclusiveModeForAll(true);

  if (forceStop) {
    for (const auto& st : scanners_) {
      st->endForceScan();
    }
  }
  // TODO We probably need a timer to ensure we
  // are not waiting here forever.
  scanners_.clear();
}

void Rackmon::forceScan() {
  logInfo << "Force Scan was requested" << std::endl;
  std::shared_lock lk(threadMutex_);
  for (const auto& st : scanners_) {
    st->runDeviceScanner(true, true);
  }
}

void Rackmon::rawCmd(
    Request& req,
    std::optional<uint16_t> uniqueDevAddr,
    Response& resp,
    ModbusTime timeout) {
  std::optional<uint8_t> port = std::nullopt;
  uint8_t addr = req.addr;
  if (uniqueDevAddr.has_value()) {
    uint8_t addr2;
    std::tie(port, addr2) =
        DeviceLocationFilter::decompose(uniqueDevAddr.value());
    if (addr != addr2) {
      throw std::runtime_error(
          "Mismatch between device address and unique device address");
    }
  }
  RACKMON_PROFILE_SCOPE(raw_cmd, "rawcmd::" + std::to_string(int(req.addr)));

  deviceInventory_.getModbusDevice(addr, port)->command(req, resp, timeout);
  // Add back the CRC removed by validate.
  resp.len += 2;
}

void Rackmon::readHoldingRegisters(
    uint8_t deviceAddress,
    std::optional<uint8_t> port,
    uint16_t registerOffset,
    std::vector<uint16_t>& registerContents,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "readRegs::" + std::to_string(int(deviceAddress)));

  deviceInventory_.getModbusDevice(deviceAddress, port)
      ->readHoldingRegisters(registerOffset, registerContents, timeout);
}

void Rackmon::writeSingleRegister(
    uint8_t deviceAddress,
    std::optional<uint8_t> port,
    uint16_t registerOffset,
    uint16_t value,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "writeReg::" + std::to_string(int(deviceAddress)));

  deviceInventory_.getModbusDevice(deviceAddress, port)
      ->writeSingleRegister(registerOffset, value, timeout);
}

void Rackmon::writeMultipleRegisters(
    uint8_t deviceAddress,
    std::optional<uint8_t> port,
    uint16_t registerOffset,
    std::vector<uint16_t>& values,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "writeRegs::" + std::to_string(int(deviceAddress)));

  deviceInventory_.getModbusDevice(deviceAddress, port)
      ->writeMultipleRegisters(registerOffset, values, timeout);
}

void Rackmon::readFileRecord(
    uint8_t deviceAddress,
    std::optional<uint8_t> port,
    std::vector<FileRecord>& records,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "ReadFile::" + std::to_string(int(deviceAddress)));
  deviceInventory_.getModbusDevice(deviceAddress, port)
      ->readFileRecord(records, timeout);
}

std::vector<ModbusDeviceInfo> Rackmon::listDevices() const {
  auto allDevices = deviceInventory_.getAllModbusDevices();
  std::vector<ModbusDeviceInfo> devices;
  std::transform(
      allDevices.begin(),
      allDevices.end(),
      std::back_inserter(devices),
      [](auto& kv) { return kv->getInfo(); });
  return devices;
}

void Rackmon::getRawData(std::vector<ModbusDeviceRawData>& data) const {
  data.clear();
  auto allDevices = deviceInventory_.getAllModbusDevices();
  std::transform(
      allDevices.begin(),
      allDevices.end(),
      std::back_inserter(data),
      [](auto& kv) { return kv->getRawData(); });
}

void Rackmon::getValueData(
    std::vector<ModbusDeviceValueData>& data,
    const ModbusDeviceFilter& devFilter,
    const ModbusRegisterFilter& regFilter,
    bool latestValueOnly) const {
  data.clear();
  auto allDevices = deviceInventory_.getAllModbusDevices();
  for (const auto& kv : allDevices) {
    const ModbusDevice& dev = *kv;
    if (devFilter.contains(dev)) {
      data.push_back(dev.getValueData(regFilter, latestValueOnly));
    }
  }
}

void Rackmon::reload(
    const ModbusDeviceFilter& devFilter,
    const ModbusRegisterFilter& regFilter) {
  auto allDevices = deviceInventory_.getAllModbusDevices();
  for (const auto& kv : allDevices) {
    ModbusDevice& dev = *kv;
    if (devFilter.contains(dev)) {
      logInfo << "Force Reloading: " << +dev.getDeviceAddress() << std::endl;
      dev.forceReloadRegisters(regFilter);
    }
  }
}

} // namespace rackmon
