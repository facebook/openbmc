// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <optional>
#include "Msg.hpp"

namespace rackmon {

struct BadResponseError : public std::runtime_error {
  BadResponseError(const std::string& field, uint32_t exp, uint32_t val)
      : std::runtime_error(
            "Bad Response Received FIELD:" + field + " Expected: " +
            std::to_string(exp) + " Got: " + std::to_string(val)) {}
};

//---------- Read Holding Registers -------

struct ReadHoldingRegistersReq : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x3;
  uint8_t deviceAddr_ = 0;
  uint16_t registerOffset_ = 0;
  uint16_t registerCount_ = 0;

 public:
  ReadHoldingRegistersReq(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t registerCount);
  ReadHoldingRegistersReq() {}

 protected:
  void encode() override;
};

struct ReadHoldingRegistersResp : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x3;
  uint8_t expectedDeviceAddr_ = 0;
  std::vector<uint16_t>& regs_;

 public:
  explicit ReadHoldingRegistersResp(
      uint8_t deviceAddr,
      std::vector<uint16_t>& regs);

 protected:
  void decode() override;
};

//----------- Write Single Register -------
struct WriteSingleRegisterReq : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x6;
  uint8_t deviceAddr_ = 0;
  uint16_t registerOffset_ = 0;
  uint16_t value_ = 0;

 public:
  WriteSingleRegisterReq(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t value);
  WriteSingleRegisterReq() {}

 protected:
  void encode() override;
};

struct WriteSingleRegisterResp : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x6;
  uint8_t expectedDeviceAddr_ = 0;
  uint16_t expectedRegisterOffset_ = 0;
  uint16_t value_ = 0;
  std::optional<uint16_t> expectedValue_{};

 public:
  WriteSingleRegisterResp(uint8_t deviceAddr, uint16_t registerOffset);
  WriteSingleRegisterResp(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t expectedValue);
  WriteSingleRegisterResp() {}
  uint16_t writtenValue() const {
    return value_;
  }

 protected:
  void decode() override;
};

//----------- Write Multiple Registers ------

// Users are expected to use the << operator to
// push in the register contents, helps them
// take advantage of any endian-conversions privided
// by msg.hpp
struct WriteMultipleRegistersReq : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x10;
  uint8_t deviceAddr_ = 0;
  uint16_t registerOffset_ = 0;

 public:
  WriteMultipleRegistersReq(uint8_t deviceAddr, uint16_t registerOffset);
  WriteMultipleRegistersReq() {}

 protected:
  void encode() override;
};

struct WriteMultipleRegistersResp : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x10;
  uint8_t expectedDeviceAddr_ = 0;
  uint16_t expectedRegisterOffset_ = 0;
  uint16_t expectedRegisterCount_ = 0;

 public:
  WriteMultipleRegistersResp(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t registerCount);
  WriteMultipleRegistersResp() {}

 protected:
  void decode() override;
};

//---------- Read File Record ----------------
struct FileRecord {
  uint16_t fileNum = 0;
  uint16_t recordNum = 0;
  std::vector<uint16_t> data{};
  FileRecord() {}
  explicit FileRecord(size_t num) : data(num) {}
  explicit FileRecord(uint16_t file, uint16_t record, size_t num)
      : fileNum(file), recordNum(record), data(num) {}
};

struct ReadFileRecordReq : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x14;
  static constexpr uint8_t kReferenceType = 0x6;
  uint8_t deviceAddr_ = 0;
  const std::vector<FileRecord>& records_;

 public:
  ReadFileRecordReq(uint8_t deviceAddr, const std::vector<FileRecord>& records);

 protected:
  void encode() override;
};

struct ReadFileRecordResp : public Msg {
 private:
  static constexpr uint8_t kExpectedFunction = 0x14;
  static constexpr uint8_t kReferenceType = 0x6;
  uint8_t deviceAddr_ = 0;
  std::vector<FileRecord>& records_;

 public:
  ReadFileRecordResp(uint8_t deviceAddr, std::vector<FileRecord>& records);

 protected:
  void decode() override;
};

} // namespace rackmon
