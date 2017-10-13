#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class CpldComponent : public Component {
  string fru_name;
  uint8_t slot_id = 0;
  Server server;
  char err_str[SERVER_ERR_STR_LEN] = {0};
  public:
    CpldComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), fru_name(fru), slot_id(_slot_id), server(_slot_id, (char *)fru_name.c_str(), err_str) {}
    int print_version() {
      if (!server.ready()) {
        printf("CPLD Version: NA (%s)\n", err_str);
      } else {
        uint8_t ver[32] = {0};
        if (bic_get_fw_ver(slot_id, FW_CPLD, ver)) {
          printf("CPLD Version: NA\n");
        }
        else {
          printf("CPLD Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
        }
      }
      return 0;
    }
    int update(string image) {
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      return bic_update_fw(slot_id, UPDATE_CPLD, (char *)image.c_str());
    }
};

CpldComponent cpld1("server", "cpld", 1);
