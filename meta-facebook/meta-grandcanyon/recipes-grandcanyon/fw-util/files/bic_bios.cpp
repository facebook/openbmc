#include "bic_bios.h"
#include <sstream>
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
#include <facebook/bic_fwupdate.h>
#include <facebook/bic_bios_fwupdate.h>

using namespace std;

#define MAX_GET_PWR_RETRY 30

// command to select mux to FPGA/PCH
#define MUX_SWITCH_FPGA 0x07
#define MUX_SWITCH_PCH 0x03

#define PWR_ON_DELAY 5

#define MAX_BIOS_VER_STR_LEN 32

enum {
  NORMAL_UPDATE = 0,
  FORCE_UPDATE,
  DUMP_FW
};
int BiosComponent::_update(string& image, uint8_t opt) {
  int ret = 0;
  uint8_t status = 0;
  int retry_count = 0;

  try {
    server.ready();
    if (opt != FORCE_UPDATE) {
      cout << "Shutting down server gracefully..." << endl;
      pal_set_server_power(FRU_SERVER, SERVER_GRACEFUL_SHUTDOWN);
  
      //Checking Server Power Status to make sure Server is really Off
      while (retry_count < MAX_GET_PWR_RETRY) {
        ret = pal_get_server_power(FRU_SERVER, &status);
        if ((ret == 0) && (status == SERVER_POWER_OFF)){
          break;
        } else {
          retry_count++;
          sleep(2);
        }
      }
      if (retry_count == MAX_GET_PWR_RETRY) {
        cerr << "Failed to Power Off Server. Stopping the update!" << endl;
        return -1;
      }  
      
      ret = bic_me_recovery(RECOVERY_MODE);
      if (ret < 0) {
        cerr << "Failed to set ME to recovery mode. Stopping the update!" << endl;
        ret = FW_STATUS_FAILURE;
        goto exit;
      }
      sleep(3);
    } else {
      cout << "Force updating BIOS firmware..." << endl;
    }
    
    bic_switch_mux_for_bios_spi(MUX_SWITCH_FPGA);
    sleep(1);
    if (opt == DUMP_FW) {
      ret = bic_dump_bios_fw((char *)image.c_str());
    } else {
      ret = bic_update_fw(FRU_SERVER, fw_comp, (char *)image.c_str(), (opt == FORCE_UPDATE) ? true : false);
    }
    if (ret != 0) {
      // recover to original setting
      bic_switch_mux_for_bios_spi(MUX_SWITCH_PCH);
      bic_me_recovery(RESTORE_FACTORY_DEFAULT);
    }
  exit:
    sleep(1);
    pal_set_server_power(FRU_SERVER, SERVER_12V_CYCLE);
    sleep(PWR_ON_DELAY);
    pal_set_server_power(FRU_SERVER, SERVER_POWER_ON);
  } catch(string &err) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  return ret;
}

int BiosComponent::update(string image) {
  return _update(image, NORMAL_UPDATE);
}

int BiosComponent::fupdate(string image) {
  return _update(image, FORCE_UPDATE);
}

int BiosComponent::dump(string image) {
  return _update(image, DUMP_FW);
}

int BiosComponent::get_ver_str(string& s) {
  uint8_t ver[MAX_BIOS_VER_STR_LEN] = {0};
  uint8_t fruid = 0;
  int ret = 0;
  stringstream  ss;

  ret = pal_get_fru_id((char *)_fru.c_str(), &fruid);
  if (ret < 0) {
    syslog(LOG_WARNING, "Failed to get fru id");
    return FW_STATUS_FAILURE;
  }

  ret = pal_get_sysfw_ver(fruid, ver);
  if (ret < 0) {
    syslog(LOG_WARNING, "Failed to get sysfw ver");
    return FW_STATUS_FAILURE;
  }

  ss << &ver[3];
  s = ss.str();

  return FW_STATUS_SUCCESS;
}

int BiosComponent::print_version() {
  string ver("");

  try {
    server.ready();
    if (get_ver_str(ver) < 0) {
      throw string("Error in getting the version of BIOS");
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
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the version of BIOS";
    }
    j["VERSION"] = ver;
  } catch(string& err) {
    if (err.find("empty") != string::npos) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
   }
  }
  return FW_STATUS_SUCCESS;
}

#endif
