#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_me.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int MeFwComponent::print_version() {
  uint8_t ver[32] = {0};
  int ret = 0;
  string board_name = board();

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    ret = bic_show_fw_ver(slot_id, FW_ME, ver, 0, 0, intf);
    // Print ME Version
    if ( ret < 0 ) {
      throw "Error in getting the version of " + board_name;
    } else {
      printf("ME Version: %x.%x.%x.%x%x\n", ver[0], ver[1], ver[2], ver[3], ver[4]);
    }
  } catch(string err) {
    printf("ME Version: NA (%s)\n", err.c_str());
  }
  return ret;
}

#endif

