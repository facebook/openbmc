#include <cstdio>
#include <stdexcept>
#include <openbmc/pal.h>
#include "expansion.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

void ExpansionBoard::ready()
{
  uint8_t type_2ou = 0xff;
  uint8_t config_status = 0;
  bool is_present = true;
  int ret = 0;
  uint8_t type = TYPE_1OU_UNKNOWN;
  uint8_t pwr_sts = 0x0;
  switch(fw_comp) {
    case FW_CPLD:
    case FW_SB_BIC:
    case FW_BB_BIC:
    case FW_BB_CPLD:
      return;
  }

  ret = bic_is_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    throw "Failed to get the config from " + fru + ":" + board_name;
  }

  config_status = (uint8_t) ret;
  if ( (config_status & PRESENT_1OU) == PRESENT_1OU ) {
    if (bic_get_1ou_type(slot_id, &type)) {
      throw string("Failed to get 1OU board type");
    }
  }

  switch (fw_comp) {
    case FW_1OU_BIC:
      if ( (config_status & PRESENT_1OU) != PRESENT_1OU )
        is_present = false;
      break;
    case FW_1OU_CPLD:
      if ( (config_status & PRESENT_1OU) != PRESENT_1OU ) {
        is_present = false;
      } else {
        if (bic_get_1ou_type(slot_id, &type)) {
          throw string("Failed to get 1OU board type");
        }
        if (type == TYPE_1OU_VERNAL_FALLS_WITH_AST) {
          throw string("Not present");
        }
      }
      break;
    case FW_1OU_CXL:
      if ( (config_status & PRESENT_1OU) != PRESENT_1OU ) {
        is_present = false;
      } else {
        uint8_t type = TYPE_1OU_UNKNOWN;
        if (bic_get_1ou_type(slot_id, &type)) {
          throw runtime_error("Failed to get 1OU board type");
        }
        if (type != TYPE_1OU_RAINBOW_FALLS) {
          throw runtime_error("Not present");
        }
      }
      break;
    case FW_2OU_BIC:
    case FW_2OU_CPLD:
      if ( fby35_common_get_2ou_board_type(slot_id, &type_2ou) < 0 ) {
        throw string("Failed to get 2OU board type");
      }
      if ( (config_status & PRESENT_2OU) != PRESENT_2OU || type_2ou == DPV2_BOARD )
        is_present = false;
      if (type == TYPE_1OU_OLMSTEAD_POINT) {
        // TODO: will check present pin after bring up
        is_present = true;
      }
      break;
    case FW_3OU_BIC:
    case FW_4OU_BIC:
      if ( (config_status & OP_PRESENT_3OU) != OP_PRESENT_3OU ) {
        is_present = false;
      }
      break;
    case FW_1OU_RETIMER:
      if ( (config_status & PRESENT_1OU) != PRESENT_1OU ) {
        is_present = false;
      }
      ret = pal_get_server_power(slot_id, &pwr_sts);
      if ( ret < 0 || pwr_sts == 0 )
          throw string("DC-off");
      break;
    case FW_3OU_RETIMER:
      if ( (config_status & OP_PRESENT_3OU) != OP_PRESENT_3OU ) {
        is_present = false;
      }
      ret = pal_get_server_power(slot_id, &pwr_sts);
      if ( ret < 0 || pwr_sts == 0 )
          throw string("DC-off");
      break;
    default:
      break;
  }

  if ( is_present == false )
    throw board_name + " is empty";
}
#endif
