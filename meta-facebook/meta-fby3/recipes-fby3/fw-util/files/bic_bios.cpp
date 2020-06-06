#include "bic_bios.h"
#include <sstream>
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define MAX_RETRY 30
#define MUX_SWITCH_CPLD 0x07
#define MUX_SWITCH_PCH 0x03
#define GPIO_RST_USB_HUB 0x10
#define GPIO_HIGH 1
#define GPIO_LOW 0

int BiosComponent::update(string image) {
  int ret;
  uint8_t status;
  int retry_count = 0;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);

    //Checking Server Power Status to make sure Server is really Off
    while (retry_count < MAX_RETRY) {
      ret = pal_get_server_power(slot_id, &status);
      if ( (ret == 0) && (status == SERVER_POWER_OFF) ){
        break;
      } else {
        retry_count++;
        sleep(2);
      }
    }
    if (retry_count == MAX_RETRY) {
      cerr << "Failed to Power Off Server " << slot_id << ". Stopping the update!" << endl;
      return -1;
    }

    me_recovery(slot_id, RECOVERY_MODE);
    bic_set_gpio(slot_id, GPIO_RST_USB_HUB, GPIO_HIGH);
    sleep(3);
    bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_CPLD);
    sleep(1);
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
    if (ret != 0) {
      return -1;
    }
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
    bic_set_gpio(slot_id, GPIO_RST_USB_HUB, GPIO_LOW);

  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int BiosComponent::get_ver_str(string& s) {
  uint8_t ver[32] = {0};
  uint8_t fruid = 0;
  int ret = 0;

  ret = pal_get_fru_id((char *)_fru.c_str(), &fruid);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Failed to get fru id");
    return FW_STATUS_FAILURE;
  }

  ret = pal_get_sysfw_ver(fruid, ver);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Failed to get sysfw ver");
    return FW_STATUS_FAILURE;
  }

  stringstream  ss;
  ss << &ver[3];
  s = ss.str();
  return FW_STATUS_SUCCESS;
}

int BiosComponent::print_version() {
  string ver("");
  try {
    server.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of BIOS";
    }
    cout << "BIOS Version: " << ver << endl;
  } catch(string& err) {
    printf("BIOS Version: NA (%s)\n", err.c_str());
  }

  return FW_STATUS_SUCCESS;
}


void BiosComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of BIOS";
    }
    j["VERSION"] = ver;
  } catch(string& err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
  return;
}

#endif
