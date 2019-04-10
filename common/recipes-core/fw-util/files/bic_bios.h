#ifndef BIC_BIOS_H_
#define BIC_BIOS_H_
#include "fw-util.h"
#include "server.h"


class BiosComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
    BiosComponent(std::string fru, std::string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}
    int update(std::string image);
    int fupdate(std::string image);
    int print_version();
};

#endif
