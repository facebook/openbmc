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

M2DevComponent::PowerInfo M2DevComponent::statusTable[MAX_DEVICE_NUM] = {0};
bool M2DevComponent::isDual[MAX_NUM_SERVER_FRU] = {false};
bool M2DevComponent::isScaned[MAX_NUM_SERVER_FRU] = {false};
M2DevComponent::Dev_Main_Slot M2DevComponent::dev_main_slot = M2DevComponent::Dev_Main_Slot::ON_EVEN;

int M2DevComponent::get_ver_str(string& s, const uint8_t alt_fw_comp) {
  return FW_STATUS_NOT_SUPPORTED;
}

void M2DevComponent::scan_all_devices(uint8_t intf, M2_DEV_INFO *m2_dev_info) {
  int ret = 0;
  uint8_t retry = MAX_READ_RETRY;

  if (isScaned[slot_id] == true) {
    return;
  }

  for (int idx = 0; idx < MAX_DEVICE_NUM; idx ++) {
    while (retry) {
      ret = bic_get_dev_power_status(slot_id, idx + 1, m2_dev_info, intf);
      if (!ret) {
        break;
      }
      msleep(50);
      retry--;
    }
    save_info(idx, ret, m2_dev_info);
    if (isDual[slot_id] == false && m2_dev_info->meff == MEFF_DUAL_M2) {
      isDual[slot_id] = true;
      if (idx % 2 == 0) {
        dev_main_slot = Dev_Main_Slot::ON_EVEN;
      } else {
        dev_main_slot = Dev_Main_Slot::ON_ODD;
      }
    }
  }
  isScaned[slot_id] = true;
}

void M2DevComponent::save_info(uint8_t idx, int ret, M2_DEV_INFO *m2_dev_info) {

  if (idx >= MAX_DEVICE_NUM) {
    return;
  }

  statusTable[idx].ret = ret;
  statusTable[idx].status = m2_dev_info->status;
  statusTable[idx].nvme_ready = m2_dev_info->nvme_ready;
  statusTable[idx].ffi = m2_dev_info->ffi;
  statusTable[idx].major_ver = m2_dev_info->major_ver;
  statusTable[idx].minor_ver = m2_dev_info->minor_ver;
  statusTable[idx].additional_ver = m2_dev_info->additional_ver;
  statusTable[idx].sec_major_ver = m2_dev_info->sec_major_ver;
  statusTable[idx].sec_minor_ver = m2_dev_info->sec_minor_ver;
  statusTable[idx].is_freya = m2_dev_info->is_freya;
}

int M2DevComponent::print_version()
{
  string ver("");
  string board_name = name;
  string err_msg("");
  uint8_t idx = (fw_comp - FW_2OU_M2_DEV0);
  uint8_t intf = REXP_BIC_INTF;
  M2_DEV_INFO m2_dev_info = {};
  //static uint8_t cnt = 0;

  if ( fw_comp >= FW_TOP_M2_DEV0 && fw_comp <= FW_TOP_M2_DEV11 ) {
    idx = (fw_comp - FW_TOP_M2_DEV0);
    intf = RREXP_BIC_INTF1;
  } else if ( fw_comp >= FW_BOT_M2_DEV0 && fw_comp <= FW_BOT_M2_DEV11 ) {
    idx = (fw_comp - FW_BOT_M2_DEV0);
    intf = RREXP_BIC_INTF2;
  }

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {

    server.ready();
    expansion.ready();
    scan_all_devices(intf, &m2_dev_info);

    if (isDual[slot_id] == true) {
      print_dual(idx, m2_dev_info);
    } else {
      print_single(idx);
    }
  } catch(string& err) {
    printf("%s DEV%d Version: NA (%s)\n", board_name.c_str(), idx, err.c_str());
  }
  if ( idx+1 == MAX_DEVICE_NUM ) {
    isScaned[slot_id] = false;
  }
  return FW_STATUS_SUCCESS;
}

void M2DevComponent::print_dual(uint8_t idx, M2_DEV_INFO m2_dev_info) {
  int first_dev, second_dev, main_dev;
  string board_name = name;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);

  if (idx % 2 == 0) {
    first_dev = idx;
    second_dev = idx + 1;
    statusTable[first_dev].isPrinted = true;
  } else {
    first_dev = idx - 1;
    second_dev = idx;
    if (statusTable[first_dev].isPrinted == true) {
      return;
    }
  }
  if (dev_main_slot == Dev_Main_Slot::ON_EVEN) {
    main_dev = first_dev;
  }
  else {
    main_dev = second_dev;
  }

  printf("%s DEV%d/%d Version: ", board_name.c_str(), first_dev, second_dev);
  if (statusTable[main_dev].ret) {
    printf("NA\n");
  } else if (!statusTable[main_dev].status) {
    printf("NA(Not Present)\n");
  } else if (!statusTable[main_dev].nvme_ready) {
    printf("NA(NVMe Not Ready)\n");
  } else if (statusTable[main_dev].ffi != FFI_0_ACCELERATOR) {
    printf("NA(Not Accelerator)\n");
  } else {
    // Freya version format meaning: major, minor, PSOC major, PSOC minor, QSPI
    if ( statusTable[main_dev].is_freya ) {
      printf("v%d.%d.%d.%d.%d\n", statusTable[main_dev].major_ver, statusTable[main_dev].minor_ver, \
                                    statusTable[main_dev].additional_ver, statusTable[main_dev].sec_major_ver, \
                                      statusTable[main_dev].sec_minor_ver);
    } else {
      printf("v%d.%d\n", statusTable[main_dev].major_ver, statusTable[main_dev].minor_ver);
    }
  }
}

void M2DevComponent::print_single(uint8_t idx) {
  string board_name = name;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);

  printf("%s DEV%d Version: ", board_name.c_str(), idx);
  if (statusTable[idx].ret) {
    printf("NA\n");
  } else if (!statusTable[idx].status) {
    printf("NA(Not Present)\n");
  } else if (!statusTable[idx].nvme_ready) {
    printf("NA(NVMe Not Ready)\n");
  } else if (statusTable[idx].ffi != FFI_0_ACCELERATOR) {
    printf("NA(Not Accelerator)\n");
  } else {
    printf("v%d.%d.%d\n", statusTable[idx].major_ver, statusTable[idx].minor_ver, statusTable[idx].additional_ver);
  }
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
