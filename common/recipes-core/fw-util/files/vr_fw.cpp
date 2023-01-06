#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <openbmc/vr.h>
#include <openbmc/kv.h>
#include "vr_fw.h"

using namespace std;

bool get_active_version(const char* key, string& ver) {
  char value[MAX_VER_STR_LEN] = {0};
  bool ret = ((kv_get(key, value, NULL, KV_FPERSIST) < 0) && (errno == ENOENT));
  ver = string(value);
  return !ret;
}

int VrComponent::get_version(json& j) {
  char ver[MAX_VER_STR_LEN] = {0};
  char active_key[MAX_VER_STR_LEN] = {0};

  j["PRETTY_COMPONENT"] = dev_name;

  try {
    if (vr_probe() || vr_fw_full_version(-1, dev_name.c_str(), ver, active_key)) {
      throw "Error in getting the version of " + dev_name;
    }
    vr_remove();

    // IFX: Infineon $ver, Remaining Writes: $times
    // TI:  Texas Instruments $ver
    // MPS: MPS $ver
    string str(ver);
    string act_str;
    string tmp_str;
    string rw_str;
    size_t start;
    auto end = str.find(',');

    if (end != string::npos) {
      start = str.rfind(' ') + 1;
      rw_str = str.substr(start, str.size() - start);
      transform(rw_str.begin(), rw_str.end(), rw_str.begin(), ::tolower);
      j["REMAINING_WRITE"] = rw_str;
    } else {
      end = str.size();
    }
    start = str.rfind(' ', end);

    tmp_str = str.substr(0, start);
    string vendor_str(tmp_str);
    transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
    j["VENDOR"] = tmp_str;

    start++;
    tmp_str = str.substr(start, end - start);
    transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
    j["VERSION"] = tmp_str;
    if(rw_str.empty())
      j["PRETTY_VERSION"] = vendor_str + " " + tmp_str;
    else
      j["PRETTY_VERSION"] = vendor_str + " " + tmp_str + ", Remaining Write: " + rw_str;

    if(get_active_version(active_key, act_str)) {
      end = act_str.find(',');
      if (end == string::npos) {
        end = act_str.size();
      }
      start = act_str.rfind(' ', end) + 1;

      tmp_str = act_str.substr(start, end - start);
      transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
      j["VERSION_ACTIVE"] = tmp_str;
    } else {
      j["VERSION_ACTIVE"] = string(j["VERSION"]);
    }
  } catch (string& err) {
    j["VERSION"] = "NA";
  }

  return FW_STATUS_SUCCESS;
}

int VrComponent::update(string image, bool force) {
  int ret;
  string comp = this->component();

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
  ret = vr_fw_update(dev_name.c_str(), (char *)image.c_str(), force);
  if (ret < 0) {
    cout << "ERROR: VR Firmware update failed!" << endl;
  } else {
    syslog(LOG_CRIT, "Component %s %s completed", comp.c_str(), force? "force upgrade": "upgrade");
  }

  vr_remove();
  return ret;
}

int VrComponent::update(string image) {
  return update(image, 0);
}

int VrComponent::fupdate(string image) {
  return update(image, 1);
}
