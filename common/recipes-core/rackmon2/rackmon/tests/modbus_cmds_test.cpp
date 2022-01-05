#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "modbus_cmds.hpp"

using namespace std;
using namespace testing;

TEST(ReadHoldingRegisters, Req) {
  ReadHoldingRegistersReq msg(0x1, 0x1234, 0x20);
  msg.encode();
  // addr(1) = 0x01,
  // func(1) = 0x03,
  // reg_off(2) = 0x1234,
  // reg_cnt(2) = 0x0020,
  // crc(2) (Pre-computed)
  EXPECT_EQ(msg, 0x01031234002000a4_M);
  EXPECT_EQ(msg.addr, 0x1);
  uint16_t crc;
  msg >> crc;
  EXPECT_EQ(crc, 0xa4);
}

TEST(ReadHoldingRegisters, GoodResp) {
  std::vector<uint16_t> vec(3);
  std::vector<uint16_t> exp{0x1122, 0x3344, 0x5566};
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  // addr(1) = 0x0a,
  // func(1) = 0x03,
  // bytes(1) = 0x06,
  // regs(3*2) = 0x1122, 0x3344, 0x5566
  // crc(2) = 0x5928 (pre-computed)
  msg.Msg::operator=(0x0a03061122334455665928_M);
  msg.decode();
  EXPECT_EQ(msg.dev_addr, 0xa);
  EXPECT_EQ(msg.addr, 0xa);
  EXPECT_EQ(vec, exp);
}

TEST(ReadHoldingRegisters, BadCRCResp) {
  std::vector<uint16_t> vec(3);
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  // addr(1) = 0x0a,
  // func(1) = 0x03,
  // bytes(1) = 0x06,
  // regs(3*2) = 0x1122, 0x3344, 0x5566
  // crc(2) = 0x5929 (should be 0x5928)
  msg.Msg::operator=(0x0a03061122334455665929_M);
  EXPECT_THROW(msg.decode(), crc_exception);
}

TEST(ReadHoldingRegisters, BadFuncResp) {
  std::vector<uint16_t> vec(3);
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  // Good CRC, bad Function
  // addr(1) = 0x0a,
  // func(1) = 0x04 (should be 0x03),
  // bytes(1) = 0x06,
  // regs(3*2) = 0x1122, 0x3344, 0x5566
  // crc(2) = 0x18ce
  msg.Msg::operator=(0x0a040611223344556618ce_M);
  EXPECT_THROW(msg.decode(), bad_resp_error);
}

TEST(ReadHoldingRegisters, BadBytesResp) {
  std::vector<uint16_t> vec(3);
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  // Good CRC, bad byte count
  // addr(1) = 0x0a,
  // func(1) = 0x03,
  // bytes(1) = 0x04 (should be 6),
  // regs(3*2) = 0x1122, 0x3344, 0x5566
  // crc(2) = 0x7ae8
  msg.Msg::operator=(0x0a03041122334455667ae8_M);
  EXPECT_THROW(msg.decode(), bad_resp_error);
}

TEST(WriteSingleRegister, Req) {
  WriteSingleRegisterReq msg(0x1, 0x1234, 0x5678);
  msg.encode();
  // addr(1) 0x01
  // func(1) 0x06
  // reg_off(2) 0x1234
  // val(2) 0x5678
  EXPECT_EQ(msg, 0x010612345678_EM);
  // we have already tested CRC, just ensure decode
  // does not throw.
  msg.decode();
}

TEST(WriteSingleRegister, Resp) {
  WriteSingleRegisterResp msg(0x1, 0x1234);
  EXPECT_EQ(msg.len, 8);
  // addr(1) 0x01
  // func(1) 0x06
  // reg_off(2) 0x1234
  // val(2) 0x5678
  msg.Msg::operator=(0x010612345678_EM);
  msg.decode();
  EXPECT_EQ(msg.value, 0x5678);
}

TEST(WriteSingleRegister, RespSelfTest) {
  WriteSingleRegisterResp msg(0x1, 0x1234, 0x5678);
  EXPECT_EQ(msg.len, 8);
  // addr(1) 0x01
  // func(1) 0x06
  // reg_off(2) 0x1234
  // val(2) 0x5678
  msg.Msg::operator=(0x010612345678_EM);
  msg.decode();
}

TEST(WriteSingleRegister, RespSelfTestFail) {
  WriteSingleRegisterResp msg(0x1, 0x1234, 0x5678);
  EXPECT_EQ(msg.len, 8);
  // addr(1) 0x01
  // func(1) 0x06
  // reg_off(2) 0x1234
  // val(2) 0x5678
  msg.Msg::operator=(0x010612345679_EM);
  EXPECT_THROW(msg.decode(), bad_resp_error);
}
