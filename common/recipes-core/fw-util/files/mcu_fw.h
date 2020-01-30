#ifndef _MCU_FW_H_
#define _MCU_FW_H_

#include "fw-util.h"

class McuFwComponent : public Component {
  std::string pld_name;
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t type;
  public:
    McuFwComponent(std::string fru, std::string comp, std::string name, uint8_t bus, uint8_t addr, uint8_t is_signed)
      : Component(fru, comp), pld_name(name), bus_id(bus), slv_addr(addr), type(is_signed) {}
    int update(std::string image);
};

class McuFwBlComponent : public Component {
  uint8_t bus_id;
  uint8_t slv_addr;
  uint8_t target_id;
  public:
    McuFwBlComponent(std::string fru, std::string comp, uint8_t bus, uint8_t addr, uint8_t target)
      : Component(fru, comp), bus_id(bus), slv_addr(addr), target_id(target) {}
    int update(std::string image);
};

#endif
