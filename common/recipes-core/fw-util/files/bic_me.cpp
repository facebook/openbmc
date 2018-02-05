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
  uint8_t target_name[10] = {0};
  pal_get_me_name(target_name);
  try {
    server.ready();
    if (bic_get_fw_ver(slot_id, FW_ME, ver)){
      printf("%s Version: NA\n",target_name);
    }
    else {
      if(!strcmp((const char*)target_name, "ME"))
        printf("%s Version: %x.%x.%x.%x%x\n",target_name, ver[0], ver[1], ver[2], ver[3], ver[4]);
      else if(!strcmp((const char*)target_name, "IMC"))
        printf("%s Version: IMC.DF.%x.%x.%x-%x%x%x%x%x\n",target_name, ver[0], ver[1], ver[2], ver[3], ver[4], ver[5], ver[6], ver[7]);
    }
  } catch(string err) {
    printf("%s Version: NA (%s)\n",target_name, err.c_str());
  }
  return 0;
}
#endif

