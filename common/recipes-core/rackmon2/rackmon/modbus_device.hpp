#include <nlohmann/json.hpp>
#include <ctime>
#include <iostream>
#include "modbus.hpp"
#include "modbus_cmds.hpp"
#include "regmap.hpp"

enum ModbusDeviceMode {
  ACTIVE = 0,
  DORMANT = 1
};

struct ModbusDeviceStatus {
  static constexpr uint32_t max_consecutive_failures = 10;
  uint8_t addr = 0;
  uint32_t baudrate = 0;
  uint32_t crc_failures = 0;
  uint32_t timeouts = 0;
  uint32_t misc_failures = 0;
  time_t last_active = 0;
  uint32_t num_consecutive_failures = 0;
  ModbusDeviceMode get_mode() const {
    return num_consecutive_failures < max_consecutive_failures ?
      ModbusDeviceMode::ACTIVE : ModbusDeviceMode::DORMANT;
  }
};
void to_json(nlohmann::json& j, const ModbusDeviceStatus& m);

struct ModbusDeviceMonitorData : public ModbusDeviceStatus {
  std::vector<RegisterValueStore> register_list{};
};
void to_json(nlohmann::json& j, const ModbusDeviceMonitorData& m);

struct ModbusDeviceFormattedData : public ModbusDeviceStatus {
  std::string type;
  std::vector<std::string> register_list{};
};
void to_json(nlohmann::json& j, const ModbusDeviceFormattedData& m);

class ModbusDevice {
  Modbus& interface;
  uint8_t addr;
  const RegisterMap& register_map;
  std::mutex register_list_mutex{};
  ModbusDeviceMonitorData info{};

 public:
  ModbusDevice(Modbus& iface, uint8_t a, const RegisterMap& reg);

  void command(
      Msg& req,
      Msg& resp,
      modbus_time timeout = modbus_time::zero(),
      modbus_time settle_time = modbus_time::zero());

  void ReadHoldingRegisters(
      uint16_t register_offset,
      std::vector<uint16_t>& regs);

  void monitor();
  bool is_active() const {
    return info.get_mode() == ModbusDeviceMode::ACTIVE;
  }
  void set_active() {
    info.num_consecutive_failures = 0;
  }
  time_t last_active() const {
    return info.last_active;
  }
  // Simple func, returns a copy of the monitor
  // data.
  ModbusDeviceMonitorData get_monitor_data() {
    std::unique_lock lk(register_list_mutex);
    // Makes a deep copy.
    return info;
  }
  ModbusDeviceStatus get_status() {
    std::unique_lock lk(register_list_mutex);
    return info;
  }
  ModbusDeviceFormattedData get_formatted_data();
};
