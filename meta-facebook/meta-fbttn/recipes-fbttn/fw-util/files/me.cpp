#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class MeComponent : public Component {
  string fru_name;
  uint8_t slot_id = 0;
  Server server;
  char err_str[SERVER_ERR_STR_LEN] = {0};
  public:
    MeComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), fru_name(fru), slot_id(_slot_id), server(_slot_id, (char *)fru_name.c_str(), err_str) {}
    int print_version() {
      uint8_t ver[32] = {0};
      if (!server.ready()) {
        printf("ME Version: NA (%s)\n", err_str);
      } else {
        if (bic_get_fw_ver(slot_id, FW_ME, ver)){
          printf("ME Version: NA\n");
        }
        else {
          printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);
        }
      }
      return 0;
    }
};

MeComponent me1("server", "me", 1);
