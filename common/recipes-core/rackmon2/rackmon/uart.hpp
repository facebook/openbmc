#pragma once

#include <mutex>
#include <vector>
#include "dev.hpp"

class UARTDevice : public Device {
 protected:
  int baudrate = -1;

  void wait_write() override;
  void set_attribute(bool read_en);

  void read_enable() {
    set_attribute(true);
  }
  void read_disable() {
    set_attribute(false);
  }

 public:
  UARTDevice(const std::string& dev, int baud) : Device(dev), baudrate(baud) {}
  virtual ~UARTDevice();

  int get_baudrate() const {
    return baudrate;
  }
  void set_baudrate(int baud) {
    if (baud == baudrate)
      return;
    baudrate = baud;
    set_attribute(true);
  }

  virtual void open() override;
  virtual void write(const uint8_t* buf, size_t len) override;

  void write(const std::vector<uint8_t>& buf) {
    write(buf.data(), buf.size());
  }
};

class AspeedRS485Device : public UARTDevice {
 public:
  AspeedRS485Device(const std::string& dev, int baud) : UARTDevice(dev, baud) {}
  virtual void open() override;
};
