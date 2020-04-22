#ifndef _BIC_VR_H_
#define _BIC_VR_H_
#include <string>
#include "fw-util.h"
#include "server.h"

class VrComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  Server server;
  public:
    VrComponent(std::string fru, std::string comp, uint8_t _slot_id, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), server(_slot_id, fru){}
    int print_version();
    int update(std::string image);
};

#endif
