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

WriteMultipleRegistersReq::WriteMultipleRegistersReq(uint8_t a, uint16_t off)
    : dev_addr(a), starting_addr(off) {
  // addr(1), function(1), reg_start(2), reg_count(2), bytes(1)
  addr = a;
  len = 7;
}

void WriteMultipleRegistersReq::encode() {
  if (len <= 7)
    throw std::underflow_error("No registers to write");
  uint8_t data_len = len - 7;
  // Pad if the result does not fit in whole 16bit regs.
  // XXX We probably need to throw here instead of padding.
  if ((data_len % 2) != 0) {
    *this << uint8_t(0);
    data_len += 1;
  }
  // Count the total registers.
  reg_count = data_len / 2;
  len = 0; // Clear so we can get to the header
  *this << dev_addr << function << starting_addr << reg_count << data_len;
  len += data_len;
  finalize();
}

WriteMultipleRegistersResp::WriteMultipleRegistersResp(
    uint8_t a,
    uint16_t off,
    uint16_t cnt)
    : expected_dev_addr(a),
      expected_starting_addr(off),
      expected_reg_count(cnt) {
  // addr(1), func(1), reg_off(2), reg_count(2), crc(2)
  len = 8;
}

void WriteMultipleRegistersResp::decode() {
  // addr(1), func(1), off(2), count(2), crc(2)
  validate();
  // Pop fields from behind
  *this >> reg_count >> starting_addr >> function >> dev_addr;
  check_value("dev_addr", dev_addr, expected_dev_addr);
  check_value("function", function, expected_function);
  check_value("starting_addr", starting_addr, expected_starting_addr);
  check_value("reg_count", reg_count, expected_reg_count);
}
