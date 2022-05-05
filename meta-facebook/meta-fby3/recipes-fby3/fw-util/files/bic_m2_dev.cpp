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
#include "bic_m2_dev.h"
#include <openbmc/kv.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
#include <openbmc/pal.h>
using namespace std;

#define FFI_0_ACCELERATOR 0x01
int M2DevComponent::get_ver_str(string& s, const uint8_t alt_fw_comp) {
  return FW_STATUS_NOT_SUPPORTED;
}

void M2DevComponent::get_version(json& j) {
  return;
}

int M2DevComponent::print_version()
{
  string ver("");
  string board_name = name;
  string err_msg("");
  int ret = 0;
  uint8_t nvme_ready = 0, status = 0, ffi = 0, meff = 0, major_ver = 0, minor_ver = 0,additional_ver = 0;
  uint8_t idx = (fw_comp - FW_2OU_M2_DEV0) + 1;
  uint8_t intf = REXP_BIC_INTF;
  uint16_t vendor_id = 0;
  static bool is_dual_m2 = false;
  //static uint8_t cnt = 0;

  if ( fw_comp >= FW_TOP_M2_DEV0 && fw_comp <= FW_TOP_M2_DEV11 ) {
    idx = (fw_comp - FW_TOP_M2_DEV0) + 1;
    intf = RREXP_BIC_INTF1;
  } else if ( fw_comp >= FW_BOT_M2_DEV0 && fw_comp <= FW_BOT_M2_DEV11 ) {
    idx = (fw_comp - FW_BOT_M2_DEV0) + 1;
    intf = RREXP_BIC_INTF2;
  }

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();

    ret = bic_get_dev_power_status(slot_id, idx, &nvme_ready, &status, \
                                  &ffi, &meff, &vendor_id, &major_ver,&minor_ver,&additional_ver, intf);

    if ( ret < 0 ) {
      throw string("Error in getting the version");
    } else if ( idx % 2 > 0 ) {
      is_dual_m2 = (meff == MEFF_DUAL_M2)?true:false;
    }

    //cnt++; //it's used to count how many devices have been checked.

    // it's dual m2, then return since it's printed
    if ( is_dual_m2 == true && (idx % 2) == 0 ) {
      is_dual_m2 = false;
      return FW_STATUS_SUCCESS;
    } else if ( status == 0 ) {
      throw string("Not Present");
    } else if ( nvme_ready == 0 ) {
      throw string("NVMe Not Ready");
    } else if ( ffi != FFI_0_ACCELERATOR ) {
      throw string("Not Accelerator");
    }
    
    if ( is_dual_m2 == true ) printf("%s DEV%d/%d Version: v%d.%d\n", board_name.c_str(), idx - 1, idx, major_ver, minor_ver);
    else printf("%s DEV%d Version: v%d.%d.%d\n", board_name.c_str(), idx - 1, major_ver, minor_ver,additional_ver);
  } catch(string& err) {
    printf("%s DEV%d Version: NA (%s)\n", board_name.c_str(), idx - 1, err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

int M2DevComponent::update_internal(string image, bool force) {
  int ret = FW_STATUS_SUCCESS;
  uint8_t nvme_ready = 0, status = 0, type = DEV_TYPE_UNKNOWN;
  uint8_t idx = (fw_comp - FW_2OU_M2_DEV0) + 1;
  uint8_t bmc_location = 0;
  uint8_t fru = slot_id;

  if ( fw_comp >= FW_TOP_M2_DEV0 && fw_comp <= FW_TOP_M2_DEV11 ) {
    idx = (fw_comp - FW_TOP_M2_DEV0) + 1;
    fru = FRU_2U_TOP;
  } else if ( fw_comp >= FW_BOT_M2_DEV0 && fw_comp <= FW_BOT_M2_DEV11 ) {
    idx = (fw_comp - FW_BOT_M2_DEV0) + 1;
    fru = FRU_2U_BOT;
  }

  try {
    // get BMC location
    ret = fby3_common_get_bmc_location(&bmc_location);
    if ( ret < 0 ) {
      throw string("Failed to get BMC's location");
    }

    // check the status of the board
    server.ready();
    expansion.ready();

    // need to make sure M2 exists
    ret = pal_get_dev_info(fru, idx, &nvme_ready ,&status, &type);
    if ( ret < 0 ) {
      throw string("Error in getting the m2 device info");
    }

    syslog(LOG_WARNING, "update: Slot%u Dev%d power=%u nvme_ready=%u type=%u", slot_id, idx - 1, status, nvme_ready, type);

    // 0 means the device is not detected, we should return
    if ( status == 0 ) {
      throw string("The m2 device is not present");
    }

    // nvme type - SPH/BRCM
    // nvme status is 0 which means the device is not ready
    // tooling team doesn’t care that it’s recovery or normal update, it expects a unified syntax
    // remove `--force` flag here
    if ( bmc_location != NIC_BMC ) {
      // BRCM VK recovery update
      cerr << "BRCM VK Recovery Update" << endl;
      ret = bic_disable_brcm_parity_init(slot_id, fw_comp);
      if ( ret < 0 ) throw string("Failed to disable BRCM parity init");
    }

    // no matter which one is used, bic_update_fw will be called in the end
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
    if ( ret < 0 ) {
      syslog(LOG_WARNING, "update: Slot%u Dev%d failed", slot_id, idx - 1);
      throw string("Failed to access the device");
    }

    // run dev power cycle for BRCM VK
    if ( bmc_location != NIC_BMC ) {
      ret = pal_set_device_power(slot_id, DEV_ID0_2OU + idx - 1, SERVER_POWER_CYCLE);
      if ( ret < 0 ) throw string("Failed to do dev power cycle");
    }
  } catch (string& err) {
    ret = FW_STATUS_FAILURE;
    printf("Failed reason: %s\n", err.c_str());
  }

  return ret;
}

int M2DevComponent::fupdate(string image) {
  return update_internal(image, true);
}

int M2DevComponent::update(string image) {
  return update_internal(image, false);
}
#endif
