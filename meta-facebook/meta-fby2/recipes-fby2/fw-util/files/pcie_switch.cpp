#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include <syslog.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#if defined(CONFIG_FBY2_GPV2)

class PcieSwitchComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
  PcieSwitchComponent(string fru, string comp, uint8_t _slot_id)
    : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {
    if (fby2_get_slot_type(_slot_id) != SLOT_TYPE_GPV2) {
      (*fru_list)[fru].erase(comp);
      if ((*fru_list)[fru].size() == 0) {
        fru_list->erase(fru);
      }
    }
  }

  int print_version() {
    uint8_t ver[32] = {0};

    if (fby2_get_slot_type(slot_id) != SLOT_TYPE_GPV2)
      return -1;

    if (bic_get_fw_ver(slot_id, FW_PCIE_SWITCH, ver)){
      printf("PCIE switch Version: NA\n");
    } else {
      printf("PCIE switch Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
    }
    return 0;
  }

  int update(string image) {
    if (fby2_get_slot_type(slot_id) != SLOT_TYPE_GPV2)
      return FW_STATUS_NOT_SUPPORTED;

    int ret = 0;
    try {
      server.ready();
      ret = bic_update_fw(slot_id, UPDATE_PCIE_SWITCH, (char *)image.c_str());
    } catch(string err) {
      ret = FW_STATUS_NOT_SUPPORTED;
    }
    return ret;
  }
};

// Register PCIE switch components
PcieSwitchComponent pcie_sw1("slot1", "pcie_sw", 1);
PcieSwitchComponent pcie_sw3("slot3", "pcie_sw", 3);
#endif

#endif
