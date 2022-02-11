#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "bic_cpld.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

class CpldExtComponent : public CpldComponent {
  uint8_t slot_id = 0;
  Server server;
  public:
    CpldExtComponent(string fru, string comp, uint8_t _slot_id)
      : CpldComponent(fru, comp, _slot_id), slot_id(_slot_id), server(_slot_id, fru) {}
    int print_version();
};

int CpldExtComponent::print_version() {
  try {
    server.ready();
    uint8_t ver[32] = {0};
    if (bic_get_fw_ver(slot_id, FW_CPLD, ver)) {
      printf("CPLD Version: NA\n");
    } else {
      if (fby2_get_slot_type(slot_id) == SLOT_TYPE_GPV2) {
        printf("CPLD Version: 0x%02x\n", ver[0]);
      } else {
        printf("CPLD Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
      }
    }
  } catch(string err) {
    printf("CPLD Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

// Register CPLD components
CpldExtComponent cpld1("slot1", "cpld", 1);
CpldExtComponent cpld2("slot2", "cpld", 2);
CpldExtComponent cpld3("slot3", "cpld", 3);
CpldExtComponent cpld4("slot4", "cpld", 4);

#endif
