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
#include "bic_vr.h"
#include <openbmc/kv.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>
#include <openbmc/pal.h>
using namespace std;

                           // addr, VR name
map<uint8_t, string> list = {{0xC0, "VCCIN_VSA"},
                             {0xC4, "VCCIO"},
                             {0xC8, "VDDQ_AB"},
                             {0xCC, "VDDQ_DE"}};

int VrComponent::get_ver_str(uint8_t& addr, string& s) {
  int ret = 0;
  constexpr auto bus = 0x8;
  char ver_str[MAX_VALUE_LEN] = {0};

  ret = bic_get_vr_ver_cache(bus, addr, ver_str);
  s = string(ver_str);

  return ret;
}

int VrComponent::get_version(json& j) {
  uint8_t addr = 0;
  size_t start = 0;
  string str("");
  bool is_TI_VR = false;

  for (auto& vr:list) {
    try {
      server.ready();
      addr = vr.first;
      if (get_ver_str(addr, str) < 0) {
        throw "Error in getting the version of " + vr.second;
      }
      //For IFX and RNS, the str is $vendor $ver, Remaining Writes: $times
      //For TI, the str is $vendor_token1 $vendor_token2 $ver
      string tmp_str("");
      start = 0;
      auto end = str.find(',');
      is_TI_VR = false;

      if (end == string::npos) {
        is_TI_VR = true;
        start = str.rfind(' ');
        end = str.size();
      } else {
        start = str.find(' ');
      }

      tmp_str = str.substr(0, start);
      transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
      j["VERSION"][vr.second]["vendor"] = tmp_str;

      start++;
      tmp_str = str.substr(start, end - start);
      transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
      j["VERSION"][vr.second]["version"] = tmp_str;

      if ( is_TI_VR == true ) continue;
      start = str.rfind(' ');
      end = str.size();
      tmp_str = str.substr(start, end - start);
      transform(tmp_str.begin(), tmp_str.end(), tmp_str.begin(), ::tolower);
      j["VERSION"][vr.second]["rmng_w"] = tmp_str;
    } catch (string& err) {
      if (err.find("empty") != string::npos) {
        j["VERSION"] = "not_present";
      } else {
        j["VERSION"] = "error_returned";
      } 
    }
  }
  return FW_STATUS_SUCCESS;
}

int VrComponent::print_version() {
  string ver("");
  uint8_t addr = 0;

  // Print VR Version
  for (auto& vr:list) {
    try {
      server.ready();
      addr = vr.first;
      if (get_ver_str(addr, ver) < 0) {
        throw "Error in getting the version of " + vr.second;
      }
      cout << vr.second << " Version : " << ver << endl;
    } catch (string err) {
      printf("%s Version : NA (%s)\n", vr.second.c_str(), err.c_str());
    }
  }

  return FW_STATUS_SUCCESS;
}

int VrComponent::update(string image) { 
  int ret = 0;

  try {
    server.ready();

    ret = bic_update_fw(FRU_SERVER, FW_VR, (char *)image.c_str(), FORCE_UPDATE_UNSET);
    if (ret < 0) {
      return -1;
    }
  } catch (string err) {
    printf("%s\n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }

  return ret;
}
#endif
