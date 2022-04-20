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
  uint8_t rlen = 0;
  int ret = 0;
  bool expVr = alt_fw_comp == FW_2OU_PESW_VR || alt_fw_comp == FW_CWC_PESW_VR ||
            alt_fw_comp == FW_GPV3_TOP_PESW_VR || alt_fw_comp == FW_GPV3_BOT_PESW_VR;

  ret = bic_get_vr_vendor_fw_ver(slot_id, alt_fw_comp, rbuf, &rlen);

  if (rlen >= 6) {
    switch(rbuf[5]) {
      case VR_ISL: //Renesas vr vendor ID
        snprintf(ver, sizeof(ver), "Renesas %02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
        break;
      case VR_IFX: //Infineon vr vendor ID
        snprintf(ver, sizeof(ver), "Infineon %02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
        break;
      case VR_VY: //Vishay vr vendor ID
        snprintf(ver, sizeof(ver), "Vishay %02X%02X", rbuf[0], rbuf[1]);
        break;
      case VR_ON:
        snprintf(ver, sizeof(ver), "Onsemi %02X%02X", rbuf[0], rbuf[1]);
        break;
      default:
        snprintf(ver, sizeof(ver), "Unknown Vendor");
        break;
    }
    s = string(ver) + ", Remaining Writes: " + to_string(rbuf[4]);
  } else {
    if ( expVr ) {
      snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
    } else {
      snprintf(ver, sizeof(ver), "%02X%02X", rbuf[0], rbuf[1]);
    }
    s = string(ver) + ", Remaining Writes: " + to_string(expVr ? rbuf[4] : rbuf[2]);
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
                               {FW_2OU_1V8_VR, "VR P1V8"},
                               {FW_2OU_PESW_VR, "VR P0V84"},
                               {FW_CWC_PESW_VR, "VR P1V8/P0V84"},
                               {FW_GPV3_TOP_PESW_VR, "VR P0V84"},
                               {FW_GPV3_BOT_PESW_VR, "VR P0V84"},
                               {FW_2U_TOP_3V3_VR1, "VR P3V3_STBY1"},
                               {FW_2U_TOP_3V3_VR2, "VR P3V3_STBY2"},
                               {FW_2U_TOP_3V3_VR3, "VR P3V3_STBY3"},
                               {FW_2U_TOP_1V8_VR, "VR P1V8"},
                               {FW_2U_BOT_3V3_VR1, "VR P3V3_STBY1"},
                               {FW_2U_BOT_3V3_VR2, "VR P3V3_STBY2"},
                               {FW_2U_BOT_3V3_VR3, "VR P3V3_STBY3"},
                               {FW_2U_BOT_1V8_VR, "VR P1V8"}};
  string ver("");
  string board_name = name;
  string err_msg("");
  enum class VR_STATUS {
    VR_OK,
    VR_ERR,
    VR_INIT,
  };
  static VR_STATUS vr_status = VR_STATUS::VR_INIT;
  static string err_str("");

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);

  if ( VR_STATUS::VR_INIT == vr_status ) {
    try {
      server.ready();
      expansion.ready();
      vr_status = VR_STATUS::VR_OK;
    } catch(string& err) {
      vr_status = VR_STATUS::VR_ERR;
      err_str = err;
    }
  }

  if ( VR_STATUS::VR_ERR == vr_status ) {
    printf("%s %s Version: NA (%s)\n", board_name.c_str(), list[fw_comp].c_str(), err_str.c_str());
    return FW_STATUS_SUCCESS;
  }

  try {
    //Print VR FWs
    if ( get_ver_str(ver, fw_comp) < 0 ) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " " << list[fw_comp] << " Version: " << ver << endl;
  } catch(string& err) {
    printf("%s %s Version: NA (%s)\n", board_name.c_str(), list[fw_comp].c_str(), err.c_str());
  }

  return FW_STATUS_SUCCESS;
}

int VrExtComponent::update(string image)
{
  int ret = 0;
  try {
    server.ready();
    expansion.ready();
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
  } catch(string& err) {
    cout << "Failed Reason: " << err << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}
#endif
