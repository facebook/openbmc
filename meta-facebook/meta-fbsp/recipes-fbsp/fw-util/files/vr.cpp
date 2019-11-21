#include "fw-util.h"
#include <openbmc/pal.h>
#include <facebook/vr.h>

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

  if (vr_fw_version(VR_PCH_PVNN, ver)) {
    cout << "VR_PCH_[PVNN, P1V05] Version: NA" << endl;
  } else {
    cout << "VR_PCH_[PVNN, P1V05] " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU0_VCCIN, ver)) {
    cout << "VR_CPU0_[VCCIN, VCCSA] Version: NA" << endl;
  } else {
    cout << "VR_CPU0_[VCCIN, VCCSA] " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU0_VCCIO, ver)) {
    cout << "VR_CPU0_VCCIO Version: NA" << endl;
  } else {
    cout << "VR_CPU0_VCCIO " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU0_VDDQ_ABC, ver)) {
    cout << "VR_CPU0_VDDQ_ABC Version: NA" << endl;
  } else {
    cout << "VR_CPU0_VDDQ_ABC " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU0_VDDQ_DEF, ver)) {
    cout << "VR_CPU0_VDDQ_DEF Version: NA" << endl;
  } else {
    cout << "VR_CPU0_VDDQ_DEF " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU1_VCCIN, ver)) {
    cout << "VR_CPU1_[VCCIN, VCCSA] Version: NA" << endl;
  } else {
    cout << "VR_CPU1_[VCCIN, VCCSA] " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU1_VCCIO, ver)) {
    cout << "VR_CPU1_VCCIO Version: NA" << endl;
  } else {
    cout << "VR_CPU1_VCCIO " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU1_VDDQ_ABC, ver)) {
    cout << "VR_CPU1_VDDQ_ABC Version: NA" << endl;
  } else {
    cout << "VR_CPU1_VDDQ_ABC " << string(ver) << endl;
  }

  if (vr_fw_version(VR_CPU1_VDDQ_DEF, ver)) {
    cout << "VR_CPU1_VDDQ_DEF Version: NA" << endl;
  } else {
    cout << "VR_CPU1_VDDQ_DEF " << string(ver) << endl;
  }

  return 0;
}

int VrComponent::update(string image) {
  int ret = 0;

  ret = vr_fw_update((char *)image.c_str());
  if (ret < 0) {
    printf("ERROR: VR Firmware update failed!\n");
  }

  return ret;
}

VrComponent vr("mb", "vr");
