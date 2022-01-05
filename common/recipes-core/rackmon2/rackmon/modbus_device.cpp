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
  for (const auto& sp : reg.special_handlers) {
    ModbusSpecialHandler hdl{};
    hdl.SpecialHandlerInfo::operator=(sp);
    special_handlers.push_back(hdl);
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
  for (auto& h : special_handlers) {
    h.handle(*this);
  }
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

static std::string command_output(const std::string& shell) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(
      popen(shell.c_str(), "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

void ModbusSpecialHandler::handle(ModbusDevice& dev) {
  // Check if it is time to handle.
  if (!can_handle())
    return;
  std::string str_value{};
  WriteMultipleRegistersReq req(dev.addr, reg);
  if (info.shell) {
    str_value = command_output(info.shell.value());
  } else if (info.value) {
    str_value = info.value.value();
  } else {
    std::cerr << "NULL action ignored" << std::endl;
    return;
  }
  if (info.interpret == RegisterValueType::INTEGER) {
    int32_t ival = std::stoi(str_value);
    if (len == 1)
      req << uint16_t(ival);
    else if (len == 2)
      req << uint32_t(ival);
  } else if (info.interpret == RegisterValueType::STRING) {
    for (char c : str_value)
      req << uint8_t(c);
  }
  WriteMultipleRegistersResp resp(dev.addr, reg, len);
  try {
    dev.command(req, resp);
  } catch (std::exception& e) {
    log_error << "Error executing special handler" << std::endl;
  }
  last_handle_time = std::time(NULL);
  handled = true;
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
