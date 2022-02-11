#include "fw-util.h"
#include <openbmc/vr.h>
#include <openbmc/pal.h>

using namespace std;

class VrComponent : public Component {
  private:
  uint8_t g_board_rev_id = BOARD_REV_EVT;
  uint8_t g_vr_cpu0_vddq_abc;
  uint8_t g_vr_cpu0_vddq_def;
  uint8_t g_vr_cpu1_vddq_ghj;
  uint8_t g_vr_cpu1_vddq_klm;

  public:
  VrComponent(string fru, string comp)
    : Component(fru, comp) {
    pal_get_board_rev_id(&g_board_rev_id);

    if (g_board_rev_id == BOARD_REV_POWERON ||
      g_board_rev_id == BOARD_REV_EVT ) {
      g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC_EVT;
      g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF_EVT;
      g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ_EVT;
      g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM_EVT;
    } else {
      g_vr_cpu0_vddq_abc = VR_CPU0_VDDQ_ABC;
      g_vr_cpu0_vddq_def = VR_CPU0_VDDQ_DEF;
      g_vr_cpu1_vddq_ghj = VR_CPU1_VDDQ_GHJ;
      g_vr_cpu1_vddq_klm = VR_CPU1_VDDQ_KLM;
    }
  }

  int print_version() {
    char ver[128] = {0};
    // Print VR Version
    if (vr_fw_version(VR_PCH_PVNN, ver)) {
      cout << "VR_PCH_[PVNN, PV1V05] Version: NA Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_PCH_[PVNN, PV1V05] " << string(ver) << endl;
    }

    if (vr_fw_version(VR_CPU0_VCCIN, ver)) {
      cout << "VR_CPU0_[VCCIN, VSA] Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU0_[VCCIN, VSA] " << string(ver) << endl;
    }

    if (vr_fw_version(VR_CPU0_VCCIO, ver)) {
      cout << "VR_CPU0_VCCIO Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU0_VCCIO " << string(ver) << endl;
    }

    if (vr_fw_version(g_vr_cpu0_vddq_abc, ver)) {
      cout << "VR_CPU0_VDDQ_ABC Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU0_VDDQ_ABC " << string(ver) << endl;
    }

    if (vr_fw_version(g_vr_cpu0_vddq_def, ver)) {
      cout << "VR_CPU0_VDDQ_DEF Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU0_VDDQ_DEF " << string(ver) << endl;
    }

    if (vr_fw_version(VR_CPU1_VCCIN, ver)) {
      cout << "VR_CPU1_[VCCIN, VSA] Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU1_[VCCIN, VSA] " << string(ver) << endl;
    }

    if (vr_fw_version(VR_CPU1_VCCIO, ver)) {
      cout << "VR_CPU1_VCCIO Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU1_VCCIO " << string(ver) << endl;
    }

    if (vr_fw_version(g_vr_cpu1_vddq_ghj, ver)) {
      cout << "VR_CPU1_VDDQ_GHJ Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU1_VDDQ_GHJ " << string(ver) << endl;
    }

    if (vr_fw_version(g_vr_cpu1_vddq_klm, ver)) {
      cout << "VR_CPU1_VDDQ_KLM Version: NA, Checksum: NA, DeviceID: NA" << endl;
    } else {
      cout << "VR_CPU1_VDDQ_KLM " << string(ver) << endl;
    }
    return 0;
  }
  int update(string image) {
    uint8_t board_info;
    int ret = 0;
    if (pal_get_platform_id(&board_info) < 0) {
      printf("Get PlatformID failed!\n");
      ret = -1;
    } else {
      //call vr function
      ret = vr_fw_update(FRU_MB, board_info, image.c_str());
      if (ret < 0) {
        printf("ERROR: VR Firmware update failed!\n");
      }
    }
    return ret;
  }
};

VrComponent vr("mb", "vr");

