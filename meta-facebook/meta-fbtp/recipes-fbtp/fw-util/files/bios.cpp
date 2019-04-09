#include "fw-util.h"
#include <openbmc/bios.h>
#include <openbmc/pal.h>

using namespace std;

class BiosComponent : public Component {
  public:
    BiosComponent(string fru, string comp)
      : Component(fru, comp) {}
    int update(string image) {
      return bios_program(FRU_MB, image.c_str(), true);
    }
    int fupdate(string image) {
      return bios_program(FRU_MB, image.c_str(), false);
    }
    int print_version() {
      char ver[128] = {0};
      // Print BIOS version
      if (bios_get_ver(FRU_MB, ver)) {
        printf("BIOS Version: NA\n");
      } else {
        printf("BIOS Version: %s\n", (char *)ver);
      }
      return 0;
    }
};

BiosComponent bios("mb", "bios");
