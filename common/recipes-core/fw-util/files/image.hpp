#pragma once
#include <stdexcept>
#include <string>

class Image {
  size_t fsize_ = 0;
  char* buffer_ = nullptr;
  int fd_ = -1;
  std::string path_;

 public:
  explicit Image(const std::string& path);
  ~Image();
  size_t size() const {
    return fsize_;
  }
  char* peekExact(size_t offset, const size_t size) const {
    if ((offset + size) > fsize_) {
      throw std::runtime_error("Image too short");
    }
    return buffer_ + offset;
  }
  char* peek(size_t offset, size_t& size) const {
    if ((offset + size) > fsize_) {
      size = fsize_ - offset;
    }
    return buffer_ + offset;
  }
};
