#include "modbus_device.hpp"
#include <iomanip>
#include <sstream>
#include "log.hpp"

using nlohmann::json;

ModbusDevice::ModbusDevice(Modbus& iface, uint8_t a, const RegisterMap& reg)
    : interface(iface), addr(a), register_map(reg) {
  info.addr = a;
  info.baudrate = reg.default_baudrate;
  for (auto& it : reg.register_descriptors) {
    info.register_list.emplace_back(it.second);
  }
}

void ModbusDevice::command(
    Msg& req,
    Msg& resp,
    modbus_time timeout,
    modbus_time settle_time) {
  try {
    interface.command(req, resp, info.baudrate, timeout, settle_time);
    info.num_consecutive_failures = 0;
    info.last_active = std::time(0);
  } catch (timeout_exception& e) {
    info.timeouts++;
    info.num_consecutive_failures++;
    throw;
  } catch (crc_exception& e) {
    info.crc_failures++;
    info.num_consecutive_failures++;
    throw;
  } catch (std::runtime_error& e) {
    info.misc_failures++;
    info.num_consecutive_failures++;
    log_error << e.what() << std::endl;
    throw;
  } catch (...) {
    log_error << "Unknown exception" << std::endl;
    info.misc_failures++;
    info.num_consecutive_failures++;
    throw;
  }
}

void ModbusDevice::ReadHoldingRegisters(
    uint16_t register_offset,
    std::vector<uint16_t>& regs) {
  ReadHoldingRegistersReq req(addr, register_offset, regs.size());
  ReadHoldingRegistersResp resp(regs);
  command(req, resp);
}

void ModbusDevice::WriteSingleRegister(
    uint16_t register_offset,
    uint16_t value) {
  WriteSingleRegisterReq req(addr, register_offset, value);
  WriteSingleRegisterResp resp(addr, register_offset);
  command(req, resp);
}

void ModbusDevice::WriteMultipleRegisters(
    uint16_t register_offset,
    std::vector<uint16_t>& value) {
  WriteMultipleRegistersReq req(addr, register_offset);
  for (uint16_t val : value)
    req << val;
  WriteMultipleRegistersResp resp(addr, register_offset, value.size());
  command(req, resp);
}

void ModbusDevice::ReadFileRecord(std::vector<FileRecord>& records) {
  ReadFileRecordReq req(addr, records);
  ReadFileRecordResp resp(addr, records);
  command(req, resp);
}

void ModbusDevice::monitor() {
  uint32_t timestamp = std::time(0);
  std::unique_lock lk(register_list_mutex);
  for (auto& h : info.register_list) {
    uint16_t reg = h.reg_addr;
    auto& v = h.front();
    try {
      ReadHoldingRegisters(reg, v.value);
      v.timestamp = timestamp;
      // If we dont care about changes or if we do
      // and we notice that the value is different
      // from the previous, increment store to
      // point to the next.
      if (!v.desc.changes_only || v != h.back()) {
        ++h;
      }
    } catch (std::exception& e) {
      log_info << "DEV:0x" << std::hex << int(addr) << " ReadReg 0x" << std::hex
               << reg << ' ' << h.desc.name << " caught: " << e.what()
               << std::endl;
      continue;
    }
  }
}

ModbusDeviceFmtData ModbusDevice::get_fmt_data() {
  std::unique_lock lk(register_list_mutex);
  ModbusDeviceFmtData data;
  data.ModbusDeviceStatus::operator=(info);
  data.type = register_map.name;
  for (const auto& reg : info.register_list) {
    std::string str = reg;
    data.register_list.emplace_back(std::move(str));
  }
  return data;
}

ModbusDeviceValueData ModbusDevice::get_value_data() {
  std::unique_lock lk(register_list_mutex);
  ModbusDeviceValueData data;
  data.ModbusDeviceStatus::operator=(info);
  data.type = register_map.name;
  for (const auto& reg : info.register_list) {
    data.register_list.emplace_back(reg);
  }
  return data;
}

NLOHMANN_JSON_SERIALIZE_ENUM(
    ModbusDeviceMode,
    {{ModbusDeviceMode::ACTIVE, "active"},
     {ModbusDeviceMode::DORMANT, "dormant"}})

void to_json(json& j, const ModbusDeviceStatus& m) {
  j["addr"] = m.addr;
  j["crc_fails"] = m.crc_failures;
  j["timeouts"] = m.timeouts;
  j["misc_fails"] = m.misc_failures;
  j["mode"] = m.get_mode();
  j["baudrate"] = m.baudrate;
}

void to_json(json& j, const ModbusDeviceRawData& m) {
  const ModbusDeviceStatus& s = m;
  to_json(j, s);
  j["now"] = std::time(0);
  j["ranges"] = m.register_list;
}

void to_json(json& j, const ModbusDeviceFmtData& m) {
  const ModbusDeviceStatus& s = m;
  to_json(j, s);
  j["type"] = m.type;
  j["now"] = std::time(0);
  j["ranges"] = m.register_list;
}

void to_json(json& j, const ModbusDeviceValueData& m) {
  const ModbusDeviceStatus& s = m;
  to_json(j, s);
  j["type"] = m.type;
  j["now"] = std::time(0);
  j["ranges"] = m.register_list;
}
