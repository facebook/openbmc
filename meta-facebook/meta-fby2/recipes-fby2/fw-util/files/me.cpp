#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class MeComponent : public Component {
  uint8_t slot_id;
  Server server;
  public:
    MeComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id) {}
    int print_version() {
      uint8_t ver[32] = {0};
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      if (bic_get_fw_ver(slot_id, FW_ME, ver)){
        printf("ME Version: NA\n");
      }
      else {
        printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);
      }
      return 0;
    }
};

MeComponent me1("slot1", "me", 1);
MeComponent me2("slot2", "me", 2);
MeComponent me3("slot3", "me", 3);
MeComponent me4("slot4", "me", 4);
