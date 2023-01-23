#include <cstdio>
#include <syslog.h>
#include <openbmc/pal.h>
#include "bic_m2_dev.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define FFI_0_ACCELERATOR 0x01
int M2DevComponent::get_ver_str(string&, const uint8_t) {
  return FW_STATUS_NOT_SUPPORTED;
}

int M2DevComponent::print_version()
{
  string ver("");
  string board_name = name;
  string err_msg("");
  int ret = 0;
  uint8_t nvme_ready = 0, status = 0, ffi = 0, meff = 0, major_ver = 0, minor_ver = 0;
  uint8_t idx = (fw_comp - FW_2OU_M2_DEV0) + 1;
  uint16_t vendor_id = 0;
  static bool is_dual_m2 = false;
  //static uint8_t cnt = 0;
  int config_status = 0;
  uint8_t board_type = UNKNOWN_BOARD;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    ret = pal_get_board_type(slot_id, &config_status, &board_type);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s() get board config fail", __func__);
      return ret;
    }
    ret = bic_get_dev_power_status(slot_id, idx, &nvme_ready, &status, \
                                   &ffi, &meff, &vendor_id, &major_ver,&minor_ver, REXP_BIC_INTF, board_type);
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
    else printf("%s DEV%d Version: v%d.%d\n", board_name.c_str(), idx - 1, major_ver, minor_ver);
  } catch(string& err) {
    printf("%s DEV%d Version: NA (%s)\n", board_name.c_str(), idx - 1, err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

int M2DevComponent::update(string image)
{
  int ret = 0;
  uint8_t nvme_ready = 0, status = 0, type = DEV_TYPE_UNKNOWN;
  uint8_t idx = (fw_comp - FW_2OU_M2_DEV0) + 1;

  try {
    server.ready();
    expansion.ready();
    ret = pal_get_dev_info(slot_id, idx, &nvme_ready ,&status, &type);
    if (!ret) {
      if (status != 0) {
        syslog(LOG_WARNING, "update: Slot%u Dev%d power=%u nvme_ready=%u type=%u", slot_id, idx - 1, status, nvme_ready, type);
        if ( nvme_ready == 0 ) {
          ret = FW_STATUS_NOT_SUPPORTED;
          throw string("The m2 device's NVMe is not ready");
        } else if ( type == DEV_TYPE_BRCM_ACC ) {
          ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
          if (ret == FW_STATUS_SUCCESS) {
            pal_set_device_power(slot_id, DEV_ID0_2OU + idx -1, SERVER_POWER_CYCLE);
          } else {
            syslog(LOG_WARNING, "update: Slot%u Dev%d brcm vk failed", slot_id, idx - 1);
          }
        } else if ( type == DEV_TYPE_SPH_ACC ) {
          ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
        } else {
          ret = FW_STATUS_NOT_SUPPORTED;
          throw string("The m2 device does not support update");
        }
      } else {
        ret = FW_STATUS_NOT_SUPPORTED;
        throw string("The m2 device is not present");
      }
    } else {
      ret = FW_STATUS_NOT_SUPPORTED;
      throw string("Error in getting the m2 device info");
    }
  } catch (string& err) {
    printf("Failed reason: %s\n", err.c_str());
  }

  return ret;
}
#endif
