#include "fw-util.h"
#include "server.h"

class CapsuleComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  Server server;
  public:
    CapsuleComponent(std::string fru, std::string comp, uint8_t _slot_id, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), server(_slot_id, fru) {}
    int update(std::string image);
    int fupdate(std::string image);
    int print_version();
};
