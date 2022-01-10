#include "dev.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "log.hpp"

static std::error_code sys_error() {
  return std::error_code(errno, std::generic_category());
}

void Device::open() {
  if (deviceFd_ >= 0) {
    throw std::runtime_error("Device already opened");
  }
  deviceFd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY);
  if (deviceFd_ < 0) {
    throw std::system_error(sys_error(), "Open of " + device_ + " failed");
  }
}

void Device::close() {
  if (deviceFd_ >= 0) {
    ::close(deviceFd_);
    deviceFd_ = -1;
  }
}

void Device::write(const uint8_t* buf, size_t len) {
  int ret = ::write(deviceFd_, buf, len);
  if (ret != (int)len) {
    throw std::system_error(
        sys_error(),
        "Writing " + std::to_string(len) + " to " + device_ + "failed!");
  }
}

void Device::ioctl(int32_t cmd, void* data) {
  int ret = ::ioctl(deviceFd_, cmd, data);
  if (ret == -1) {
    throw std::system_error(
        sys_error(), "IOCTL " + std::to_string(cmd) + " failed!");
  }
}

void Device::waitRead(int timeoutMs) {
  fd_set fdset;
  struct timeval timeout;
  struct timeval* timeoutPtr = nullptr;
  FD_ZERO(&fdset);
  FD_SET(deviceFd_, &fdset);
  if (timeoutMs > 0) {
    timeout.tv_sec = 0;
    timeout.tv_usec = timeoutMs * 1000;
    timeoutPtr = &timeout;
  }
  int rc = select(deviceFd_ + 1, &fdset, NULL, NULL, timeoutPtr);
  if (rc == -1) {
    throw std::system_error(
        sys_error(), "select returned error for " + device_);
  }
  if (rc == 0) {
    throw TimeoutException();
  }
}

void Device::read(uint8_t* buf, size_t exactLen, int timeoutMs) {
  uint8_t readBuf[16];
  std::memset(buf, 0, exactLen);
  size_t pos = 0;
  size_t iter;
  // We are at minimum going with len/16 iterations.
  // Add some upper bounds on this to make sure we do
  // not loop here forever.
  size_t maxIter = (1 + exactLen / sizeof(readBuf)) * 4;
  for (pos = 0, iter = 0; pos < exactLen && iter < maxIter; iter++) {
    try {
      waitRead(timeoutMs);
    } catch (std::system_error& e) {
      // Print error and ignore/retry
      log_error << e.what() << std::endl;
    }
    int readSize = ::read(deviceFd_, readBuf, sizeof(readBuf));
    if (readSize < 0) {
      if (errno == EAGAIN)
        continue;
      throw std::system_error(sys_error(), "read response failure");
    }
    if ((pos + readSize) > exactLen) {
      throw std::overflow_error("Read overflowed excpected size");
    }
    std::memcpy(buf + pos, readBuf, readSize);
    pos += readSize;
  }
  if (pos != exactLen) {
    throw std::runtime_error(
        "Aborted read after iterations: " + std::to_string(iter));
  }
}
