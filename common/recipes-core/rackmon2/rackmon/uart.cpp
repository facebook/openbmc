#include <fcntl.h>
#include <linux/serial.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include "uart.hpp"

using namespace std::literals;

const std::unordered_map<int, speed_t> speed_map = {
    {0, B0},
    {50, B50},
    {110, B110},
    {134, B134},
    {150, B150},
    {200, B200},
    {300, B300},
    {600, B600},
    {1200, B1200},
    {1800, B1800},
    {2400, B2400},
    {4800, B4800},
    {9600, B9600},
    {19200, B19200},
    {38400, B38400},
    {57600, B57600},
    {115200, B115200}};

UARTDevice::~UARTDevice(void) {
  Device::close();
}

void UARTDevice::open() {
  Device::open();
  set_attribute(true);
}

void AspeedRS485Device::open() {
  UARTDevice::open();
  struct serial_rs485 rs485conf {};
  /*
   * NOTE: "SER_RS485_RTS_AFTER_SEND" and "SER_RS485_RX_DURING_TX" flags
   * are not handled in kernel 4.1, but they are required in the latest
   * kernel.
   */
  rs485conf.flags = SER_RS485_ENABLED;
  rs485conf.flags |= (SER_RS485_RTS_AFTER_SEND | SER_RS485_RX_DURING_TX);

  ioctl(TIOCSRS485, &rs485conf);
}

void UARTDevice::set_attribute(bool read_en) {
  struct termios tio = {0};
  cfsetspeed(&tio, speed_map.at(baudrate));
  tio.c_cflag |= PARENB;
  tio.c_cflag |= CLOCAL;
  tio.c_cflag |= CS8;
  if (read_en)
    tio.c_cflag |= CREAD;
  tio.c_iflag |= INPCK;
  tio.c_cc[VMIN] = 1;
  tio.c_cc[VTIME] = 0;

  if (tcsetattr(dev_fd, TCSANOW, &tio)) {
    throw std::runtime_error("Setting attribute failed");
  }
}

void UARTDevice::wait_write() {
  int loops;
  for (loops = 0; loops < 100; loops++) {
    int lsr;
    ioctl(TIOCSERGETLSR, &lsr);
    if (lsr & TIOCSER_TEMT)
      break;
    // never should hit this if kernel supports RS485 mode
  }
  if (loops > 0)
    std::cerr << "Waited loops: " << loops << '\n';
}

void UARTDevice::write(const uint8_t* buf, size_t len) {
  // Write with read disabled. Hence we need to do this
  // as fast as possible. So, muck around with the priorities
  // to make sure we get there.
  struct sched_param sp;
  sp.sched_priority = 50;
  pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
  read_disable();
  Device::write(buf, len);
  wait_write();
  read_enable();
  pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);
}
