#include "fw-util.h"
#include <sstream>
#include <openbmc/pal.h>
#include <openbmc/nm.h>

using namespace std;

class MeComponent : public Component {
  public:
    MeComponent(const string &fru, const string &comp)
      : Component(fru, comp) {}
    int get_version(json& j) {
      j["PRETTY_COMPONENT"] = "ME";
      char ver[128] = {0};
      NM_RW_INFO info = {NM_IPMB_BUS_ID, NM_SLAVE_ADDR, 0, 0};

      // Print ME Version
      if (lib_get_me_fw_ver(&info, (uint8_t *)ver)) {
        j["VERSION"] = "NA";
      } else {
        stringstream ss;
        ss << std::hex
          << +ver[0] << '.'
          << +ver[1] << '.'
          << +ver[2] << '.'
          << +ver[3] << +ver[4];
        j["VERSION"] = ss.str();
      }
      return 0;
    }
};

MeComponent me("mb", "me");
