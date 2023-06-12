#include <cstdio>
#include <iomanip>
#include <sstream>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <algorithm>
#include <iterator>
#include <openbmc/pal.h>
#include <facebook/fby35_common.h>
#include <facebook/bic.h>
#include <facebook/bic_ipmi.h>
#include "bic_retimer.h"

using namespace std;

static constexpr uint8_t retimer_bus = 3;
static constexpr uint8_t retimer_addr = 0x46;
static constexpr uint8_t vendor_id_size = 7;

const uint8_t astera_vendor_id[vendor_id_size] = {0x06, 0x04, 0x00, 0x01, 0x00, 0xFA, 0x1D};
const uint8_t montage_vendor_id[vendor_id_size] = {0x06, 0x04, 0x00, 0x40, 0x20, 0x00, 0xD8};

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

int RetimerFwComponent::get_vendor_id(VendorID* vendor_id) {
  uint8_t intf = 0;
  uint8_t bus = 0;
  uint8_t tbuf[64] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 0, rlen = 0;

  if (!vendor_id) {
    return -1;
  }
  if (fw_comp == FW_1OU_RETIMER) {
    intf = FEXP_BIC_INTF;
  } else {
    intf = EXP3_BIC_INTF;
  }
  if (bic_disable_sensor_monitor(slot_id, 0, intf) < 0) { //disable
    cerr << "Failed to disable sensor monitor" << endl;
    return -1;
  }
  bus = (retimer_bus << 1) | 1;

  tbuf[0] = bus;
  tbuf[1] = retimer_addr;
  tbuf[2] = 0;
  tbuf[3] = 0x02; //COMMAND CODE = 0x02 END=0, START=1, FUNC=3'b000, PEC=0
  tbuf[4] = 0x02; //byte count
  tbuf[5] = 0x04; //vendor id lower offset
  tbuf[6] = 0x00; //vendor id upper offset
  tlen = 7;
  if (bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen,
    rbuf, &rlen, intf) < 0) {
    cerr << "Failed to set RETIMER vendor ID offset" << endl;
  }

  tbuf[2] = vendor_id_size;
  tbuf[3] = 0x01; //COMMAND CODE = 0x01 END=1, START=0, FUNC=3'b000, PEC=0
  tlen = 4;
  if (bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen,
    rbuf, &rlen, intf) < 0) {
    cerr << "Failed to read RETIMER vendor id" << endl;
  }
  if (equal(rbuf, rbuf + vendor_id_size, astera_vendor_id)) {
    *vendor_id = VendorID::ASTERA_LABS;
  } else if (equal(rbuf, rbuf + vendor_id_size, montage_vendor_id)) {
    *vendor_id = VendorID::MONTAGE;
  } else {
    *vendor_id = VendorID::UNKNOWN_VENDOR;
  }
  bic_disable_sensor_monitor(slot_id, 1, intf); //enable
  return 0;
}

int RetimerFwComponent::get_ver_str(string& s) {
  int ret = 0;
  uint8_t rbuf[32] = {0};
  VendorID vendor_id = VendorID::UNKNOWN_VENDOR;
  uint16_t build_num = 0;

  get_vendor_id(&vendor_id);
  ret = bic_get_fw_ver(slot_id, fw_comp, rbuf);
  if (!ret) {
    stringstream ver;
    switch (vendor_id) {
      case VendorID::ASTERA_LABS:
        /*
          Byte 0: Major version
          Byte 1: Minor version
          Byte 2: Build number (MSB)
          Byte 3: Build number (LSB)
        */
        build_num = (rbuf[2] << 8) | rbuf[3];
        ver << "Astera Labs v" << (int)rbuf[0] << "_" << (int)rbuf[1] << "_" << build_num;
        break;
      case VendorID::MONTAGE:
        ver << "Montage " << setfill('0') << setw(2) << hex << (int)rbuf[0]
        << setfill('0') << setw(2) << hex << (int)rbuf[1]
        << setfill('0') << setw(2) << hex << (int)rbuf[2]
        << setfill('0') << setw(2) << hex << (int)rbuf[3];
        break;
      default:
        cerr << "Not supported vendor ID" << endl;
        return -1;
    }
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
