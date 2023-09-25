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
    int get_ver_str(const uint8_t vr_comp, const string& name, string& s);
    std::map<uint8_t, map<uint8_t, string>>& get_vr_list();
  public:
    VrComponent(const string& fru, const string& comp, uint8_t comp_id)
      : Component(fru, comp), slot_id(fru.at(4) - '0'), fw_comp(comp_id), server(slot_id, fru) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

/* Definition of Halfdome board rev id:
 *   HW Stage : bit[3:0]
 *   VR Type   : bit[5:4]
* */
static inline int get_vr_type(uint8_t board_rev ) {
  return (board_rev >> 4) & 0x3;
}

#endif
