#ifndef _USB_DBG_H_
#define _USB_DBG_H_

#include "mcu_fw.h"

class UsbDbgComponent : public McuFwComponent {
  std::string pld_name;
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t type;
  public:
    UsbDbgComponent(std::string fru, std::string comp, std::string name, uint8_t bus, uint8_t addr, uint8_t is_signed)
      : McuFwComponent(fru, comp, name, bus, addr, is_signed), bus_id(bus), slv_addr(addr), type(is_signed) {}
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
