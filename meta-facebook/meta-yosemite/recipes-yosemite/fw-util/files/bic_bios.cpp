#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "bic_bios.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int BiosComponent::update(string image) {
  int ret;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_POWER_OFF);
    ret = bic_update_firmware(slot_id, UPDATE_BIOS, (char *)image.c_str(), 0);
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosComponent::fupdate(string image) {
  int ret;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_POWER_OFF);
    ret = bic_update_firmware(slot_id, UPDATE_BIOS, (char *)image.c_str(), 1);
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosComponent::print_version() {
  uint8_t ver[32] = {0};
  int i;

  try {
    server.ready();
    if (pal_get_sysfw_ver(slot_id, ver)) {
      cout << "BIOS Version: NA" << endl;
    } else {
      // BIOS version response contains the length at offset 2 followed by ascii string
      printf("BIOS Version: ");
      for (i = 3; i < 3+ver[2]; i++) {
        printf("%c", ver[i]);
      }
      printf("\n");
    }
  } catch (string err) {
    printf("BIOS Version: NA (%s)\n", err.c_str());
  }
  return 0;
}
#endif
