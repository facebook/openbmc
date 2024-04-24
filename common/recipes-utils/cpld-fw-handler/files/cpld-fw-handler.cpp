
#include "cpld-fw-handler.hpp"

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>

#include <iomanip>
#include <iostream>

int CpldManager::i2cWriteReadCmd(const std::vector<uint8_t>& cmdData,
                                 size_t rx_len, std::vector<uint8_t>& readData)
{
    if (debugMode)
    {
        std::cout << "[DEBUG] CMD data = ";
        for (const auto& i : cmdData)
        {
            std::cout << std::hex << std::setfill('0') << std::setw(2)
                      << unsigned(i) << " ";
        }
        std::cout << "\n";
    }

    struct i2c_rdwr_ioctl_data iomsg;
    struct i2c_msg i2cmsg[2];

    i2cmsg[0].addr = addr & 0xFF;
    i2cmsg[0].flags = 0;
    i2cmsg[0].len = cmdData.size();
    i2cmsg[0].buf = const_cast<uint8_t*>(cmdData.data());
    iomsg.nmsgs = 1;
    if (rx_len > 0)
    {
        i2cmsg[1].addr = addr & 0xFF;
        i2cmsg[1].flags = I2C_M_RD;
        i2cmsg[1].len = rx_len;
        i2cmsg[1].buf = reinterpret_cast<uint8_t*>(readData.data());
        iomsg.nmsgs++;
    }
    iomsg.msgs = i2cmsg;
    if (ioctl(i2c_fd, I2C_RDWR, &iomsg) < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << unsigned(bus)
                  << ", Addr: " << unsigned(addr) << "\n";
        return -1;
    }
    return 0;
}
