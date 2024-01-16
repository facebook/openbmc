#include "fw-util.h"
#include "server.h"
#include <openbmc/pal.h>
#include <syslog.h>
#ifdef COMMON_BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

class VrComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t vr_id = 0;
  string vr_name{};
  Server server;
  public:
  VrComponent(string fru, string comp, uint8_t _slot_id, uint8_t _vr_id, const std::string& _vr_name)
    : Component(fru, comp), slot_id(_slot_id), vr_id(_vr_id), vr_name(_vr_name), server(_slot_id, fru) {}

  int get_version(json& j) override {
    if (vr_name == "") {
      return FW_STATUS_NOT_SUPPORTED;
    }
    j["PRETTY_COMPONENT"] = vr_name;
    uint8_t ver[32] = {0};

    try {
      server.ready();

      /* Print PVCCIN VR Version */
      if (bic_get_fw_ver(slot_id, vr_id, ver)) {
        j["VERSION"] = "NA";
      } else {
        stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(2)
          << "0x" << +ver[0] << +ver[1]
          << ", 0x" << +ver[2] << +ver[3];
        j["VERSION"] = ss.str();
      }
    } catch (string err) {
      j["VERSION"] = "NA (" + err + ")";
    }
    return 0;
  }

  int update(string image) override {
    int ret;

    try {
      server.ready();
      ret = bic_update_fw(slot_id, UPDATE_VR, (char *)image.c_str());
    } catch(string err) {
      ret = FW_STATUS_NOT_SUPPORTED;
    }
    return ret;
  }
};

VrComponent vr("scm", "vr", 0, 0, "");
VrComponent vr_pvccin("scm", "vr_pvccin", 0, FW_PVCCIN_VR, "PVCCIN VR");
VrComponent vr_ddrab("scm", "vr_ddrab", 0, FW_DDRAB_VR, "DDRAB VR");
VrComponent vr_p1v05("scm", "vr_p1v05", 0, FW_P1V05_VR, "P1V05 VR");
#endif
