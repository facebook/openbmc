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
