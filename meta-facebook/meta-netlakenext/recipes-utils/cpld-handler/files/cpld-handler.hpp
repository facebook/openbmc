#pragma once

#include <fcntl.h>
#include <climits>
#include <algorithm>
#include <unistd.h>
#include <array>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Simple span-like template for C++17 compatibility
template<typename T>
class span {
public:
    span() : data_(nullptr), size_(0) {}
    span(T* data, size_t size) : data_(data), size_(size) {}
    
    // Constructor for std::vector
    template<typename U>
    span(std::vector<U>& v) : data_(reinterpret_cast<T*>(v.data())), size_(v.size()) {}
    template<typename U>
    span(const std::vector<U>& v) : data_(reinterpret_cast<T*>(const_cast<U*>(v.data()))), size_(v.size()) {}
    
    // Constructor for std::array
    template<typename U, size_t N>
    span(std::array<U, N>& arr) : data_(reinterpret_cast<T*>(arr.data())), size_(N) {}
    template<typename U, size_t N>
    span(const std::array<U, N>& arr) : data_(reinterpret_cast<T*>(const_cast<U*>(arr.data()))), size_(N) {}
    
    // Conversion from non-const to const
    template<typename U>
    span(const span<U>& other) : data_(other.data()), size_(other.size()) {}
    
    T* data() const { return data_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
    
    span<T> subspan(size_t offset, size_t count = static_cast<size_t>(-1)) const {
        if (count == static_cast<size_t>(-1)) count = size_ - offset;
        return span<T>(data_ + offset, count);
    }
    
    span<T> first(size_t count) const {
        return span<T>(data_, std::min(count, size_));
    }
    
private:
    T* data_;
    size_t size_;
};

class CpldManager
{
  public:
    CpldManager(uint8_t bus, uint8_t addr, const std::string& path,
                const std::string& chip, const std::string& interface,
                const std::string& target, bool debugMode) :
        bus(bus),
        addr(addr), imagePath(path), chip(chip), interface(interface),
        target(target), debugMode(debugMode)
    {
        if (interface == "i2c") // open I2C device
        {
            std::string filename = "/dev/i2c-" + std::to_string(bus);
            i2c_fd = open(filename.c_str(), O_RDWR);
            if (i2c_fd < 0)
            {
                std::cerr << "Error: Could not open file " << filename << ", "
                          << std::strerror(errno) << std::endl;
            }
        }
    }

    CpldManager() = delete;

    virtual ~CpldManager()
    {
        if (i2c_fd >= 0)
        {
            close(i2c_fd);
        }
    }

    virtual int fwUpdate(bool /* legacy */)
    {
        std::cerr << "fwUpdate() not implemented" << std::endl;
        return -1;
    }

    virtual int fwVerifyOnly(bool /* legacy */)
    {
        std::cerr << "fwVerifyOnly() not implemented" << std::endl;
        return -1;
    }

    virtual int getVersion()
    {
        std::cerr << "getVersion() not implemented" << std::endl;
        return -1;
    }

    int i2cWriteReadCmd(span<const uint8_t> cmdData, size_t rx_len = 0,
                        span<uint8_t> readData = {});
    uint8_t bus;
    uint8_t addr;
    std::string imagePath;
    std::string chip;
    std::string interface;
    std::string target;
    bool isLCMXO3D = false;
    bool debugMode = false;

  private:
    int i2c_fd = -1;
};
