#ifndef _USB_DBG_H_
#define _USB_DBG_H_

#include "mcu_fw.h"

class UsbDbgComponent : public McuFwComponent {
  uint8_t bus_id;
  uint8_t slv_addr;
  public:
    UsbDbgComponent(std::string fru, std::string comp, uint8_t bus, uint8_t addr)
      : McuFwComponent(fru, comp, bus, addr), bus_id(bus), slv_addr(addr) {}
    int print_version();
};

class UsbDbgBlComponent : public McuFwBlComponent {
  uint8_t bus_id;
  uint8_t slv_addr;
  public:
    UsbDbgBlComponent(std::string fru, std::string comp, uint8_t bus, uint8_t addr, uint8_t target)
      : McuFwBlComponent(fru, comp, bus, addr, target), bus_id(bus), slv_addr(addr) {}
    int print_version();
};

#endif
