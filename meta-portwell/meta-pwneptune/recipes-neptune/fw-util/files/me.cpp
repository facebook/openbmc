#include "fw-util.h"
#include <openbmc/me.h>

using namespace std;

class MeComponent : public Component {
  public:
    MeComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version() {
      char ver[128] = {0};
      // Print ME Version
      if (me_get_fw_ver((uint8_t *)ver)){
        printf("ME Version: NA\n");
      } else {
        printf("ME Version: SPS_%02X.%02X.%02X.%02X%X.%X\n", ver[0], ver[1]>>4, ver[1] & 0x0F,
                    ver[3], ver[4]>>4, ver[4] & 0x0F);
      }
      return 0;
    }
};

MeComponent me("mb", "me");
