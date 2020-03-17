#include "fw-util.h"
#include <openbmc/pal.h>

using namespace std;

class MeComponent : public Component {
  public:
    MeComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version() {
      char ver[128] = {0};
      // Print ME Version
      if (pal_get_me_fw_ver(NM_IPMB_BUS_ID, NM_SLAVE_ADDR, (uint8_t *)ver)) {
        printf("ME Version: NA\n");
      } else {
        printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);
      }
      return 0;
    }
};

MeComponent me("mb", "me");
