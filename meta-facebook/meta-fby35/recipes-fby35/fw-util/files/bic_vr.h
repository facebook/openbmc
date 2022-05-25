#ifndef _BIC_VR_H_
#define _BIC_VR_H_
#include "fw-util.h"
#include "server.h"
#include "expansion.h"
#include <string>

using std::string;

class VrComponent : public Component {
  static bool vr_printed;
  uint8_t slot_id;
  uint8_t fw_comp;
  Server server;
  private:
    int update_internal(const string& image, bool force);
    int get_ver_str(const string& name, string& s);
  public:
    VrComponent(const string& fru, const string& comp, uint8_t comp_id)
      : Component(fru, comp), slot_id(fru.at(4) - '0'), fw_comp(comp_id), server(slot_id, fru) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    void get_version(json& j);
};

class VrExtComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(string& s, const uint8_t alt_fw_comp);
  public:
    VrExtComponent(const string& fru, const string& comp, uint8_t _slot_id, const string& _name, int8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int print_version();
    int update(std::string image);
    void get_version(json& j);
};

#endif
