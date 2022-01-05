#include "modbus_cmds.hpp"
#include <algorithm>

static void
check_value(const std::string& what, uint32_t value, uint32_t expected_value) {
  if (value != expected_value)
    throw bad_resp_error(what, expected_value, value);
}

ReadHoldingRegistersReq::ReadHoldingRegistersReq(
    uint8_t a,
    uint16_t reg_off,
    uint16_t cnt)
    : dev_addr(a), starting_addr(reg_off), reg_count(cnt) {
  addr = a;
}

void ReadHoldingRegistersReq::encode() {
  *this << dev_addr << function << starting_addr << reg_count;
  finalize();
}

ReadHoldingRegistersResp::ReadHoldingRegistersResp(std::vector<uint16_t>& r)
    : regs(r) {
  if (!r.size())
    throw std::underflow_error("Response too small");
  // addr(1), func(1), bytecount(1), <2 * count regs>, crc(2)
  len = 5 + (2 * r.size());
}

void ReadHoldingRegistersResp::decode() {
  // addr(1), func(1), count(1), <2 * count regs>, crc(2)
  validate();
  *this >> regs >> byte_count >> function >> dev_addr;
  check_value("function", function, expected_function);
  check_value("byte_count", byte_count, regs.size() * 2);
}

WriteSingleRegisterReq::WriteSingleRegisterReq(
    uint8_t a,
    uint16_t off,
    uint16_t val)
    : dev_addr(a), reg_off(off), value(val) {
  addr = a;
}

void WriteSingleRegisterReq::encode() {
  *this << dev_addr << function << reg_off << value;
  finalize();
}

WriteSingleRegisterResp::WriteSingleRegisterResp(uint8_t a, uint16_t off)
    : dev_addr(a), expected_reg_off(off) {
  // addr(1), func(1), reg(2), value(2), crc(2)
  len = 8;
}

WriteSingleRegisterResp::WriteSingleRegisterResp(
    uint8_t a,
    uint16_t off,
    uint16_t val)
    : dev_addr(a), expected_reg_off(off), expected_value(val) {
  // addr(1), func(1), reg(2), value(2), crc(2)
  len = 8;
}

void WriteSingleRegisterResp::decode() {
  validate();
  *this >> value >> reg_off >> function >> dev_addr;
  check_value("reg_off", reg_off, expected_reg_off);
  if (expected_value)
    check_value("value", value, expected_value.value());
}
