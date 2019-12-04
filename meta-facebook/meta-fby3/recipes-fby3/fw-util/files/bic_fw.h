#ifndef _BIC_FW_H_
#define _BIC_FW_H_
#include "fw-util.h"
#include "server.h"

class BicFwComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t intf = 0;
  Server server;
  public:
    BicFwComponent(std::string fru, std::string board, std::string comp, uint8_t _slot_id, uint8_t _intf)
      : Component(fru, board, comp), slot_id(_slot_id), intf(_intf), server(_slot_id, fru) {}
    int update(std::string image);
    int fupdate(std::string image);
    int print_version();
};

class BicFwBlComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t intf = 0;
  Server server;
  public:
    BicFwBlComponent(std::string fru, std::string board, std::string comp, uint8_t _slot_id, uint8_t _intf)
      : Component(fru, board, comp), slot_id(_slot_id), intf(_intf), server(_slot_id, fru) {}
    int update(std::string image);
    int print_version();
};

class BicFwExtComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t intf = 0;
  Server server;
  public:
    BicFwExtComponent(std::string fru, std::string board, std::string comp, uint8_t _slot_id, uint8_t _intf)
      : Component(fru, board, comp), slot_id(_slot_id), intf(_intf), server(_slot_id, fru) {}
    int update(std::string image);
    int fupdate(std::string image);
    int print_version();
};

class BicFwBlExtComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t intf = 0;
  Server server;
  public:
    BicFwBlExtComponent(std::string fru, std::string board, std::string comp, uint8_t _slot_id, uint8_t _intf)
      : Component(fru, board, comp), slot_id(_slot_id), intf(_intf), server(_slot_id, fru) {}
    int update(std::string image);
    int print_version();
};

#endif
