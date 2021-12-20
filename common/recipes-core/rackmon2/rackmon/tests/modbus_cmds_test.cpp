#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "modbus_cmds.hpp"

using namespace std;
using namespace testing;

TEST(ReadHoldingRegisters, Req) {
  ReadHoldingRegistersReq msg(0x1, 0x1234, 0x20);
  msg.encode();
  EXPECT_EQ(msg.len, 8);
  EXPECT_EQ(msg.addr, 0x1);
  EXPECT_EQ(msg.raw[0], 0x1);
  EXPECT_EQ(msg.raw[1], 0x3);
  EXPECT_EQ(msg.raw[2], 0x12);
  EXPECT_EQ(msg.raw[3], 0x34);
  EXPECT_EQ(msg.raw[4], 0x0);
  EXPECT_EQ(msg.raw[5], 0x20);
  uint16_t crc;
  msg >> crc;
  EXPECT_EQ(crc, 0xa4);
}

TEST(ReadHoldingRegisters, GoodResp) {
  std::vector<uint16_t> vec(3);
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  std::vector<uint8_t> raw{0xa, 0x3, 0x6, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x59, 0x28};
  std::copy(raw.begin(), raw.end(), msg.raw.begin());
  msg.decode();
  EXPECT_EQ(msg.dev_addr, 0xa);
  EXPECT_EQ(msg.addr, 0xa);
  EXPECT_EQ(vec[0], 0x1122);
  EXPECT_EQ(vec[1], 0x3344);
  EXPECT_EQ(vec[2], 0x5566);
}

TEST(ReadHoldingRegisters, BadCRCResp) {
  std::vector<uint16_t> vec(3);
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  std::vector<uint8_t> raw{0xa, 0x3, 0x6, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x59, 0x29};
  std::copy(raw.begin(), raw.end(), msg.raw.begin());
  EXPECT_THROW(msg.decode(), crc_exception);
}

TEST(ReadHoldingRegisters, BadFuncResp) {
  std::vector<uint16_t> vec(3);
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  // Good CRC, bad Function
  std::vector<uint8_t> raw{0xa, 0x4, 0x6, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x18, 0xce};
  std::copy(raw.begin(), raw.end(), msg.raw.begin());
  EXPECT_THROW(msg.decode(), bad_resp_error);
}

TEST(ReadHoldingRegisters, BadBytesResp) {
  std::vector<uint16_t> vec(3);
  ReadHoldingRegistersResp msg(vec);
  EXPECT_EQ(msg.len, 11);
  // Good CRC, bad byte count
  std::vector<uint8_t> raw{0xa, 0x3, 0x4, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x7a, 0xe8};
  std::copy(raw.begin(), raw.end(), msg.raw.begin());
  EXPECT_THROW(msg.decode(), bad_resp_error);
}
