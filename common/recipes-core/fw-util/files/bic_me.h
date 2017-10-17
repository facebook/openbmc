#ifndef BIC_ME_H_
#define BIC_ME_H_
#include "fw-util.h"
#include "server.h"


class MeComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
    MeComponent(std::string fru, std::string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}
    int print_version();
};

#endif
