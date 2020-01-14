#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/mman.h>
#include <syslog.h>
#include "bic_vr.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
using namespace std;
 
map<uint8_t, string> list = {{0xC0, "VCCIN/VSA"},
                             {0xC4, "VCCIO"},
                             {0xC8, "VDDQ_ABC"},
                             {0xCC, "VDDQ_DEF"}};

int VrComponent::print_version()
{
  uint8_t ver[6] = {0};
  uint8_t len = 0;
  int ret = 0;

  // Print VR Version
  for ( auto vr:list ) {
    try {
      server.ready();
      ret = bic_get_vr_ver(slot_id, FW_VR, ver, &len, vr.first, intf);
      if ( ret < 0 ) {
        throw "Error in getting the version of " + vr.second;
      } else {
        printf("%s %s Version: 0x", (len > 3)?"Renesas":"Texas Instruments", vr.second.c_str());
        for (int i = 0; i < len; i++) printf("%02X", ver[i]);
        printf("\n");
      }
    } catch (string err) {
      printf("%s Version: NA (%s)\n", vr.second.c_str(), err.c_str());
    }
  }

  return ret;
}

int VrComponent::update(string image)
{
  int ret;
  try {
    server.ready();
    return bic_update_fw(slot_id, UPDATE_VR, intf, (char *)image.c_str(), 0);
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}
#endif
