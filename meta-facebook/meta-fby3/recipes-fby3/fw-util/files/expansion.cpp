#include <iostream>
#include <cstdio>
#include <cstring>
#include "expansion.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

void ExpansionBoard::ready()
{
  uint8_t config_status = 0;
  bool is_present = true;
  int ret = 0;

  switch(fw_comp) {
    case FW_CPLD:
    case FW_BIC:
    case FW_BIC_BOOTLOADER:
    case FW_BB_BIC:
    case FW_BB_BIC_BOOTLOADER:
    case FW_BB_CPLD:
      return;
  }

  ret = bic_is_m2_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    throw "Failed to get the config from " + fru + ":" + board_name;
  }

  config_status = (uint8_t) ret;
  switch (fw_comp) {
    case FW_1OU_BIC:
    case FW_1OU_BIC_BOOTLOADER:
    case FW_1OU_CPLD:
      if ( (config_status & PRESENT_1OU) != PRESENT_1OU ) 
        is_present = false;
      break;
    case FW_2OU_BIC:
    case FW_2OU_BIC_BOOTLOADER:
    case FW_2OU_CPLD:
      if ( (config_status & PRESENT_2OU) != PRESENT_2OU )
        is_present = false;
      break;
    default:
      break;
  }

  if ( is_present == false )
    throw board_name + " is empty";
}
#endif
