#include <cstdio>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include "bic_cxl.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
#include <openbmc/pal.h>

extern int attempt_server_power_off(uint8_t slot, bool force);

using namespace std;

constexpr auto P1V8_ASIC_EN_R = 0x11;


image_info CxlComponent::check_image(const string& image, bool force) {
  image_info image_sts = {"", false, false};

  if (force == true) {
    image_sts.result = true;
  }

  if (fby35_common_is_valid_img(image.c_str(), fw_comp, BOARD_ID_RF, 0) == true) {
    image_sts.result = true;
    image_sts.sign = true;
  }

  return image_sts;
}

int CxlComponent::update_internal(const std::string& image, int fd, bool force) {
  int ret;

  try {
    cerr << "Checking if the server is ready..." << endl;
    server.ready_except();
    expansion.ready();
  } catch(const std::exception& err) {
    cout << err.what() << endl;
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

  if (attempt_server_power_off(slot_id, force) < 0) {
    cerr << "Failed to Power Off Server " << slot_id << ". Stopping the update!" << endl;
    return FW_STATUS_FAILURE;
  }

  sleep(3);  //Avoid start update during CXL power off sequence
  if (!image.empty()) {
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), force);
  } else {
    ret = bic_update_fw_fd(slot_id, fw_comp, fd, force);
  }

  //Disable RBF 1V8
  remote_bic_set_gpio(slot_id, P1V8_ASIC_EN_R, 0, FEXP_BIC_INTF);

  if (ret != 0) {
    cerr << "CXL update failed. ret = " << ret << endl;
  }
  sleep(1);

  cerr << "Power-cycling the server..." << endl;
  pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
  return ret;
}

int CxlComponent::update(const string image) {
  return update_internal(image, -1, false /* force */);
}

int CxlComponent::update(int fd, bool force) {
  return update_internal("", fd, force);
}

int CxlComponent::fupdate(const string image) {
  return update_internal(image, -1, true /* force */);
}

int CxlComponent::get_ver_str(string& s) {
  uint8_t ver[32] = {0};

  if (bic_get_fw_ver(slot_id, fw_comp, ver)) {
    return FW_STATUS_FAILURE;
  }

  s = string(reinterpret_cast<const char*>(ver));
  return FW_STATUS_SUCCESS;
}

int CxlComponent::print_version() {
  string ver("");
  try {
    server.ready_except();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw runtime_error("Error in getting the version of CXL");
    }
    cout << "CXL Version: " << ver << endl;
  } catch(const runtime_error& err) {
    cout << "CXL Version: NA (" << err.what() << ")" << endl;
  }

  return FW_STATUS_SUCCESS;
}

int CxlComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready_except();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw runtime_error("Error in getting the version of CXL");
    }
    j["VERSION"] = ver;
  } catch(const runtime_error& err) {
    if ( string(err.what()).find("empty") != string::npos )
      j["VERSION"] = "not_present";
    else
      j["VERSION"] = "error_returned";
  }

  return FW_STATUS_SUCCESS;
}

#endif
