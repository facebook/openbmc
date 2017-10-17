#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
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
      // Print PVCCIN VR Version
      if (bic_get_fw_ver(slot_id, FW_PVCCIN_VR, ver)){
        printf("PVCCIN VR Version: NA\n");
      }
      else {
        printf("PVCCIN VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
      }

      // Print DDRAB VR Version
      if (bic_get_fw_ver(slot_id, FW_DDRAB_VR, ver)){
        printf("DDRAB VR Version: NA\n");
      } else {
        printf("DDRAB VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
      }

      // Print P1V05 VR Version
      if (bic_get_fw_ver(slot_id, FW_P1V05_VR, ver)){
        printf("P1V05 VR Version: NA\n");
      }
      else {
        printf("P1V05 VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
      }

      // Print PVCCGBE VR Version
      if (bic_get_fw_ver(slot_id, FW_PVCCGBE_VR, ver)){
        printf("PVCCGBE VR Version: NA\n");
      } else {
        printf("PVCCGBE VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
      }

      // Print PVCCSCSUS VR Version
      if (bic_get_fw_ver(slot_id, FW_PVCCSCSUS_VR, ver)){
        printf("PVCCSCSUS VR Version: NA\n");
      } else {
        printf("PVCCSCSUS VR Version: 0x%02x%02x, 0x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
      }
    } catch (string err) {
      printf("PVCCIN VR Version: NA (%s)\n", err.c_str());
      printf("DDRAB VR Version: NA (%s)\n", err.c_str());
      printf("P1V05 VR Version: NA (%s)\n", err.c_str());
      printf("PVCCGBE VR Version: NA (%s)\n", err.c_str());
      printf("PVCCSCSUS VR Version: NA (%s)\n", err.c_str());
    }
    return 0;
  }
};

VrComponent vr1("server", "vr", 1);

