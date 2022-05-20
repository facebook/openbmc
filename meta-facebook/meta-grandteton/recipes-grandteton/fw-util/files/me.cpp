#include "fw-util.h"
#include <openbmc/pal.h>
#include <openbmc/nm.h>

using namespace std;

class MeComponent : public Component {
  public:
    MeComponent(const string &fru, const string &comp)
      : Component(fru, comp) {}
    int print_version() {
      char ver[128] = {0};
      NM_RW_INFO info = {.bus = NM_IPMB_BUS_ID,
                         .nm_addr = NM_SLAVE_ADDR};

      // Print ME Version
      if (lib_get_me_fw_ver(&info, (uint8_t *)ver)) {
        printf("ME Version: NA\n");
      } else {
        printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);
      }
      return 0;
    }
};

MeComponent me("mb", "me");
