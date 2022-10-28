#ifndef _BIC_VR_H_
#define _BIC_VR_H_
#include <string>
#include "fw-util.h"
#include "server.h"
#include "expansion.h"

class VrComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  Server server;
  private:
    int get_ver_str(uint8_t& addr, std::string& s);
  public:
    VrComponent(std::string fru, std::string comp, uint8_t _slot_id, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), server(_slot_id, fru){}
    int print_version();
    int update(std::string image);
    int get_version(json& j) override;
};

class VrExtComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(std::string& s, const uint8_t alt_fw_comp);
  public:
    VrExtComponent(std::string fru, std::string comp, uint8_t _slot_id, std::string _name, int8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int print_version();
    int update(std::string image);
};

#endif
