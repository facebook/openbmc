#ifndef _BIC_FW_H_
#define _BIC_FW_H_
#include "fw-util.h"
#include "server.h"

class BicFwComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
    BicFwComponent(std::string fru, std::string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}
    int update(std::string image);
    int fupdate(std::string image);
    int print_version();
};

class BicFwBlComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
    BicFwBlComponent(std::string fru, std::string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}
    int update(std::string image);
    int print_version();
};

#endif
