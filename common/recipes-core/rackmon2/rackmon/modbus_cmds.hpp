#pragma once

#include <optional>
#include "msg.hpp"

struct bad_resp_error : public std::runtime_error {
  bad_resp_error(const std::string& field, uint32_t exp, uint32_t val)
      : std::runtime_error(
            "Bad Response Received FIELD:" + field + " Expected: " +
            std::to_string(exp) + " Got: " + std::to_string(val)) {}
};

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

//----------- Write Single Register -------
struct WriteSingleRegisterReq : public Msg {
  static constexpr uint8_t expected_function = 0x6;
  uint8_t dev_addr = 0;
  uint8_t function = expected_function;
  uint16_t reg_off = 0;
  uint16_t value = 0;
  WriteSingleRegisterReq(uint8_t a, uint16_t off, uint16_t val);
  WriteSingleRegisterReq() {}
  void encode() override;
  using Msg::decode;
};

struct WriteSingleRegisterResp : public Msg {
  static constexpr uint8_t expected_function = 0x6;
  uint8_t dev_addr = 0;
  uint8_t function = 0;
  uint16_t reg_off = 0;
  uint8_t expected_dev_addr = 0;
  uint16_t expected_reg_off = 0;
  uint16_t value = 0;
  std::optional<uint16_t> expected_value{};
  WriteSingleRegisterResp(uint8_t a, uint16_t off);
  WriteSingleRegisterResp(uint8_t a, uint16_t off, uint16_t val);
  WriteSingleRegisterResp() {}
  void decode() override;
  using Msg::encode;
};

//----------- Write Multiple Registers ------

// Users are expected to use the << operator to
// push in the register contents, helps them
// take advantage of any endian-conversions privided
// by msg.hpp
struct WriteMultipleRegistersReq : public Msg {
  static constexpr uint8_t expected_function = 0x10;
  uint8_t dev_addr = 0;
  uint8_t function = expected_function;
  uint16_t starting_addr = 0;
  uint16_t reg_count = 0;
  WriteMultipleRegistersReq(uint8_t a, uint16_t off);
  WriteMultipleRegistersReq() {}
  void encode() override;
  using Msg::decode;
};

struct WriteMultipleRegistersResp : public Msg {
  static constexpr uint8_t expected_function = 0x10;
  uint8_t dev_addr = 0;
  uint8_t function = 0;
  uint16_t starting_addr = 0;
  uint16_t reg_count = 0;
  uint8_t expected_dev_addr = 0;
  uint16_t expected_starting_addr = 0;
  uint16_t expected_reg_count = 0;
  WriteMultipleRegistersResp(uint8_t a, uint16_t off, uint16_t cnt);
  WriteMultipleRegistersResp() {}
  void decode() override;
  using Msg::encode;
};
