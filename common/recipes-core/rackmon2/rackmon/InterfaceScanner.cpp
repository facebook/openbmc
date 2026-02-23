// Copyright 2021-present Facebook. All Rights Reserved.
#include "InterfaceScanner.h"
#include <iomanip>
#include "Log.h"

namespace rackmon {

bool InterfaceScanner::probe(const DeviceLocation& key) {
  if (!key.interface.isPresent()) {
    return false;
  }
  for (auto it = registerMapDB_.find(key.addr); it != registerMapDB_.end();
       ++it) {
    const auto& rmap = *it;
    std::vector<uint16_t> v(1);
    try {
      ReadHoldingRegistersReq req(key.addr, rmap.probeRegister, v.size());
      ReadHoldingRegistersResp resp(key.addr, v);
      key.interface.command(
          req, resp, rmap.baudrate, kProbeTimeout, rmap.parity);
      deviceInventory_.addDevice(key, rmap);
      logInfo << std::setw(2) << std::setfill('0') << "Found " << key << " on "
              << key.interface.name() << std::endl;
      return true;
    } catch (std::exception&) {
      // Exceptions are expected for unfound addresses.
    }
  }
  return false;
}

void InterfaceScanner::fullScan() {
  logInfo << "Starting scan of all devices on " << interface_.get()->name()
          << std::endl;
  bool atLeastOne = false;
  // Retry the scan loop to ensure we discover any flaky
  // devices which might have missed the first loop.
  for (int i = 0; i < kScanNumRetry; i++) {
    DeviceLocationIterator locationIterator(registerMapDB_, interface_);
    while (locationIterator != locationIterator.end()) {
      DeviceLocation key = *locationIterator;
      ++locationIterator;
      if (deviceInventory_.isDeviceKnown(key)) {
        continue;
      }
      if (reqForceScan_.load() == false) {
        logWarn << "Full scan aborted" << std::endl;
        return;
      }
      if (probe(key)) {
        atLeastOne = true;
      }
    }
  }
  logInfo << "Finished scan of all devices on " << interface_.get()->name()
          << std::endl;
  // When scan is complete, request for a monitor.
  if (atLeastOne) {
    // TODO:  FIX THIS LATER
    // triggerMonitorThread();
  }
  reqForceScan_ = false;
}

void InterfaceScanner::scan() {
  // Circular iterator.
  if (reqForceScan_.load()) {
    fullScan();
    return;
  }

  // Probe for the address only if we already dont know it.
  if (!deviceInventory_.isDeviceKnown(**nextDeviceToProbe_)) {
    if (probe(**nextDeviceToProbe_)) {
      // TODO:  FIX THIS LATER
      // triggerMonitorThread();
    }
  }

  // Try and recover dormant devices
  deviceInventory_.recoverDormant(getTime());
  ++(*nextDeviceToProbe_);
  if (*nextDeviceToProbe_ == nextDeviceToProbe_->end()) {
    nextDeviceToProbe_ =
        std::make_unique<DeviceLocationIterator>(registerMapDB_, interface_);
  }
}

} // namespace rackmon
