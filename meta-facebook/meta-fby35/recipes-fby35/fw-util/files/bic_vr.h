#ifndef _BIC_VR_H_
#define _BIC_VR_H_
#include "fw-util.h"
#include "server.h"
#include "expansion.h"
#include <string>

using std::string;

class VrComponent : public Component {
  static bool vr_printed;
  static bool rbf_vr_printed;
  uint8_t slot_id;
  uint8_t fw_comp;
  Server server;
  private:
    int update_internal(const string& image, bool force);
    int get_ver_str(const string& name, string& s);
    std::map<uint8_t, map<uint8_t, string>>& get_vr_list();
  public:
    VrComponent(const string& fru, const string& comp, uint8_t comp_id)
      : Component(fru, comp), slot_id(fru.at(4) - '0'), fw_comp(comp_id), server(slot_id, fru) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

#endif
