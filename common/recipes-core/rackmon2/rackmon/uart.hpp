#pragma once

#include <mutex>
#include <vector>
#include "dev.hpp"

class UARTDevice : public Device {
 protected:
  int baudrate_ = -1;

  virtual void waitWrite() override;
  virtual void setAttribute(bool readEnable, int baudrate);

  void readEnable() {
    setAttribute(true, baudrate_);
  }
  void readDisable() {
    setAttribute(false, baudrate_);
  }

 public:
  UARTDevice(const std::string& device, int baudrate)
      : Device(device), baudrate_(baudrate) {}

  int getBaudrate() const {
    return baudrate_;
  }
  void setBaudrate(int baudrate) {
    if (baudrate == baudrate_)
      return;
    baudrate_ = baudrate;
    setAttribute(true, baudrate);
  }

  virtual void open() override;
  virtual void write(const uint8_t* buf, size_t len) override;

  void write(const std::vector<uint8_t>& buf) {
    write(buf.data(), buf.size());
  }
};

class AspeedRS485Device : public UARTDevice {
 public:
  AspeedRS485Device(const std::string& device, int baudrate)
      : UARTDevice(device, baudrate) {}
  virtual void open() override;
};
