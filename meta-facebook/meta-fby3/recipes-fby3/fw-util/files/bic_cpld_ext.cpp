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
          cout << "Failed to get slot" << slot_id << " 2ou board type\n";
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

int CpldExtComponent::get_version(json& j) {
  uint8_t ver[4] = {0};
  string board_name = name;
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  j["PRETTY_COMPONENT"] = board_name + " CPLD";

  try {
    server.ready();
    expansion.ready();
    if ( fw_comp == FW_BB_CPLD && bic_is_crit_act_ongoing(slot_id) == true ) {
      throw string("A critical activity is ongoing on the sled");
    }
    stringstream ss;
    if (bic_get_fw_ver(slot_id, fw_comp, ver) < 0) {
      throw "Error in getting the version of " + name;
    }
    ss << std::hex << std::uppercase << setfill('0')
       << setw(2) << +ver[0]
       << setw(2) << +ver[1]
       << setw(2) << +ver[2]
       << setw(2) << +ver[3];
    j["VERSION"] = ss.str();
  } catch(string& err) {
    j["PRETTY_VERSION"] = "NA (" + err + ")";
    if ( err.find("empty") != string::npos ) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
    }
  }
  return FW_STATUS_SUCCESS;
}
#endif

