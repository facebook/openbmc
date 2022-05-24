#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <math.h>
#include "utils.hpp"

using namespace std;
using namespace testing;

class UtilTest : public ::testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtilTest, Encode_One_Padding_Characters_1) {
  vector<uint8_t> testBytes = {97, 110, 121, 32,  99, 97,  114, 110, 97,  108,
                               32, 112, 108, 101, 97, 115, 117, 114, 101, 46};

  std::string answer = "YW55IGNhcm5hbCBwbGVhc3VyZS4=";
  std::string encoded = encodeBase64(testBytes);

  EXPECT_EQ(encoded, answer);
}

TEST_F(UtilTest, Encode_Two_Padding_Characters_1) {
  vector<uint8_t> testBytes = {
      97,
      110,
      121,
      32,
      99,
      97,
      114,
      110,
      97,
      108,
      32,
      112,
      108,
      101,
      97,
      115,
      117,
      114,
      101};
  std::string answer = "YW55IGNhcm5hbCBwbGVhc3VyZQ==";

  std::string encoded = encodeBase64(testBytes);
  EXPECT_EQ(encoded, answer);
}

TEST_F(UtilTest, Encode_Zero_Padding_Characters) {
  vector<uint8_t> testBytes = {
      97,
      110,
      121,
      32,
      99,
      97,
      114,
      110,
      97,
      108,
      32,
      112,
      108,
      101,
      97,
      115,
      117,
      114};
  std::string answer = "YW55IGNhcm5hbCBwbGVhc3Vy";

  std::string encoded = encodeBase64(testBytes);
  EXPECT_EQ(encoded, answer);
}

TEST_F(UtilTest, Encode_One_Padding_Characters_2) {
  vector<uint8_t> testBytes = {
      97,
      110,
      121,
      32,
      99,
      97,
      114,
      110,
      97,
      108,
      32,
      112,
      108,
      101,
      97,
      115,
      117};
  std::string answer = "YW55IGNhcm5hbCBwbGVhc3U=";

  std::string encoded = encodeBase64(testBytes);
  EXPECT_EQ(encoded, answer);
}

TEST_F(UtilTest, Encode_Two_Padding_Characters_2) {
  vector<uint8_t> testBytes = {
      97, 110, 121, 32, 99, 97, 114, 110, 97, 108, 32, 112, 108, 101, 97, 115};
  std::string answer = "YW55IGNhcm5hbCBwbGVhcw==";

  std::string encoded = encodeBase64(testBytes);
  EXPECT_EQ(encoded, answer);
}

TEST_F(UtilTest, Encode_Plus) {
  vector<uint8_t> testBytes = {126, 127, 128};
  std::string answer = "fn+A";

  std::string encoded = encodeBase64(testBytes);
  EXPECT_EQ(encoded, answer);
}

TEST_F(UtilTest, Large_Encoding_Test) {
  vector<uint8_t> testBytes;
  std::string answer =
      "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUm"
      "JygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xN"
      "Tk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0"
      "dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqb"
      "nJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHC"
      "w8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp"
      "6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==";

  for (int number = 0; number < 255; number += 3) {
    testBytes.push_back(number);
    testBytes.push_back(number + 1);
    testBytes.push_back(number + 2);

    std::string encoded = encodeBase64(testBytes);
    int numChars = ((int)testBytes.size() / 3) * 4;
    std::string subAnswer = answer.substr(0, numChars);

    EXPECT_EQ(encoded, subAnswer);
  }
}

TEST_F(UtilTest, Decode_One_Padding_Characters_1) {
  vector<uint8_t> answer = {97, 110, 121, 32,  99, 97,  114, 110, 97,  108,
                            32, 112, 108, 101, 97, 115, 117, 114, 101, 46};

  std::string encoded = "YW55IGNhcm5hbCBwbGVhc3VyZS4=";
  vector<uint8_t> decoded = decodeBase64(encoded);

  EXPECT_EQ(decoded, answer);
}

TEST_F(UtilTest, Decode_Two_Padding_Characters_1) {
  vector<uint8_t> answer = {
      97,
      110,
      121,
      32,
      99,
      97,
      114,
      110,
      97,
      108,
      32,
      112,
      108,
      101,
      97,
      115,
      117,
      114,
      101};
  std::string encoded = "YW55IGNhcm5hbCBwbGVhc3VyZQ==";

  vector<uint8_t> decoded = decodeBase64(encoded);
  EXPECT_EQ(decoded, answer);
}

TEST_F(UtilTest, Decode_Zero_Padding_Characters) {
  vector<uint8_t> answer = {
      97,
      110,
      121,
      32,
      99,
      97,
      114,
      110,
      97,
      108,
      32,
      112,
      108,
      101,
      97,
      115,
      117,
      114};
  std::string encoded = "YW55IGNhcm5hbCBwbGVhc3Vy";

  vector<uint8_t> decoded = decodeBase64(encoded);
  EXPECT_EQ(decoded, answer);
}

TEST_F(UtilTest, Decode_One_Padding_Characters_2) {
  vector<uint8_t> answer = {
      97,
      110,
      121,
      32,
      99,
      97,
      114,
      110,
      97,
      108,
      32,
      112,
      108,
      101,
      97,
      115,
      117};
  std::string encoded = "YW55IGNhcm5hbCBwbGVhc3U=";

  vector<uint8_t> decoded = decodeBase64(encoded);
  EXPECT_EQ(decoded, answer);
}

TEST_F(UtilTest, Decode_Two_Padding_Characters_2) {
  vector<uint8_t> answer = {
      97, 110, 121, 32, 99, 97, 114, 110, 97, 108, 32, 112, 108, 101, 97, 115};
  std::string encoded = "YW55IGNhcm5hbCBwbGVhcw==";

  vector<uint8_t> decoded = decodeBase64(encoded);
  EXPECT_EQ(decoded, answer);
}

TEST_F(UtilTest, Decode_Plus_Char) {
  vector<uint8_t> answer = {126, 127, 128};
  std::string encoded = "fn+A";

  vector<uint8_t> decoded = decodeBase64(encoded);
  EXPECT_EQ(decoded, answer);
}

TEST_F(UtilTest, Encode_Decode) {
  vector<uint8_t> answer;
  vector<uint8_t> decoded;
  std::string encoded;

  for (size_t index = 0; index < 500; ++index)
    answer.push_back(index);

  encoded = encodeBase64(answer);
  decoded = decodeBase64(encoded);

  EXPECT_EQ(decoded, answer);
}

