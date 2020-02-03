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
#include "bic_cpld.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
using namespace std;

int CpldComponent::print_version()
{
  uint8_t ver[4] = {0};
  int ret = 0;
  string board_name = board();

  //make the strig to uppercase.
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);

  try {
    server.ready();
    if ( intf == NONE_INTF || intf == BB_BIC_INTF ) {
      ; // do nothing
    } else {
      ret = bic_is_m2_exp_prsnt(slot_id);
      if ( ret < 0 ) {
        throw "Error in getting the present status of " + board_name;
      } else if ( intf == FEXP_BIC_INTF && (ret == PRESENT_1OU || ret == (PRESENT_1OU + PRESENT_2OU)) ) {
        ; // Correct status, do nothing
      } else if ( intf == REXP_BIC_INTF && (ret == PRESENT_2OU || ret == (PRESENT_1OU + PRESENT_2OU)) ) {
        ; // Correct status, do nothing
      } else {
        throw board_name + " Board is empty";
      }
    }
    ret = bic_show_fw_ver(slot_id, FW_CPLD, ver, bus, addr, intf);
    // Print CPLD Version
    if ( ret < 0 ) {
      throw  "Error in getting the version of " + board_name;
    } else {
      printf("%s CPLD Version: %02X%02X%02X%02X\n", board_name.c_str(), ver[3], ver[2], ver[1], ver[0]);
    }
  } catch(string err) {
    printf("%s CPLD Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return ret;

}

int CpldComponent::update(string image)
{
  int ret = 0;
  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_CPLD, intf, (char *)image.c_str(), 0);
  } catch (string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}
#endif
