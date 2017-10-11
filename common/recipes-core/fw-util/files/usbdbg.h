#ifndef _USB_DBG_H_
#define _USB_DBG_H_
#include "fw-util.h"

class UsbDbgComponent : public Component {
  public:
    UsbDbgComponent(std::string fru, std::string comp, 
        uint8_t bus, uint8_t addr, uint8_t ioexp_addr);
    int update(std::string image);
};

class UsbDbgBlComponent : public Component {
  public:
    UsbDbgBlComponent(std::string fru, std::string comp, 
        uint8_t bus, uint8_t addr, uint8_t ioexp_addr);
    int update(std::string image);
};

#endif
