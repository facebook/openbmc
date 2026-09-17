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

#define GRACEFUL_OFF_MAX_RETRY 30
#define GRACEFUL_OFF_RETRY_DELAY 2

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

#ifdef CONFIG_GRANDCANYON2
#define MAX_PWR_OFF_RETRY 5
#define FORCE_OFF_RETRY_DELAY 1
#define ME_RECOVERY_MAX_ATTEMPTS 10
#define ME_RECOVERY_RETRY_DELAY 1

static int wait_server_power_off(int max_retry, int delay_sec)
{
  uint8_t status = SERVER_POWER_ON;

  for (int retry = 0; retry < max_retry; retry++) {
    if ((pal_get_server_power(FRU_SERVER, &status) == 0) &&
        (status == SERVER_POWER_OFF)) {
      return 0;
    }

    sleep(delay_sec);
  }

  return -1;
}

static int attempt_server_power_off(bool force)
{
  // Force update:
  if (force) {
    cout << "Force powering off server..." << endl;

    if (pal_set_server_power(FRU_SERVER, SERVER_POWER_OFF) < 0) {
      cerr << "Failed to execute server power off." << endl;
      return -1;
    }

    if (wait_server_power_off(MAX_PWR_OFF_RETRY, FORCE_OFF_RETRY_DELAY) < 0) {
      cerr << "Failed to Power Off Server." << endl;
      return -1;
    }

    return 0;
  }

  // Normal update:
  cout << "Shutting down server gracefully..." << endl;

  if (pal_set_server_power(FRU_SERVER, SERVER_GRACEFUL_SHUTDOWN) < 0) {
    cerr << "Failed to execute graceful shutdown, trying power off directly..." << endl;
  } else if (wait_server_power_off(GRACEFUL_OFF_MAX_RETRY, GRACEFUL_OFF_RETRY_DELAY) == 0) {
    return 0;
  }

  cout << "Graceful shutdown failed, trying power off directly..." << endl;

  if (pal_set_server_power(FRU_SERVER, SERVER_POWER_OFF) < 0) {
    cerr << "Failed to execute server power off." << endl;
    return -1;
  }

  if (wait_server_power_off(MAX_PWR_OFF_RETRY, FORCE_OFF_RETRY_DELAY) < 0) {
    cerr << "Failed to Power Off Server." << endl;
    return -1;
  }

  return 0;
}

static int attempt_me_recovery()
{
  int ret = -1;

  for (int retry = 0; retry < ME_RECOVERY_MAX_ATTEMPTS; retry++) {
    ret = bic_me_recovery(RECOVERY_MODE);
    if (ret == 0) {
      return 0;
    }

    syslog(LOG_WARNING, "Failed to enter ME Recovery Mode, attempt %d/%d", retry + 1, ME_RECOVERY_MAX_ATTEMPTS);

    if (retry < ME_RECOVERY_MAX_ATTEMPTS - 1) {
      sleep(ME_RECOVERY_RETRY_DELAY);
    }
  }

  return ret;
}
#endif

int BiosComponent::_update(const string& image, uint8_t opt) {
  int ret = 0;

  try {
    printf("[%s]Start bic_bios _update...\n", __func__);
    server.ready();
#ifdef CONFIG_GRANDCANYON2

    if (opt == FORCE_UPDATE_UNSET) {
      if (is_valid_image(image) == false) {
        cerr << "Invalid bios image. Stopping the update!" << endl;
        return FW_STATUS_FAILURE;
      }
    }

    if (attempt_server_power_off(opt == FORCE_UPDATE) < 0) {
      cerr << "Failed to Power Off Server. Stopping the update!"<< endl;
      return FW_STATUS_FAILURE;
    }

    cerr << "Server is powered off. Setting ME to recovery mode..." << endl;

    if (attempt_me_recovery() < 0) {
        if (opt == FORCE_UPDATE) {
          cerr << "Failed to set ME to recovery mode. Continuing the force update anyway..." << endl;
          syslog(LOG_CRIT, "Force BIOS update: Failed to enter ME Recovery Mode " "after %d attempts. Continuing the update.", ME_RECOVERY_MAX_ATTEMPTS);
        } else {
          cerr << "Failed to set ME to recovery mode. Stopping the update!" << endl;
          syslog(LOG_CRIT, "BIOS update: Failed to enter ME Recovery Mode after %d attempts. Stopping the update.", ME_RECOVERY_MAX_ATTEMPTS);
          ret = FW_STATUS_FAILURE;
          goto exit;
        }
    } else {
      sleep(3);
    }
#else
    uint8_t status = 0;
    int retry_count = 0;

    if (opt != FORCE_UPDATE) {
      cout << "Shutting down server gracefully..." << endl;
      pal_set_server_power(FRU_SERVER, SERVER_GRACEFUL_SHUTDOWN);

      //Checking Server Power Status to make sure Server is really Off
      while (retry_count < GRACEFUL_OFF_MAX_RETRY) {
        ret = pal_get_server_power(FRU_SERVER, &status);
        if ((ret == 0) && (status == SERVER_POWER_OFF)){
          break;
        } else {
          retry_count++;
          sleep(2);
        }
      }
      if (retry_count == GRACEFUL_OFF_MAX_RETRY) {
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
#endif

#ifdef CONFIG_GRANDCANYON2
    // OPENBIC don't need mux bios spi flash by FPGA, it can directly control it.
#else
    bic_switch_mux_for_bios_spi(MUX_SWITCH_FPGA);
#endif

    sleep(1);
    if (opt == DUMP_FW) {
      ret = bic_dump_bios_fw((char *)image.c_str());
    } else {
      ret = bic_update_fw(FRU_SERVER, fw_comp, (char *)image.c_str(), (opt == FORCE_UPDATE) ? true : false);
    }
#ifndef CONFIG_GRANDCANYON2
    if (ret != 0) {
      // recover to original setting
      bic_switch_mux_for_bios_spi(MUX_SWITCH_PCH);
      bic_me_recovery(RESTORE_FACTORY_DEFAULT);
    }
#endif
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

int BiosComponent::update(const string& image) {
  return _update(image, NORMAL_UPDATE);
}

int BiosComponent::fupdate(const string& image) {
  return _update(image, FORCE_UPDATE);
}

int BiosComponent::dump(const string& image) {
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

bool BiosComponent::is_valid_image(const std::string& image) {
  uint8_t board_rev_id = 0xFF;
  bool ret = false;

  if (get_server_board_revision_id(&board_rev_id, sizeof(board_rev_id)) < 0) {
    syslog(LOG_WARNING, "%s() failed to get server board revision id", __func__);
    return false;
  }

  if (fbgc_common_validate_img(image.c_str(), FW_BIOS, BOARD_ID_SB, board_rev_id) == 0) {
    ret = true;
  }
  return ret;
}
#endif
