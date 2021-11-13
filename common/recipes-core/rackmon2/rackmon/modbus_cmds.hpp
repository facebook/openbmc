#pragma once

#include "msg.hpp"

//---------- Read Holding Registers -------

struct ReadHoldingRegistersReq : public Msg {
  static constexpr uint8_t expected_function = 0x3;
  uint8_t dev_addr = 0;
  uint8_t function = expected_function;
  uint16_t starting_addr = 0;
  uint16_t reg_count = 0;

  ReadHoldingRegistersReq(uint8_t a, uint16_t reg_off, uint16_t cnt);
  ReadHoldingRegistersReq() {}
  void encode() override;
};

struct ReadHoldingRegistersResp : public Msg {
  static constexpr uint8_t expected_function = 0x3;
  uint8_t dev_addr = 0;
  uint8_t function = 0;
  uint8_t byte_count = 0;
  std::vector<uint16_t>& regs;
  explicit ReadHoldingRegistersResp(std::vector<uint16_t>& r);
  void decode() override;
};
