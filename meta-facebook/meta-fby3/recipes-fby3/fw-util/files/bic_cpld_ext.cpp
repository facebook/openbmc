#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_cpld_ext.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int CpldExtComponent::update_internal(string& image, bool force) {
  int m2_prsnt;
  int ret = 0;
  int intf = FEXP_BIC_INTF;
  uint8_t type_2ou = UNKNOWN_BOARD;
  string comp = component();
  try {
    server.ready();
    expansion.ready();
    if (comp == "1ou_cpld") {
      intf = FEXP_BIC_INTF;
    } else {
      intf = REXP_BIC_INTF;
      m2_prsnt = bic_is_m2_exp_prsnt(slot_id);
      if (m2_prsnt < 0) {
        throw "Failed to get 1ou & 2OU present status\n";
      }
      if ( (m2_prsnt & PRESENT_2OU) == PRESENT_2OU ) {
        if ( fby3_common_get_2ou_board_type(slot_id, &type_2ou) < 0) {
          syslog(LOG_WARNING, "Failed to get slot%d 2ou board type\n",slot_id);
          printf("Failed to get slot%d 2ou board type\n",slot_id);
        }
      }
    }
    if ( type_2ou != GPV3_MCHP_BOARD && type_2ou != GPV3_BRCM_BOARD && type_2ou != CWC_MCHP_BOARD ) {
      remote_bic_set_gpio(slot_id, EXP_GPIO_RST_USB_HUB, VALUE_HIGH, intf);
    }
    if (pal_is_exp() == PAL_EOK) {
      bic_open_cwc_usb(slot_id);
    } else {
      bic_set_gpio(slot_id, GPIO_RST_USB_HUB, VALUE_HIGH);
    }
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), (force ? FORCE_UPDATE_SET : FORCE_UPDATE_UNSET));
  } catch (string& err) {
    syslog(LOG_WARNING, "%s(): %s", __func__, err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int CpldExtComponent::update(string image) {
  return update_internal(image, false);
}

int CpldExtComponent::fupdate(string image) {
  return update_internal(image, true);
}

int CpldExtComponent::get_ver_str(string& s) {
  char ver[32] = {0};
  uint8_t rbuf[4] = {0};
  int ret = 0;
  ret = bic_get_fw_ver(slot_id, fw_comp, rbuf);
  snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
  s = string(ver);
  return ret;
}

int CpldExtComponent::print_version() {
  string ver("");
  string board_name = name;
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    if ( fw_comp == FW_BB_CPLD && bic_is_crit_act_ongoing(slot_id) == true ) {
      throw string("A critical activity is ongoing on the sled");
    }

    // Print Bridge-IC Version
    if ( get_ver_str(ver) < 0 ) {
      throw string("Error in getting the version of " + board_name);
    }
    cout << board_name << " CPLD Version: " << ver << endl;
  } catch(string& err) {
    printf("%s CPLD Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

int CpldExtComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + name;
    }

    j["VERSION"] = ver;
  } catch(string& err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
  return FW_STATUS_SUCCESS;
}
#endif

