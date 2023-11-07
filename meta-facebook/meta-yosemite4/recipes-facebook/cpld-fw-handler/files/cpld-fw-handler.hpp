#pragma once

#include <fcntl.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class CpldManager
{
  public:
    CpldManager(uint8_t bus, uint8_t addr, const std::string& path,
                const std::string& chip, const std::string& interface,
                bool debugMode) :
        bus(bus),
        addr(addr), imagePath(path), chip(chip), interface(interface),
        debugMode(debugMode)
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

    virtual int fwUpdate()
    {
        std::cerr << "fwUpdate() not implemented" << std::endl;
        return -1;
    }

    virtual int getVersion()
    {
        std::cerr << "getVersion() not implemented" << std::endl;
        return -1;
    }

    int i2cWriteReadCmd(const std::vector<uint8_t>& cmdData, size_t rx_len,
                        std::vector<uint8_t>& readData);
    uint8_t bus;
    uint8_t addr;
    std::string imagePath;
    std::string chip;
    std::string interface;
    bool isLCMXO3D = false;
    bool debugMode = false;

  private:
    int i2c_fd = -1;
};
