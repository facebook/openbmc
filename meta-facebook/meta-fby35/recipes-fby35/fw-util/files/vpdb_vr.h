#ifndef _VPDB_VR_H_
#define _VPDB_VR_H_
#include "fw-util.h"
#include <string>

using namespace std;

class VpdbVrComponent : public Component {
  uint8_t slot_id;
  uint8_t fw_comp;
  private:
    int update_internal(const string& image, bool force);
    int get_ver_str(const string& name, string& s);
  public:
    VpdbVrComponent(const string& fru, const string& comp, uint8_t comp_id)
      : Component(fru, comp), slot_id(FRU_BMC), fw_comp(comp_id) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

#endif
