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
  uint8_t nvme_ready = 0, status = 0, ffi = 0, meff = 0, major_ver = 0, minor_ver = 0;
  uint8_t idx = (fw_comp - FW_2OU_M2_DEV0) + 1;
  uint16_t vendor_id = 0;
  static bool is_dual_m2 = false;
  //static uint8_t cnt = 0;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    ret = bic_get_dev_power_status(slot_id, idx, &nvme_ready, &status, \
                                   &ffi, &meff, &vendor_id, &major_ver,&minor_ver, REXP_BIC_INTF);
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
    
    if ( is_dual_m2 == true ) printf("%s DEV%d/%d Version: v%d.%d\n", board_name.c_str(), idx, idx + 1, major_ver, minor_ver);
    else printf("%s DEV%d Version: v%d.%d\n", board_name.c_str(), idx, major_ver, minor_ver);
  } catch(string& err) {
    printf("%s DEV%d Version: NA (%s)\n", board_name.c_str(), idx, err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

int M2DevComponent::update(string image)
{
  int ret = 0;
  uint8_t nvme_ready = 0, status = 0, ffi = 0, meff = 0, major_ver = 0, minor_ver = 0;
  uint8_t idx = (fw_comp - FW_2OU_M2_DEV0) + 1;
  uint16_t vendor_id = 0;

  try {
    server.ready();
    expansion.ready();
    ret = bic_get_dev_power_status(slot_id, idx, &nvme_ready, &status, \
                                   &ffi, &meff, &vendor_id, &major_ver,&minor_ver, REXP_BIC_INTF);
    if ( ret < 0 ) {
      throw string("Error in getting the m2 version");
    }
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET); 
  } catch (string& err) {
    printf("Failed reason: %s\n", err.c_str()); 
  }

  return ret;
}
#endif
