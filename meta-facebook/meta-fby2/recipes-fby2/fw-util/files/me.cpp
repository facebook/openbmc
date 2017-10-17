#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class MeComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
    MeComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}
    int print_version() {
      uint8_t ver[32] = {0};
      try {
        server.ready();
        if (bic_get_fw_ver(slot_id, FW_ME, ver)){
          printf("ME Version: NA\n");
        }
        else {
          printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);
        }
      } catch(string err) {
        printf("ME Version: NA (%s)\n", err.c_str());
      }
      return 0;
    }
};

MeComponent me1("slot1", "me", 1);
MeComponent me2("slot2", "me", 2);
MeComponent me3("slot3", "me", 3);
MeComponent me4("slot4", "me", 4);
