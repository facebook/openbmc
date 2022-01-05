#include "modbus_cmds.hpp"
#include <algorithm>

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
  // Pop registers from behind.
  for (auto it = regs.rbegin(); it != regs.rend(); it++) {
    *this >> *it;
  }
  // Pop fields from behind
  *this >> byte_count >> function >> dev_addr;
  if (function != expected_function)
    throw bad_resp_error("function", 0x3, function);
  if (byte_count != (regs.size() * 2))
    throw bad_resp_error("byte_count", regs.size() * 2, byte_count);
}
