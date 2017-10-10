#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class BicFwComponent : public Component {
  uint8_t slot_id;
  Server server;
  public:
    BicFwComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id) {}
    int update(string image) {
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      return bic_update_fw(slot_id, UPDATE_BIC, (char *)image.c_str());
    }
    int print_version() {
      uint8_t ver[32];
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      // Print Bridge-IC Bootloader Version
      if (bic_get_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver)) {
        printf("Bridge-IC Bootloader Version: NA\n");
      }
      else {
        printf("Bridge-IC Bootloader Version: v%x.%02x\n", ver[0], ver[1]);
      }
      return 0;
    }
};

class BicFwBlComponent : public Component {
  uint8_t slot_id;
  Server server;
  public:
    BicFwBlComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id) {}
    int update(string image) {
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      return bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, (char *)image.c_str());
    }
    int print_version() {
      uint8_t ver[32];
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      // Print Bridge-IC Version
      if (bic_get_fw_ver(slot_id, FW_BIC, ver)) {
        printf("Bridge-IC Version: NA\n");
      }
      else {
        printf("Bridge-IC Version: v%x.%02x\n", ver[0], ver[1]);
      }
      return 0;
    }
};

BicFwComponent bicfw1("slot1", "bic", 1);
BicFwComponent bicfw2("slot2", "bic", 2);
BicFwComponent bicfw3("slot3", "bic", 3);
BicFwComponent bicfw4("slot4", "bic", 4);
BicFwComponent bicfwbl1("slot1", "bicbl", 1);
BicFwComponent bicfwbl2("slot2", "bicbl", 2);
BicFwComponent bicfwbl3("slot3", "bicbl", 3);
BicFwComponent bicfwbl4("slot4", "bicbl", 4);
