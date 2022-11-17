#include "bic_bios.h"
#include <sstream>
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include <openbmc/misc-utils.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define MAX_RETRY 30
#define MUX_SWITCH_CPLD 0x07
#define MUX_SWITCH_PCH 0x03
#define GPIO_HIGH 1
#define GPIO_LOW 0

int BiosComponent::update_internal(const std::string &image, int fd, bool force) {
  int ret;
  uint8_t status;
  int retry_count = 0;
  uint8_t bmc_location = 0;

  try {
    cerr << "Checking if the server is ready..." << endl;
    server.ready();
  } catch(string &err) {
    cerr << "Server is not ready." << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }
  if (image.empty() && fd < 0) {
    cerr << "File or fd is required." << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  if (!force) {
    cerr << "Powering off the server gracefully" << endl;
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);
    if (retry_cond(!pal_get_server_power(slot_id, &status) && status == SERVER_POWER_OFF, 5*60, 1000)) {
      cerr << "Retry powering off the server..." << endl;
      pal_set_server_power(slot_id, SERVER_POWER_OFF);
      if (retry_cond(!pal_get_server_power(slot_id, &status) && status == SERVER_POWER_OFF, 1, 1000)) {
        cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
        return -1;
      }
      // in power off condition, ME will be reset. We need to wait enough time for ME ready
      sleep(5);
    }
  } else {
    cerr << "Powering off the server forcefully" << endl;
    pal_set_server_power(slot_id, SERVER_POWER_OFF);
    if (retry_cond(!pal_get_server_power(slot_id, &status) && status == SERVER_POWER_OFF, 1, 1000)) {
      cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
      return -1;
    }
    // in force condition, ME will be reset. We need to wait enough time for ME ready
    sleep(5);
  }

  cerr << "Putting ME into recovery mode..." << endl;
  me_recovery(slot_id, RECOVERY_MODE);
  cerr << "Enabling USB..." << endl;
  if (pal_is_exp() == PAL_EOK) {
    bic_open_cwc_usb(slot_id);
  } else {
    bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_HIGH);
    sleep(1);
  }

  cerr << "Switching BIOS SPI MUX for update..." << endl;
  bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_CPLD);
  sleep(1);
  if (!image.empty()) {
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
  } else {
    ret = bic_update_fw_fd(slot_id, fw_comp, fd, FORCE_UPDATE_UNSET);
  }
  cerr << "Disabling USB..." << endl;
  if (pal_is_exp() == PAL_EOK) {
    bic_close_cwc_usb(slot_id);
  } else {
    bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_LOW);
    sleep(1);
  }

  if (ret != 0) {
    return -1;
  }
  
  cerr << "Switching BIOS SPI MUX for default value..." << endl;
  bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_PCH);
  sleep(3);
  cerr << "Doing ME Reset..." << endl;
  me_reset(slot_id);
  cerr << "Power-cycling the server..." << endl;
  fby3_common_get_bmc_location(&bmc_location);
  if ( bmc_location != NIC_BMC ) {
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
  }
  sleep(5);
  pal_set_server_power(slot_id, SERVER_POWER_ON);

  return ret;
}

int BiosComponent::update(string image) {
  return update_internal(image, -1, false /* force */);
}

int BiosComponent::update(int fd, bool force) {
  return update_internal("", fd, force);
}

int BiosComponent::fupdate(string image) {
  return update_internal(image, -1, true /* force */);
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
      throw std::runtime_error("Error in getting the version of BIOS");
    }
    cout << "BIOS Version: " << ver << endl;
  } catch(string& err) {
    printf("BIOS Version: NA (%s)\n", err.c_str());
  }

  return FW_STATUS_SUCCESS;
}


int BiosComponent::get_version(json& j) {
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
  return FW_STATUS_SUCCESS;
}

#endif
