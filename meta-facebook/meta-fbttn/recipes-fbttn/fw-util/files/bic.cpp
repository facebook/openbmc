#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class BicFwComponent : public Component {
  string fru_name;
  uint8_t slot_id = 0;
  Server server;
  char err_str[SERVER_ERR_STR_LEN] = {0};
  public:
    BicFwComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), fru_name(fru), slot_id(_slot_id), server(_slot_id, (char *)fru_name.c_str(), err_str) {}
    int update(string image) {
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      return bic_update_fw(slot_id, UPDATE_BIC, (char *)image.c_str());
    }
    int print_version() {
      uint8_t ver[32];
      if (!server.ready()) {
        printf("Bridge-IC Version: NA (%s)\n", err_str);
      } else {
        // Print Bridge-IC Version
        if (bic_get_fw_ver(slot_id, FW_BIC, ver)) {
          printf("Bridge-IC Version: NA\n");
        }
        else {
          printf("Bridge-IC Version: v%x.%02x\n", ver[0], ver[1]);
        }
      }
      return 0;
    }
};

class BicFwBlComponent : public Component {
  string fru_name;
  uint8_t slot_id = 0;
  Server server;
  char err_str[SERVER_ERR_STR_LEN] = {0};
  public:
    BicFwBlComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), fru_name(fru), slot_id(_slot_id), server(_slot_id, (char *)fru_name.c_str(), err_str) {}
    int update(string image) {
      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      return bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, (char *)image.c_str());
    }
    int print_version() {
      uint8_t ver[32];
      if (!server.ready()) {
        printf("Bridge-IC Bootloader Version: NA (%s)\n", err_str);
      } else {
        // Print Bridge-IC Bootloader Version
        if (bic_get_fw_ver(slot_id, FW_BIC_BOOTLOADER, ver)) {
          printf("Bridge-IC Bootloader Version: NA\n");
        }
        else {
          printf("Bridge-IC Bootloader Version: v%x.%02x\n", ver[0], ver[1]);
        }
      }
      return 0;
    }
};

BicFwComponent bicfw1("server", "bic", 1);
BicFwBlComponent bicfwbl1("server", "bicbl", 1);
