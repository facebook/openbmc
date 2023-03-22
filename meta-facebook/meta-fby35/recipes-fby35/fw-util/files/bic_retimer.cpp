#include <cstdio>
#include <iomanip>
#include <sstream>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include <facebook/fby35_common.h>
#include <facebook/bic.h>
#include <facebook/bic_ipmi.h>
#include "bic_retimer.h"

using namespace std;

int RetimerFwComponent::update_internal(const std::string& image, bool force) {
  int ret = 0;

  try {
    cout << "Checking if the server is ready..." << endl;
    server.ready_except();
    expansion.ready();
  } catch(const std::exception& err) {
    cout << err.what() << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  if (image.empty()) {
    cout << "Image path is required." << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), force);
  if (ret != 0) {
    cerr << "Retimer update failed. ret = " << ret << endl;
  }
  sleep(1); // wait for BIC to complete update process

  cout << "Power-cycling the server..." << endl;
  pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
  return ret;
}

int RetimerFwComponent::update(const string image) {
  return update_internal(image, false /* force */);
}

int RetimerFwComponent::fupdate(const string image) {
  return update_internal(image, true /* force */);
}

/*
  Byte 0: Major version
  Byte 1: Minor version
  Byte 2: Build number (MSB)
  Byte 3: Build number (LSB)
*/
int RetimerFwComponent::get_ver_str(string& s) {
  int ret = 0;
  uint8_t rbuf[32] = {0};
  uint16_t build_num = 0;
  ret = bic_get_fw_ver(slot_id, fw_comp, rbuf);
  if (!ret) {
    stringstream ver;
    build_num = (rbuf[2] << 8) | rbuf[3];
    ver << "v" << (int)rbuf[0] << "_" << (int)rbuf[1] << "_" << build_num;
    s = ver.str();
  }
  return ret;
}

int RetimerFwComponent::print_version() {
  string ver("");
  string board_name = board;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the retimer version of " + board_name;
    }
    cout << board_name << " Retimer Version: " << ver << endl;
  } catch (string& err) {
    cout << board_name + " Retimer Version: NA (" + err + ")" << endl;
  }

  return FW_STATUS_SUCCESS;
}

int RetimerFwComponent::get_version(json& j) {
  string ver("");
  string board_name = board;

  try {
    server.ready();
    expansion.ready();
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the retimer version of " + board_name;
    }
    j["VERSION"] = ver;
  } catch (string& err) {
    if (err.find("empty") != string::npos) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
    }
  }
  return FW_STATUS_SUCCESS;
}
