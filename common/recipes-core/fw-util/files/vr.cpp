#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include <syslog.h>
#ifdef COMMON_BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

class VrComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
  VrComponent(string fru, string comp, uint8_t _slot_id)
    : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}

  int print_version() {
    uint8_t ver[32] = {0};

    try {
      server.ready();

      /* Print PVCCIN VR Version */
      if (bic_get_fw_ver(slot_id, FW_PVCCIN_VR, ver)) {
        printf("PVCCIN VR Version: NA\n");
      } else {
        printf("PVCCIN VR Version: 0x%02x%02x, 0x%02x%02x\n",
               ver[0], ver[1], ver[2], ver[3]);
      }

      /* Print DDRAB VR Version */
      if (bic_get_fw_ver(slot_id, FW_DDRAB_VR, ver)){
        printf("DDRAB VR Version: NA\n");
      } else {
        printf("DDRAB VR Version: 0x%02x%02x, 0x%02x%02x\n",
               ver[0], ver[1], ver[2], ver[3]);
      }

      /* Print P1V05 VR Version */
      if (bic_get_fw_ver(slot_id, FW_P1V05_VR, ver)){
        printf("P1V05 VR Version: NA\n");
      } else {
        printf("P1V05 VR Version: 0x%02x%02x, 0x%02x%02x\n",
               ver[0], ver[1], ver[2], ver[3]);
      }
    } catch (string err) {
        printf("PVCCIN VR Version: NA (%s)\n", err.c_str());
        printf("DDRAB VR Version: NA (%s)\n", err.c_str());
        printf("P1V05 VR Version: NA (%s)\n", err.c_str());
    }
    return 0;
  }

  int update(string image) {
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

VrComponent vr("scm", "vr", 0);
#endif
