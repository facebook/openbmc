#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <openbmc/vr.h>
#include "vr_fw.h"

using namespace std;

int VrComponent::get_version(json& j) {
  char ver[MAX_VER_STR_LEN] = {0};

  try {
    if (vr_probe() || vr_fw_version(-1, dev_name.c_str(), ver)) {
      throw "Error in getting the version of " + dev_name;
    }
    vr_remove();

    // IFX: Infineon $ver, Remaining Writes: $times
    // TI:  Texas Instruments $ver
    // MPS: MPS $ver
    string str(ver);
    string tmp_str;
    size_t start;
    auto end = str.find(',');

    if (end != string::npos) {
      start = str.rfind(' ') + 1;
      tmp_str = str.substr(start, str.size() - start);
      transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
      j["rmng_w"] = tmp_str;
    } else {
      end = str.size();
    }
    start = str.rfind(' ', end);

    tmp_str = str.substr(0, start);
    transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
    j["vendor"] = tmp_str;

    start++;
    tmp_str = str.substr(start, end - start);
    transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
    j["VERSION"] = tmp_str;
  } catch (string& err) {
    j["VERSION"] = "error_returned";
  }

  return FW_STATUS_SUCCESS;
}

int VrComponent::print_version() {
  char ver[MAX_VER_STR_LEN] = {0};

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  if (vr_fw_version(-1, dev_name.c_str(), ver)) {
    cout << dev_name << " Version: NA" << endl;
  } else {
    cout << dev_name << " Version: " << string(ver) << endl;
  }

  vr_remove();
  return 0;
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
