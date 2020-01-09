#include <sstream>
#include "fw-util.h"
#include <openbmc/pal.h>
#include <openbmc/vr.h>

using namespace std;

class VrComponent0 : public Component {
  public:
    VrComponent0(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int update(string image);
};

int VrComponent0::print_version()
{
  char ver[MAX_VER_STR_LEN] = {0};
  string name("VR_P0V8_VDD");

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  for (int i = 0; i < 4; i++) {
    ostringstream vr_name;
    vr_name << name << i;
    cout << vr_name.str() << " Version: ";
    if (vr_fw_version(-1, vr_name.str().c_str(), ver))
      cout << "NA" << endl;
    else
      cout << string(ver) << endl;
  }

  vr_remove();
  return 0;
}

int VrComponent0::update(string image)
{
  int ret = 0;
  string name("VR_P0V8_VDD");

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  for (int i = 0; i < 4; i++) {
    ostringstream vr_name;
    vr_name << name << i;
    ret = vr_fw_update(vr_name.str().c_str(), image.c_str());
    if (ret < 0) {
      cout << "ERROR: VR Firmware update failed!\n" << endl;
    }
  }

  vr_remove();
  return ret;
}

class VrComponent1 : public Component {
  public:
    VrComponent1(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int update(string image);
};

int VrComponent1::print_version()
{
  char ver[MAX_VER_STR_LEN] = {0};
  string name("VR_P1V0_AVD");

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  for (int i = 0; i < 4; i++) {
    ostringstream vr_name;
    vr_name << name << i;
    cout << vr_name.str() << " Version: ";
    if (vr_fw_version(-1, vr_name.str().c_str(), ver))
      cout << "NA" << endl;
    else
      cout << string(ver) << endl;
  }

  vr_remove();
  return 0;
}

int VrComponent1::update(string image)
{
  int ret = 0;
  string name("VR_P1V0_AVD");

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }

  for (int i = 0; i < 4; i++) {
    ostringstream vr_name;
    vr_name << name << i;
    ret = vr_fw_update(vr_name.str().c_str(), image.c_str());
    if (ret < 0) {
      cout << "ERROR: VR Firmware update failed!\n" << endl;
    }
  }

  vr_remove();
  return ret;
}

VrComponent0 vr0("mb", "vr0");
VrComponent1 vr1("mb", "vr1");
