#ifndef _BMC_CPLD_H_
#define _BMC_CPLD_H_
#include <string>
#include "fw-util.h"

class BmcCpldComponent : public Component {
  uint8_t bus = 0;
  uint8_t addr = 0;
  int get_cpld_version(uint8_t *ver);
  public:
    BmcCpldComponent(std::string fru, std::string board, std::string comp, uint8_t _bus, uint8_t _addr)
      : Component(fru, board, comp), bus(_bus), addr(_addr){}
    int print_version();
};

#endif
