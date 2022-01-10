#pragma once
#include <stdexcept>
#include <string>

struct TimeoutException : public std::runtime_error {
  TimeoutException() : std::runtime_error("Timeout") {}
};

class Device {
 protected:
  const std::string device_;
  int deviceFd_ = -1;

 public:
  explicit Device(const std::string& device) : device_(device) {}
  virtual ~Device() {
    close();
  }
  virtual void open();
  virtual void close();
  virtual void write(const uint8_t* buf, size_t len);
  virtual void ioctl(int32_t cmd, void* data);
  virtual void waitRead(int timeoutMs);
  virtual void waitWrite() {}
  virtual void read(uint8_t* buf, size_t exactLen, int timeoutMs);
};
