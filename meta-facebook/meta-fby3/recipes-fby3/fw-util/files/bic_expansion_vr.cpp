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
#include <openbmc/pal.h>
using namespace std;

int VrExtComponent::get_ver_str(string& s, const uint8_t alt_fw_comp) {
  char ver[32] = {0};
  uint8_t rbuf[6] = {0};
  int ret = 0;
  ret = bic_get_fw_ver(slot_id, alt_fw_comp, rbuf);
  snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
  s = string(ver);
  if ( alt_fw_comp == FW_2OU_PESW_VR ) {
    s += ", Remaining Writes: " + to_string(rbuf[4]);
  }
  return ret;
}

void VrExtComponent::get_version(json& j) {
  return;
}

int VrExtComponent::print_version()
{
  map<uint8_t, string> list = {{FW_2OU_3V3_VR1, "VR P3V3_STBY1"},
                               {FW_2OU_3V3_VR2, "VR P3V3_STBY2"},
                               {FW_2OU_3V3_VR3, "VR P3V3_STBY3"},
                               {FW_2OU_PESW_VR, "VR PESW"}};
  string ver("");
  string board_name = name;
  string err_msg("");
 
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    uint8_t type = 0xff;
    if ( fby3_common_get_2ou_board_type(slot_id, &type) < 0 ) {
      throw string("Failed to get 2OU board type");
    } else if ( type != GPV3_MCHP_BOARD && type != GPV3_BRCM_BOARD ) {
      throw string("Not present");
    }
  } catch(string& err) {
    for ( auto& node:list ) {
      printf("%s %s Version: NA (%s)\n", board_name.c_str(), node.second.c_str(), err.c_str());
    }
    return FW_STATUS_SUCCESS;
  }

  for (auto& node:list ) {
    try {
      //Print VR FWs
      if ( get_ver_str(ver, node.first) < 0 ) {
        throw "Error in getting the version of " + board_name;
      }
      cout << board_name << " " << node.second << " Version: " << ver << endl;
    } catch(string& err) {
      printf("%s %s Version: NA (%s)\n", board_name.c_str(), node.second.c_str(), err.c_str());
    }
  } 

  return FW_STATUS_SUCCESS;
}

int VrExtComponent::update(string image)
{
  return FW_STATUS_NOT_SUPPORTED;
}
#endif
