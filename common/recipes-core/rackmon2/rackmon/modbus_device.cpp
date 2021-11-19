#include "modbus_device.hpp"
#include <iomanip>
#include <sstream>

using nlohmann::json;

ModbusDevice::ModbusDevice(Modbus& iface, uint8_t a, const RegisterMap& reg)
    : interface(iface),
      addr(a),
      register_map(reg) {
  info.addr = a;
  info.baudrate = reg.default_baudrate;
  for (auto& it : reg.register_descriptors) {
    info.history.emplace_back(it.first, it.second.keep, it.second.length);
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
    std::cerr << e.what() << std::endl;
    throw;
  } catch (...) {
    std::cerr << "Unknown exception" << std::endl;
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

void ModbusDevice::monitor() {
  uint32_t timestamp = std::time(0);
  std::unique_lock lk(history_mutex);
  for (auto& h : info.history) {
    uint16_t reg = h.reg_addr;
    auto& v = h.history[h.idx];
    if (register_map.at(reg).changes_only) {
      std::vector<uint16_t> value(v.value.size());
      ReadHoldingRegisters(reg, value);
      int prev_idx = h.idx == 0 ? h.history.size() - 1 : h.idx - 1;
      if (h.history[prev_idx].timestamp == 0 ||
          h.history[prev_idx].value != value) {
        v.value = value;
        v.timestamp = timestamp;
        h.idx = (h.idx + 1) % h.history.size();
      }
    } else {
      ReadHoldingRegisters(reg, v.value);
      v.timestamp = timestamp;
      h.idx = (h.idx + 1) % h.history.size();
    }
  }
}

void to_json(json& j, const RegisterValue& m) {
  j["time"] = m.timestamp;
  std::stringstream ss;
  for (auto& d : m.value) {
    uint8_t l = d & 0xff, h = (d >> 8) & 0xff;
    ss << std::setfill('0') << std::setw(2) << std::right << std::hex << int(l);
    ss << std::setfill('0') << std::setw(2) << std::right << std::hex << int(h);
  }
  j["data"] = ss.str();
}

void to_json(json& j, const RegisterValueHistory& m) {
  j["begin"] = m.reg_addr;
  j["readings"] = m.history;
}

void to_json(json& j, const ModbusDeviceStatus& m) {
  j["addr"] = m.addr;
  j["crc_fails"] = m.crc_failures;
  j["timeouts"] = m.timeouts;
  j["misc_fails"] = m.misc_failures;
  j["last_active"] = m.last_active;
  j["consecutive_failures"] = m.num_consecutive_failures;
}

void to_json(json& j, const ModbusDeviceMonitorData& m) {
  const ModbusDeviceStatus& s = m;
  to_json(j, s);
  j["now"] = std::time(0);
  j["ranges"] = m.history;
}
