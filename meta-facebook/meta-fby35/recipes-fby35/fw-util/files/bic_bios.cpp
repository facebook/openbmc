#include <cstdio>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/misc-utils.h>
#include <openbmc/pal.h>
#include "bic_bios.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define FORCED_OFF 1
#define DELAY_ME_RESET 10

int attempt_server_power_off(uint8_t slot, bool force) {
  uint8_t status = SERVER_POWER_ON;

  if (!pal_get_server_power(slot, &status) && status == SERVER_POWER_OFF) {
    return 0;
  }

  if (!force) {
    cerr << "Powering off the server gracefully" << endl;
    pal_set_server_power(slot, SERVER_GRACEFUL_SHUTDOWN);
    if (retry_cond(!pal_get_server_power(slot, &status) && status == SERVER_POWER_OFF, 60, 1000)) {
      cerr << "Retry force powering off the server..." << endl;
      pal_set_server_power(slot, SERVER_POWER_OFF);
      if (retry_cond(!pal_get_server_power(slot, &status) && status == SERVER_POWER_OFF, 1, 1000)) {
        return -1;
      }
      return FORCED_OFF;
    }
  } else {
    cerr << "Force powering off the server" << endl;
    pal_set_server_power(slot, SERVER_POWER_OFF);
    if (retry_cond(!pal_get_server_power(slot, &status) && status == SERVER_POWER_OFF, 1, 1000)) {
      return -1;
    }
    return FORCED_OFF;
  }

  return 0;
}

image_info BiosComponent::check_image(const string& image, bool force) {
  int ret = 0;
  uint8_t board_id = 0, board_rev = 0;
  image_info image_sts = {"", false, false};

  if (force == true) {
    image_sts.result = true;
  }

  if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_HD) {
    board_id = BOARD_ID_HD;
  } else {
    board_id = BOARD_ID_SB;
  }
  ret = get_board_rev(slot_id, BOARD_ID_SB, &board_rev);
  if (ret < 0) {
    cerr << "Failed to get board revision ID" << endl;
    return image_sts;
  }

  if (fby35_common_is_valid_img(image.c_str(), fw_comp, board_id, board_rev) == true) {
    image_sts.result = true;
    image_sts.sign = true;
  }

  return image_sts;
}

bool BiosComponent::checkPfrUpdate(prot::ProtDevice& prot_dev) {
  if (specificSpi) {
      return false;
  }
    cerr << "PRoT is PFR mode, update to recovery Flash" << endl;
    prot::BOOT_STATUS_ACK_PAYLOAD boot_sts{};
    if (prot_dev.portGetBootStatus(boot_sts) == prot::ProtDevice::DevStatus::SUCCESS) {
      prot::ProtSpiInfo::updateProperty upateProp(boot_sts, FW_BIOS, FW_BIOS_SPIB);
      fw_comp = upateProp.target;
      return upateProp.isPfrUpdate;
    } else {
      throw runtime_error("Error in getting XFR Boot Status");
    }
}

int BiosComponent::update_internal(const std::string& image, int fd, bool force) {
  int ret;
  int ret_recovery = 0, ret_reset = 0, ret_pwroff = 0;
  int server_type = SERVER_TYPE_NONE;
  bool pfr_update = false;
  uint8_t dev_i2c_bus = 0;
  uint8_t dev_i2c_addr = 0;
  if (pal_get_prot_address(slot_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    cerr << "pal_get_prot_address failed, fru: " << slot_id << std::endl;
    return FW_STATUS_FAILURE;
  }
  prot::ProtDevice prot_dev(slot_id, dev_i2c_bus, dev_i2c_addr);
  if (!prot_dev.isDevOpen()) {
    return FW_STATUS_FAILURE;
  }

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

  image_info image_sts = check_image(image, force);
  if (image_sts.result == false) {
    syslog(LOG_CRIT, "Update %s on %s Fail. File: %s is not a valid image",
           get_component_name(fw_comp), fru().c_str(), image.c_str());
    return FW_STATUS_FAILURE;
  }

  server_type = fby35_common_get_slot_type(slot_id);
  if (server_type < 0) {
    syslog(LOG_WARNING, "%s() Error while getting slot%d type: %d", __func__, slot_id, server_type);
    return FW_STATUS_FAILURE;
  }

  if (SERVER_TYPE_HD != server_type || isBypass) {
    if ((ret_pwroff = attempt_server_power_off(slot_id, force)) < 0) {
      cerr << "Failed to Power Off Server " << slot_id << ". Stopping the update!" << endl;
      return FW_STATUS_FAILURE;
    }
  } else {
    try {
      pfr_update = checkPfrUpdate(prot_dev);
    } catch(const runtime_error& err) {
      cerr << "XFR Error: "<< err.what() <<  endl;
      return FW_STATUS_FAILURE;
    }

    if (pfr_update) {
      if (prot_dev.portRequestRecoverySpiUnlock() !=  prot::ProtDevice::DevStatus::SUCCESS) {
        return FW_STATUS_FAILURE;
      }
    }
  }

  if (server_type == SERVER_TYPE_CL) {
    if (ret_pwroff == FORCED_OFF) {
      sleep(DELAY_ME_RESET);  // to wait for ME reset
    }
    cerr << "Putting ME into recovery mode..." << endl;
    ret_recovery = me_recovery(slot_id, RECOVERY_MODE);

    // If you are doing force update, it will keep executing
    // whether it has successfully entered recovery mode
    if ((ret_recovery < 0) && (force == false)) {
      cerr << "Failed to put ME into recovery mode. Stopping the update!" << endl;
      pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
      return ret_recovery;
    }
    sleep(1);
  }

  if (!image.empty()) {
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), force);
  } else {
    ret = bic_update_fw_fd(slot_id, fw_comp, fd, force);
  }

  if (ret != 0) {
    cerr << "BIOS update failed. ret = " << ret << endl;
  }
  sleep(1);

  if (server_type == SERVER_TYPE_CL) {
    // Have to do ME reset, because we have put ME into recovery mode
    cerr << "Doing ME Reset..." << endl;
    ret_reset = me_reset(slot_id);
    sleep(DELAY_ME_RESET);
  }
  if (SERVER_TYPE_HD != server_type || isBypass) {
    cerr << "Power-cycling the server..." << endl;

    // 12V-cycle is necessary for concerning BIOS/ME crash case
    if (ret_reset || ret_recovery) {
      syslog(LOG_CRIT, "Server 12V cycle due to %s failed when BIOS update",
             ret_recovery ? "putting ME into recovery mode" : "ME reset");
      pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    } else {
      pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
    }
  } else {
    if (pfr_update) {
      uint8_t status;

      cerr << "Sending XFR UPDATE_COMPLTE CMD..."<< endl;
      prot_dev.protUpdateComplete();
      cerr << "Sending XFR FW_UPDATE_INTENT..."<< endl;
      prot_dev.protFwUpdateIntent();
      if (prot_dev.protGetUpdateStatus(status) != prot::ProtDevice::DevStatus::SUCCESS) {
        cerr << "Failed to get BIOS PFR update status"<< endl;
        return FW_STATUS_FAILURE;
      }
      cerr << "XFR update status : "<< prot::ProtUpdateStatus::updateStatusString(status) << endl;
      if (status) {
        cerr << "BIOS PFR update failed"<< endl;
        return FW_STATUS_FAILURE;
      }
    }
  }

  return ret;
}

int BiosComponent::update(const string image) {
  return update_internal(image, -1, false /* force */);
}

int BiosComponent::update(int fd, bool force) {
  return update_internal("", fd, force);
}

int BiosComponent::fupdate(const string image) {
  return update_internal(image, -1, true /* force */);
}

int BiosComponent::dump(string image) {
  int ret;
  int server_type = 0;
  int ret_recovery = 0, ret_reset = 0;

  try {
    cerr << "Checking if the server is ready..." << endl;
    server.ready_except();
  } catch(const std::exception& err) {
    cerr << "Server is not ready." << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  if (attempt_server_power_off(slot_id, true) < 0) {
    cerr << "Failed to Power Off Server " << slot_id << ". Stopping the dump!" << endl;
    return FW_STATUS_FAILURE;
  }

  server_type = fby35_common_get_slot_type(slot_id);
  if (server_type == SERVER_TYPE_CL) {
    sleep(DELAY_ME_RESET);
    cerr << "Putting ME into recovery mode..." << endl;
    ret_recovery = me_recovery(slot_id, RECOVERY_MODE);
    if (ret_recovery < 0) {
      cerr << "Failed to put ME into recovery mode." << endl;
    }
    sleep(1);
  }

  ret = dump_bic_usb_bios(slot_id, fw_comp, (char *)image.c_str());
  if (ret != 0) {
    cerr << "BIOS dump failed. ret = " << ret << endl;
  }
  sleep(1);

  if (server_type == SERVER_TYPE_CL) {
    // Have to do ME reset, because we have put ME into recovery mode
    cerr << "Doing ME Reset..." << endl;
    ret_reset = me_reset(slot_id);
    sleep(DELAY_ME_RESET);
  }

  cerr << "Power-cycling the server..." << endl;

  // 12V-cycle is necessary for concerning BIOS/ME crash case
  if (ret_reset || ret_recovery) {
    syslog(LOG_CRIT, "Server 12V cycle due to %s failed when BIOS dump",
           ret_recovery ? "putting ME into recovery mode" : "ME reset");
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
  } else {
    pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
  }

  return ret;
}

int BiosComponent::get_ver_str(string& s) {
  uint8_t ver[MAX_SIZE_SYSFW_VER] = {0};
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
  uint8_t str[MAX_SIZE_SYSFW_VER] = {0};

  if (specificSpi) {
    //only for update/dump
    return FW_STATUS_NOT_SUPPORTED;
  }

  try {
    server.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw std::runtime_error("Error in getting the version of BIOS");
    }
    cout << "BIOS Version: " << ver << endl;
    if (pal_get_delay_activate_sysfw_ver(slot_id, str) == 0) {
      stringstream ss;
      ss << str;
      string delay_ver = ss.str();
      cout << "BIOS Version after activation: " << delay_ver << endl;
    } else { // no update before
      cout << "BIOS Version after activation: " << ver << endl;
    }
  } catch(string& err) {
    printf("BIOS Version: NA (%s)\n", err.c_str());
  }

  return FW_STATUS_SUCCESS;
}


int BiosComponent::get_version(json& j) {
  string ver("");

  if (specificSpi) {
    //only for update/dump
    return FW_STATUS_NOT_SUPPORTED;
  }

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
