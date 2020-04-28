#include <cstdio>
#include <cstring>
#include <openbmc/vr.h>
#include "vr_fw.h"

using namespace std;

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

int VrComponent::update(string image) {
  int ret;

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  ret = vr_fw_update(dev_name.c_str(), (char *)image.c_str());
  if (ret < 0) {
    cout << "ERROR: VR Firmware update failed!" << endl;
  }

  vr_remove();
  return ret;
}
