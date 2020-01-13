#include "fw-util.h"
#include <openbmc/pal.h>
#include <openbmc/vr.h>

extern struct vr_info *dev_list;
extern int dev_list_count;

using namespace std;

class VrComponent : public Component {
  public:
    VrComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int update(string image);
};

int VrComponent::print_version() {
  char ver[MAX_VER_STR_LEN] = {0};
  int i;

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  for (i = 0; i < dev_list_count; i++) {
    if (vr_fw_version(i, NULL, ver)) {
      cout << string(dev_list[i].dev_name) << " Version: NA" << endl;
    } else {
      cout << string(dev_list[i].dev_name) << " Version: " << string(ver) << endl;
    }
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

  ret = vr_fw_update(NULL, (char *)image.c_str());
  if (ret < 0) {
    cout << "ERROR: VR Firmware update failed!" << endl;
  }

  vr_remove();
  return ret;
}

VrComponent vr("mb", "vr");
