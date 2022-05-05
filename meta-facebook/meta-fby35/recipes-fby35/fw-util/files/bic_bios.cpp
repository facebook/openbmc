#include <cstdio>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include "bic_bios.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define MAX_RETRY 30
#define MUX_SWITCH_CPLD 0x07
#define MUX_SWITCH_PCH 0x03
#define GPIO_HIGH 1
#define GPIO_LOW 0

int BiosComponent::update_internal(const std::string& image, int fd, bool force) {
  int ret;
  int ret_recovery = 0, ret_reset = 0;
  uint8_t status;
  int retry_count = 0;

  try {
    cerr << "Checking if the server is ready..." << endl;
    server.ready();
  } catch(string& err) {
    cerr << "Server is not ready." << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }
  if (image.empty() && fd < 0) {
    cerr << "File or fd is required." << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }
  while (retry_count < MAX_RETRY) {
    ret = pal_get_server_power(slot_id, &status);
    cerr << "Server power status: " << (int) status << endl;
    if ( (ret == 0) && (status == SERVER_POWER_OFF) ){
      break;
    } else {
      uint8_t power_cmd = (force ? SERVER_POWER_OFF : SERVER_GRACEFUL_SHUTDOWN);
      if (retry_count == 0) {
        cerr << "Powering off the server (cmd " << ((int) power_cmd) << ")..." << endl;
      }
      pal_set_server_power(slot_id, power_cmd);
      retry_count++;
      sleep(2);
    }
  }
  if (retry_count == MAX_RETRY) {
    cerr << "Failed to Power Off Server " << slot_id << ". Stopping the update!" << endl;
    return -1;
  }

  if (!force) {
    cerr << "Putting ME into recovery mode..." << endl;
    ret_recovery = me_recovery(slot_id, RECOVERY_MODE);
  }
  // cerr << "Enabling USB..." << endl;
  // bic_set_gpio(slot_id, RST_USB_HUB_N_R, GPIO_HIGH);
  sleep(1);
  cerr << "Switching BIOS SPI MUX for update..." << endl;
  bic_set_gpio(slot_id, FM_SPI_PCH_MASTER_SEL_R, GPIO_HIGH);
  sleep(1);
  if (!image.empty()) {
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
  } else {
    ret = bic_update_fw_fd(slot_id, fw_comp, fd, FORCE_UPDATE_UNSET);
  }

  // cerr << "Disabling USB..." << endl;
  // bic_set_gpio(slot_id, RST_USB_HUB_N_R, GPIO_LOW);
  if (ret != 0) {
    cerr << "BIOS update failed. ret = " << ret << endl;
  }
  sleep(1);
  cerr << "Switching BIOS SPI MUX for default value..." << endl;
  bic_set_gpio(slot_id, FM_SPI_PCH_MASTER_SEL_R, GPIO_LOW);
  sleep(3);
  if (!force) {
    cerr << "Doing ME Reset..." << endl;
    ret_reset = me_reset(slot_id);
    sleep(5);
  }
  cerr << "Power-cycling the server..." << endl;
  if (ret_reset || ret_recovery) {
    syslog(LOG_CRIT, "Server 12V cycle due to %s failed when BIOS update", 
            ret_recovery ? "putting ME into recovery mode" : "ME reset");
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
  } else {
    pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
  }
  
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
