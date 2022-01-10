#include <nlohmann/json.hpp>
#include <ctime>
#include <iostream>
#include "modbus.hpp"
#include "modbus_cmds.hpp"
#include "regmap.hpp"

enum ModbusDeviceMode { ACTIVE = 0, DORMANT = 1 };

class ModbusDevice;

struct ModbusSpecialHandler : public SpecialHandlerInfo {
  time_t last_handle_time = 0;
  bool handled = false;
  bool can_handle() {
    if (period == -1)
      return !handled;
    return std::time(0) > (last_handle_time + period);
  }
  void handle(ModbusDevice& dev);
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
    return num_consecutive_failures < max_consecutive_failures
        ? ModbusDeviceMode::ACTIVE
        : ModbusDeviceMode::DORMANT;
  }
};
void to_json(nlohmann::json& j, const ModbusDeviceStatus& m);

struct ModbusDeviceRawData : public ModbusDeviceStatus {
  std::vector<RegisterStore> register_list{};
};
void to_json(nlohmann::json& j, const ModbusDeviceRawData& m);

struct ModbusDeviceFmtData : public ModbusDeviceStatus {
  std::string type;
  std::vector<std::string> register_list{};
};
void to_json(nlohmann::json& j, const ModbusDeviceFmtData& m);

struct ModbusDeviceValueData : public ModbusDeviceStatus {
  std::string type;
  std::vector<RegisterStoreValue> register_list{};
};
void to_json(nlohmann::json& j, const ModbusDeviceValueData& m);

class ModbusDevice {
  friend ModbusSpecialHandler;
  Modbus& interface;
  uint8_t addr;
  const RegisterMap& register_map;
  std::mutex register_list_mutex{};
  ModbusDeviceRawData info{};
  std::vector<ModbusSpecialHandler> special_handlers{};

 public:
  ModbusDevice(Modbus& iface, uint8_t a, const RegisterMap& reg);
  virtual ~ModbusDevice() {}

  virtual void command(
      Msg& req,
      Msg& resp,
      ModbusTime timeout = ModbusTime::zero(),
      ModbusTime settle_time = ModbusTime::zero());

  void ReadHoldingRegisters(
      uint16_t register_offset,
      std::vector<uint16_t>& regs);

  void WriteSingleRegister(uint16_t register_offset, uint16_t value);

  void WriteMultipleRegisters(
      uint16_t register_offset,
      std::vector<uint16_t>& value);

  void ReadFileRecord(std::vector<FileRecord>& records);

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
  ModbusDeviceRawData get_raw_data() {
    std::unique_lock lk(register_list_mutex);
    // Makes a deep copy.
    return info;
  }
  ModbusDeviceStatus get_status() {
    std::unique_lock lk(register_list_mutex);
    return info;
  }
  ModbusDeviceFmtData get_fmt_data();

  ModbusDeviceValueData get_value_data();
};
