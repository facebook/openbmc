#ifndef _BIC_VR_H_
#define _BIC_VR_H_
#include <string>
#include "fw-util.h"
#include "server.h"

class VrComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t intf = 0;
  Server server;
  public:
    VrComponent(std::string fru, std::string board, std::string comp, uint8_t _slot_id, uint8_t _intf)
      : Component(fru, board, comp), slot_id(_slot_id), intf(_intf), server(_slot_id, fru){}
    int print_version();
    int update(std::string image);
};

#endif
