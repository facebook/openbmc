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
#include <openbmc/kv.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
using namespace std;
 
map<uint8_t, string> list = {{0xC0, "VCCIN/VSA"},
                             {0xC4, "VCCIO"},
                             {0xC8, "VDDQ_ABC"},
                             {0xCC, "VDDQ_DEF"}};

int VrComponent::print_version()
{
  const uint8_t bus = 0x8;
  uint8_t addr;
  int ret = 0;
  char ver_str[MAX_VALUE_LEN] = {0};

  // Print VR Version
  for ( auto vr:list ) {
    try {
      addr = vr.first;
      server.ready();
      
      ret = bic_get_vr_ver_cache(slot_id, intf, bus, addr, ver_str);
      if ( ret == 0 ) {
        printf("%s Version : %s \n", vr.second.c_str(), ver_str);  //ex. ver_str = Infineon 0x588C254B
      } 
      
    } catch (string err) {
      printf("%s Version : NA (%s)\n", vr.second.c_str(), err.c_str());
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
