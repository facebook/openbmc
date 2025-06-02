
#include "cpld-fw-handler.hpp"

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <thread>
#include <chrono>
#include <iomanip>
#include <iostream>

template<typename Func>
int retryCond(Func func, int numRetries, int msec) {
    for (int retries = 0; retries < numRetries; retries++) {
      if (func() < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
      } else {
        return 0;
      }
    }

    return -1;
}

int CpldManager::i2cWriteReadCmd(std::span<const uint8_t> cmdData,
                                 size_t rx_len, std::span<uint8_t> readData)
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
    int msg_count = 0;

    if (!cmdData.empty())
    {
        i2cmsg[0] = {.addr = addr,
                     .flags = 0,
                     .len = static_cast<uint16_t>(cmdData.size()),
                     .buf = const_cast<uint8_t*>(cmdData.data())};
        msg_count = 1;
    }
    if (rx_len > 0)
    {
        i2cmsg[msg_count] = {
            .addr = addr,
            .flags = I2C_M_RD,
            .len = static_cast<uint16_t>(rx_len),
            .buf = reinterpret_cast<uint8_t*>(readData.data())};
        msg_count++;
    }
    iomsg.msgs = i2cmsg;
    iomsg.nmsgs = msg_count;

    if (retryCond([&]() { return ioctl(i2c_fd, I2C_RDWR, &iomsg); }, 3, 10) < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << unsigned(bus)
                  << ", Addr: " << unsigned(addr)
                  << "errno = " << std::strerror(errno) << std::endl;
        return -1;
    }

    return 0;
}
