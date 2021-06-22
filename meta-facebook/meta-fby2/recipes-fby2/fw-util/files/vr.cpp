#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <syslog.h>
#include <cstdio>
#include <cstring>
#include "fw-util.h"
#include "server.h"

using namespace std;

class VrComponent : public Component {
  uint8_t slot_id = 0;
  Server server;

 public:
  VrComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {
    int slot_type = fby2_get_slot_type(slot_id);
    if (slot_type != SLOT_TYPE_SERVER && slot_type != SLOT_TYPE_GPV2) {
      (*fru_list)[fru].erase(comp);
      if ((*fru_list)[fru].size() == 0) {
        fru_list->erase(fru);
      }
    }
  }

  int print_version() {
    uint8_t ver[32] = {0};
    if (fby2_get_slot_type(slot_id) == SLOT_TYPE_GPV2) {
      try {
        server.ready();
        // Print 3V3_VR VR Version
        if (bic_get_fw_ver(slot_id, FW_3V3_VR, ver)) {
          printf("3V3 VR Version: NA\n");
        } else {
          printf("3V3 VR Version: 0x%02x\n", ver[0]);
        }

        // Print 3V3_VR VR Version
        if (bic_get_fw_ver(slot_id, FW_0V92, ver)) {
          printf("0V92 VR Version: NA\n");
        } else {
          printf("0V92 VR Version: 0x%02x\n", ver[0]);
        }
      } catch (string err) {
        printf("3V3 VR Version: NA (%s)\n", err.c_str());
        printf("0V92 VR Version: NA (%s)\n", err.c_str());
      }
      return 0;
    }

    int ret;
    uint8_t server_type = 0xFF;
    ret = fby2_get_server_type(slot_id, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    }

    switch (server_type) {
      case SERVER_TYPE_ND:
        try {
          server.ready();
          if (bic_get_fw_ver(slot_id, FW_PVDDCR_CPU_VR, ver)) {
            printf("PVDDCR_CPU VR Version: NA\n");
          } else {
            printf(
                "PVDDCR_CPU VR Version: 0x%02x%02x, 0x%02x%02x, Remaining Writes: %u\n",
                ver[0], ver[1], ver[2], ver[3], ver[4]);
          }

          if (bic_get_fw_ver(slot_id, FW_PVDDCR_SOC_VR, ver)) {
            printf("PVDDCR_SOC VR Version: NA\n");
          } else {
            printf(
                "PVDDCR_SOC VR Version: 0x%02x%02x, 0x%02x%02x, Remaining Writes: %u\n",
                ver[0], ver[1], ver[2], ver[3], ver[4]);
          }

          if (bic_get_fw_ver(slot_id, FW_PVDDIO_ABCD_VR, ver)) {
            printf("PVDDIO_ABCD VR Version: NA\n");
          } else {
            printf(
                "PVDDIO_ABCD VR Version: 0x%02x%02x, 0x%02x%02x, Remaining Writes: %u\n",
                ver[0], ver[1], ver[2], ver[3], ver[4]);
          }

          if (bic_get_fw_ver(slot_id, FW_PVDDIO_EFGH_VR, ver)) {
            printf("PVDDIO_EFGH VR Version: NA\n");
          } else {
            printf(
                "PVDDIO_EFGH VR Version: 0x%02x%02x, 0x%02x%02x, Remaining Writes: %u\n",
                ver[0], ver[1], ver[2], ver[3], ver[4]);
          }
        } catch (string err) {
          printf("PVDDCR_CPU VR Version: NA (%s)\n", err.c_str());
          printf("PVDDCR_SOC VR Version: NA (%s)\n", err.c_str());
          printf("PVDDIO_ABCD VR Version: NA (%s)\n", err.c_str());
          printf("PVDDIO_EFGH VR Version: NA (%s)\n", err.c_str());
        }
        break;

      case SERVER_TYPE_TL:
      default:
        try {
          server.ready();
          // Print PVCCIO VR Version
          if (bic_get_fw_ver(slot_id, FW_PVCCIO_VR, ver)) {
            printf("PVCCIO VR Version: NA\n");
          } else {
            printf(
                "PVCCIO VR Version: 0x%02x%02x, 0x%02x%02x\n",
                ver[0],
                ver[1],
                ver[2],
                ver[3]);
          }

          // Print PVCCIN VR Version
          if (bic_get_fw_ver(slot_id, FW_PVCCIN_VR, ver)) {
            printf("PVCCIN VR Version: NA\n");
          } else {
            printf(
                "PVCCIN VR Version: 0x%02x%02x, 0x%02x%02x\n",
                ver[0],
                ver[1],
                ver[2],
                ver[3]);
          }

          // Print PVCCSA VR Version
          if (bic_get_fw_ver(slot_id, FW_PVCCSA_VR, ver)) {
            printf("PVCCSA VR Version: NA\n");
          } else {
            printf(
                "PVCCSA VR Version: 0x%02x%02x, 0x%02x%02x\n",
                ver[0],
                ver[1],
                ver[2],
                ver[3]);
          }

          // Print DDRAB VR Version
          if (bic_get_fw_ver(slot_id, FW_DDRAB_VR, ver)) {
            printf("DDRAB VR Version: NA\n");
          } else {
            printf(
                "DDRAB VR Version: 0x%02x%02x, 0x%02x%02x\n",
                ver[0],
                ver[1],
                ver[2],
                ver[3]);
          }

          // Print DDRDE VR Version
          if (bic_get_fw_ver(slot_id, FW_DDRDE_VR, ver)) {
            printf("DDRDE VR Version: NA\n");
          } else {
            printf(
                "DDRDE VR Version: 0x%02x%02x, 0x%02x%02x\n",
                ver[0],
                ver[1],
                ver[2],
                ver[3]);
          }

          // Print PVNNPCH VR Version
          if (bic_get_fw_ver(slot_id, FW_PVNNPCH_VR, ver)) {
            printf("PVNNPCH VR Version: NA\n");
          } else {
            printf(
                "PVNNPCH VR Version: 0x%02x%02x, 0x%02x%02x\n",
                ver[0],
                ver[1],
                ver[2],
                ver[3]);
          }

          // Print P1V05 VR Version
          if (bic_get_fw_ver(slot_id, FW_P1V05_VR, ver)) {
            printf("P1V05 VR Version: NA\n");
          } else {
            printf(
                "P1V05 VR Version: 0x%02x%02x, 0x%02x%02x\n",
                ver[0],
                ver[1],
                ver[2],
                ver[3]);
          }
        } catch (string err) {
          printf("PVCCIO VR Version: NA (%s)\n", err.c_str());
          printf("PVCCIN VR Version: NA (%s)\n", err.c_str());
          printf("PVCCSA VR Version: NA (%s)\n", err.c_str());
          printf("DDRAB VR Version: NA (%s)\n", err.c_str());
          printf("DDRDE VR Version: NA (%s)\n", err.c_str());
          printf("PVNNPCH VR Version: NA (%s)\n", err.c_str());
          printf("P1V05 VR Version: NA (%s)\n", err.c_str());
        }
        break;
    }

    return 0;
  }

  int update(string image) {
    int ret;

    try {
      server.ready();
      ret = bic_update_fw(slot_id, UPDATE_VR, (char*)image.c_str());
    } catch (string err) {
      ret = FW_STATUS_NOT_SUPPORTED;
    }
    return ret;
  }
};

VrComponent vr1("slot1", "vr", 1);
VrComponent vr2("slot2", "vr", 2);
VrComponent vr3("slot3", "vr", 3);
VrComponent vr4("slot4", "vr", 4);
