#include "dev.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

static std::error_code sys_error() {
  return std::error_code(errno, std::generic_category());
}

void Device::open() {
  if (dev_fd >= 0) {
    throw std::runtime_error("Device already opened");
  }
  dev_fd = ::open(dev.c_str(), O_RDWR | O_NOCTTY);
  if (dev_fd < 0) {
    throw std::system_error(sys_error(), "Open of " + dev + " failed");
  }
}

void Device::close() {
  if (dev_fd >= 0) {
    ::close(dev_fd);
    dev_fd = -1;
  }
}

void Device::write(const uint8_t* buf, size_t len) {
  int ret = ::write(dev_fd, buf, len);
  if (ret != (int)len) {
    throw std::system_error(
        sys_error(),
        "Writing " + std::to_string(len) + " to " + dev + "failed!");
  }
}

void Device::ioctl(int32_t cmd, void* data) {
  int ret = ::ioctl(dev_fd, cmd, data);
  if (ret == -1) {
    throw std::system_error(
        sys_error(), "IOCTL " + std::to_string(cmd) + " failed!");
  }
}

void Device::wait_read(int timeout_ms) {
  fd_set fdset;
  struct timeval timeout;
  struct timeval *timeout_ptr = nullptr;
  FD_ZERO(&fdset);
  FD_SET(dev_fd, &fdset);
  if (timeout_ms > 0) {
    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;
    timeout_ptr = &timeout;
  }
  int rc = select(dev_fd + 1, &fdset, NULL, NULL, timeout_ptr);
  if (rc == -1) {
    throw std::system_error(sys_error(), "select returned error for " + dev);
  }
  if (rc == 0) {
    throw timeout_exception();
  }
}

void Device::read(uint8_t* buf, size_t exact_len, int timeout_ms) {
  uint8_t read_buf[16];
  std::memset(buf, 0, exact_len);
  size_t pos = 0;
  size_t iter;
  // We are at minimum going with len/16 iterations.
  // Add some upper bounds on this to make sure we do
  // not loop here forever.
  size_t max_iter = (1 + exact_len / sizeof(read_buf)) * 4;
  for (pos = 0, iter = 0; pos < exact_len && iter < max_iter; iter++) {
    try {
      wait_read(timeout_ms);
    } catch (std::system_error& e) {
      // Print error and ignore/retry
      std::cerr << e.what() << std::endl;
    }
    int read_size = ::read(dev_fd, read_buf, sizeof(read_buf));
    if (read_size < 0) {
      if (errno == EAGAIN)
        continue;
      throw std::system_error(sys_error(), "read response failure");
    }
    if ((pos + read_size) > exact_len) {
      throw std::overflow_error("Read overflowed excpected size");
    }
    std::memcpy(buf + pos, read_buf, read_size);
    pos += read_size;
  }
  if (pos != exact_len) {
    throw std::runtime_error(
        "Aborted read after iterations: " + std::to_string(iter));
  }
}
