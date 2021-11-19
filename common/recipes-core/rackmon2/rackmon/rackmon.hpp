#pragma once
#include <atomic>
#include <shared_mutex>
#include <thread>
#include "modbus.hpp"
#include "modbus_device.hpp"
#include "pollthread.hpp"

struct RackmonStatus {
  bool started;
  time_t last_scan;
  time_t last_monitor;
  std::vector<ModbusDeviceStatus> active_devices;
  std::vector<ModbusDeviceStatus> dormant_devices;
};
void to_json(nlohmann::json& j, const RackmonStatus& m);

class Rackmon {
  static constexpr time_t dormant_min_inactive_time = 300;
  static constexpr modbus_time probe_timeout = std::chrono::milliseconds(50);
  std::vector<std::unique_ptr<PollThread<Rackmon>>> threads{};
  // Has to be before defining active or dormant devices
  // to ensure users get destroyed before the interface.
  std::vector<std::unique_ptr<Modbus>> interfaces{};
  RegisterMapDatabase regmap_db{};

  mutable std::shared_mutex devices_mutex{};

  // These devices are reachable and are functioning
  // properly.
  std::map<uint8_t, std::unique_ptr<ModbusDevice>> active_devices{};

  // These devices were active at one point. Now, they
  // are either bad or removed. These will be scanned
  // more regularly than the unknown addresses.
  std::map<uint8_t, std::unique_ptr<ModbusDevice>> dormant_devices{};

  // contains all the possible address allowed by currently
  // loaded register maps. A majority of these are not expected
  // to exist, but are candidates for a scan.
  std::vector<uint8_t> possible_dev_addrs{};

  // As an optimization, devices are normally scanned one by one
  // This allows someone to initiate a forced full scan.
  // This mimicks a restart of rackmond.
  std::atomic<bool> force_scan = true;

  // Timestamps of last scan
  time_t last_scan_time;
  time_t last_monitor_time;

  // Probe an interface for the presence of the address.
  bool probe(Modbus& iface, uint8_t addr);
  // Probe all interfaces for the presence of the address.
  void probe(uint8_t addr);

  // --------- Private Methods --------

  // Marks an address as dormant
  void mark_dormant(uint8_t addr);

  // Marks an address as active
  void mark_active(uint8_t addr);

  // probe dormant devices and return recovered devices.
  std::vector<uint8_t> inspect_dormant();
  // Try and recover dormant devices.
  void recover_dormant();

  bool is_device_known(uint8_t);

  // Does a monitoring scan of all defined registers
  // returns detected dormant devices.
  std::vector<uint8_t> monitor_active();
  // Monitor loop. Blocks forever as long as req_stop is true.
  void monitor();

  // Scan all possible devices. Skips active/dormant devices.
  void scan_all();

  // Scan loop. Blocks forever as long as req_stop is true.
  void scan();

 public:
  // Load configuration, preferable before starting, but can be
  // done at any time, but this is a one time only.
  void load(const std::string& conf_path, const std::string& regmap_dir);

  // Start the monitoring/scanning loops
  void start();
  // Stop the monitoring/scanning loops
  void stop();

  // Force rackmond to do a full scan on the next scan loop.
  void force_scan_all() {
    force_scan = true;
  }

  // Executes the Raw command. Throws an exception on error.
  void rawCmd(Msg& req, Msg& resp, modbus_time timeout);

  // Get monitored data
  void get_monitor_data(std::vector<ModbusDeviceMonitorData>& ret);

  // Get status of devices
  void get_monitor_status(RackmonStatus& ret);

  // TODO, this actually uses the formatting options in the regmap.
  std::string getMonitorDataParsed() {
    return "";
  }
};
