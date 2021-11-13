#pragma once
#include <stdexcept>
#include <string>

struct timeout_exception : public std::runtime_error {
  timeout_exception() : std::runtime_error("Timeout") {}
};

class Device {
 protected:
  const std::string dev;
  int dev_fd = -1;

 public:
  explicit Device(const std::string& device) : dev(device) {}
  virtual ~Device() {
    close();
  }
  virtual void open();
  virtual void close();
  virtual void write(const uint8_t* buf, size_t len);
  virtual void ioctl(int32_t cmd, void* data);
  virtual void wait_read(int timeout_us);
  virtual void wait_write() {}
  virtual void read(uint8_t* buf, size_t exact_len, int timeout_us);
};
