#pragma once
#include <array>
#include <exception>
#include <memory>
#include <vector>

// Forward declaration needed to just mark it as a friend.
class Modbus;

struct crc_exception : public std::runtime_error {
  crc_exception() : std::runtime_error("CRC Exception") {}
};

// Modbus defines a max size of 253
const size_t max_modbus_length = 253;

struct Msg {
  std::array<uint8_t, max_modbus_length> raw;
  uint8_t& addr = raw[0];
  size_t len = 0;
  size_t off = 0; // Next to be read

  virtual ~Msg() {}

  void clear() {
    len = off = 0;
  }
  // Push to the end
  Msg& operator<<(uint8_t d);
  Msg& operator<<(uint16_t d);

  // Pop from the end.
  Msg& operator>>(uint8_t& d);
  Msg& operator>>(uint16_t& d);

 protected:
  void finalize();
  void validate();

 private:
  // Truely private. Used by finalize() and validate().
  uint16_t crc16();

  // Modbus will call encode before transmitting and
  // decode right after receiving a response. Marking
  // this private and having Modbus as a friend to
  // avoid someone accidentally calling encode() or
  // decode() from the upper layers which can cause
  // checksums etc to be recomputed thus destroying
  // the message.
  virtual void encode() {
    finalize();
  }
  virtual void decode() {
    validate();
  }
  friend Modbus;
};
