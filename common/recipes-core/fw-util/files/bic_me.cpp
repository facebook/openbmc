#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "bic_me.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int MeComponent::print_version() {
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
#endif

