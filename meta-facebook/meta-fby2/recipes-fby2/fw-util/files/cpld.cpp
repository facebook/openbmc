#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class CpldComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
    CpldComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}
    int print_version() {
      try {
        server.ready();
        uint8_t ver[32] = {0};
        if (bic_get_fw_ver(slot_id, FW_CPLD, ver)) {
          printf("CPLD Version: NA\n");
        }
        else {
          printf("CPLD Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
        }
      } catch(string err) {
        printf("CPLD Version: NA (%s)\n", err.c_str());
      }
      return 0;
    }
    int update(string image) {
      int ret = 0;
      try {
        server.ready();
        ret = bic_update_fw(slot_id, UPDATE_CPLD, (char *)image.c_str());
      } catch(string err) {
        ret = FW_STATUS_NOT_SUPPORTED;
      }
      return ret;
    }
};

CpldComponent cpld1("slot1", "cpld", 1);
CpldComponent cpld2("slot2", "cpld", 2);
CpldComponent cpld3("slot3", "cpld", 3);
CpldComponent cpld4("slot4", "cpld", 4);
