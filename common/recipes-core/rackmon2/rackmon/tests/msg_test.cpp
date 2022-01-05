#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "msg.hpp"

using namespace std;
using namespace testing;

TEST(Msg, SaneInits) {
  Msg msg;
  EXPECT_EQ(msg.len, 0);
  // This is defined in the protocol.
  // Do not let anyone change this!
  EXPECT_EQ(msg.raw.size(), 253);
  EXPECT_EQ(msg.begin(), msg.end());
}

TEST(Msg, TestMsgStream) {
  Msg msg;
  uint8_t v8 = 0xf0;
  uint16_t v16 = 0x1234;
  msg << v8;
  EXPECT_EQ(msg.len, 1);
  EXPECT_EQ(msg.raw[0], v8);
  EXPECT_EQ(msg.addr, v8);
  EXPECT_NE(msg.begin(), msg.end());
  EXPECT_EQ(*msg.begin(), v8);
  msg << v16;
  EXPECT_EQ(msg.len, 3);
  EXPECT_EQ(msg.raw[0], v8);
  EXPECT_EQ(msg.raw[1], 0x12);
  EXPECT_EQ(msg.raw[2], 0x34);
  EXPECT_EQ(msg.addr, v8);
  EXPECT_NE(msg.begin(), msg.end());
  EXPECT_EQ(*(msg.begin() + 0), 0xf0);
  EXPECT_EQ(*(msg.begin() + 1), 0x12);
  EXPECT_EQ(*(msg.begin() + 2), 0x34);
  uint16_t o16;
  msg >> o16;
  EXPECT_EQ(o16, v16);
  EXPECT_EQ(msg.len, 1);
  uint8_t o8;
  msg >> o8;
  EXPECT_EQ(o8, v8);
  EXPECT_EQ(msg.len, 0);
  msg << v8;
  msg.clear();
  EXPECT_EQ(msg.len, 0);
}

TEST(Msg, TestUnderflow) {
  Msg msg;
  uint8_t v8 = 0x1;
  uint16_t v16;
  // Underflow trying to get anything out of an empty msg
  EXPECT_THROW(msg >> v8, std::underflow_error);
  // Undeflow when trying to get 2 bytes on something with 1.
  msg << v8++;
  EXPECT_THROW(msg >> v16, std::underflow_error);
  msg << v8;
  msg >> v16;
  EXPECT_EQ(v16, 0x0102);
}

TEST(Msg, TestOverflow) {
  Msg msg;
  // Almost fill it out with just 1 byte to spare.
  // all this should work.
  for (size_t i = 0; i < 252; i++)
    msg << uint8_t(0xff);
  // Try to push a 16bit value, it should throw but
  // not change the state of msg.
  EXPECT_THROW(msg << uint16_t(0xffff), std::overflow_error);
  // push the last byte, should succeed.
  msg << uint8_t(0xff);
  // Anymore pushes should fail.
  EXPECT_THROW(msg << uint8_t(0xff), std::overflow_error);
}

class TestMsg : public Msg {
 public:
  void wrap_finalize() {
    finalize();
  }
  void wrap_encode() {
    encode();
  }
  void wrap_validate() {
    validate();
  }
  void wrap_decode() {
    decode();
  }
};

TEST(Msg, TestCRC) {
  TestMsg msg;
  msg << uint8_t(1);
  msg << uint8_t(2);
  msg << uint8_t(3);
  EXPECT_EQ(msg.len, 3);
  msg.wrap_finalize();
  EXPECT_EQ(msg.len, 5);
  // Pop CRC and inspect.
  uint16_t crc;
  msg >> crc;
  ASSERT_EQ(msg.len, 3);
  ASSERT_EQ(crc, 0x6161);
  // check validate does not throw
  msg.wrap_encode();
  msg.wrap_validate();
  ASSERT_EQ(msg.len, 3);
  // Reset len to include CRC and check decode() does not throw.
  msg.len += 2;
  msg.wrap_decode();
  ASSERT_EQ(msg.len, 3);

  // Reset len and corrupt one byte, check both validate and
  // decode throws.
  msg.len += 2;
  msg.raw[0] = 4;
  EXPECT_THROW(msg.wrap_validate(), crc_exception);
  EXPECT_THROW(msg.wrap_decode(), crc_exception);
}

TEST(Msg, ConstantLiteral) {
  Msg msg = 0x123456789abcdef012_M;
  ASSERT_EQ(msg.len, 9);
  ASSERT_EQ(msg.raw[0], 0x12);
  ASSERT_EQ(msg.raw[1], 0x34);
  ASSERT_EQ(msg.raw[2], 0x56);
  ASSERT_EQ(msg.raw[3], 0x78);
  ASSERT_EQ(msg.raw[4], 0x9a);
  ASSERT_EQ(msg.raw[5], 0xbc);
  ASSERT_EQ(msg.raw[6], 0xde);
  ASSERT_EQ(msg.raw[7], 0xf0);
  ASSERT_EQ(msg.raw[8], 0x12);
}

TEST(Msg, constIter) {
  const Msg msg = 0x1234_M;
  auto it = msg.begin();
  ASSERT_EQ(*it, 0x12);
  it++;
  ASSERT_EQ(*it, 0x34);
  it++;
  ASSERT_EQ(it, msg.end());
}

TEST(Msg, EqualCompare) {
  Msg msg1 = 0x12345678_M;
  Msg msg2 = 0x123456_M;
  ASSERT_NE(msg1, msg2);
  msg2 << uint8_t(0x78);
  ASSERT_EQ(msg1, msg2);
}

TEST(Msg, CopyConstructor) {
  Msg msg1 = 0x12345678_M;
  Msg msg2(msg1);
  ASSERT_EQ(msg1, msg2);
}

TEST(Msg, AssignmentOperator) {
  Msg msg1 = 0x12345678_M;
  Msg msg2 = 0x1234_M;
  ASSERT_NE(msg1, msg2);
  msg2 = msg1;
  ASSERT_EQ(msg1, msg2);
}
