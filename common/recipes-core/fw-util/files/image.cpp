#include "image.hpp"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

Image::Image(const std::string& path) : path_(path) {
  // TODO fsize_ = std::filesystem::file_size(path);
  if (struct stat stat_buf; stat(path.c_str(), &stat_buf) == 0) {
    fsize_ = stat_buf.st_size;
  } else {
    throw std::runtime_error("Cannot get file size");
  }
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    throw std::runtime_error("Image open failed");
  }
  void* buffer = mmap(nullptr, fsize_, PROT_READ, MAP_PRIVATE, fd, 0);
  if (buffer == MAP_FAILED) {
    close(fd);
    throw std::runtime_error("map failed");
  }
  fd_ = fd;
  buffer_ = static_cast<char*>(buffer);
}

Image::~Image() {
  if (buffer_ != nullptr) {
    munmap(buffer_, fsize_);
    buffer_ = nullptr;
    fsize_ = 0;
  }
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}
