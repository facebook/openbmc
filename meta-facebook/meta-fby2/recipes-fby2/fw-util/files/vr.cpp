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
      } catch (const string& err) {
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
                "PVDDCR_CPU VR Version: %02X%02X%02X%02X, Remaining Writes: %u\n",
                ver[3], ver[2], ver[1], ver[0], ver[4]);
          }

          if (bic_get_fw_ver(slot_id, FW_PVDDCR_SOC_VR, ver)) {
            printf("PVDDCR_SOC VR Version: NA\n");
          } else {
            printf(
                "PVDDCR_SOC VR Version: %02X%02X%02X%02X, Remaining Writes: %u\n",
                ver[3], ver[2], ver[1], ver[0], ver[4]);
          }

          if (bic_get_fw_ver(slot_id, FW_PVDDIO_ABCD_VR, ver)) {
            printf("PVDDIO_ABCD VR Version: NA\n");
          } else {
            printf(
                "PVDDIO_ABCD VR Version: %02X%02X%02X%02X, Remaining Writes: %u\n",
                ver[3], ver[2], ver[1], ver[0], ver[4]);
          }

          if (bic_get_fw_ver(slot_id, FW_PVDDIO_EFGH_VR, ver)) {
            printf("PVDDIO_EFGH VR Version: NA\n");
          } else {
            printf(
                "PVDDIO_EFGH VR Version: %02X%02X%02X%02X, Remaining Writes: %u\n",
                ver[3], ver[2], ver[1], ver[0], ver[4]);
          }
        } catch (const string& err) {
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
        } catch (const string& err) {
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
    } catch (const string& err) {
      ret = FW_STATUS_NOT_SUPPORTED;
    }
    return ret;
  }

  int get_version(json& j) override;

private:
  void get_version_gpv2(json& j);
  void get_version_server(json& j);
  void get_version_server_nd(json& j);

};

int VrComponent::get_version(json& j)
{
  if (fby2_get_slot_type(slot_id) == SLOT_TYPE_GPV2) {
    get_version_gpv2(j);
    return FW_STATUS_SUCCESS;
  } else {
    int ret;
    uint8_t server_type = 0xFF;
    ret = fby2_get_server_type(slot_id, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
    }
    switch (server_type) {
    case SERVER_TYPE_ND:
      get_version_server_nd(j);
      break;
    case SERVER_TYPE_TL:
    default:
      get_version_server(j);
      break;
    }
  }
  return FW_STATUS_SUCCESS;
}

void VrComponent::get_version_gpv2(json& j)
{
  uint8_t buf[32] = {0};
  char ver_str[16] = {0};
  const map<uint8_t, string> list = {
    {FW_3V3_VR, "3V3"},
    {FW_0V92,   "0V92"}
  };

  for ( auto& vr:list ) {
    try {
      server.ready();
      if (bic_get_fw_ver(slot_id, vr.first, buf)) {
        throw "Error in getting the version of " + vr.second;
      }

      snprintf(ver_str, sizeof(ver_str), "0x%02x", buf[0]);
      j["VERSION"][vr.second]["version"] = std::string(ver_str);
    } catch (const string& err) {
      if ( err.find("empty") != string::npos )
        j["VERSION"] = "not_present";
      else
        j["VERSION"] = "error_returned";
      return;
    }
  }
}

void VrComponent::get_version_server(json& j)
{
  uint8_t buf[32] = {0};
  char ver_str[16] = {0};
  const map<uint8_t, string> list = {
    {FW_PVCCIO_VR,  "PVCCIO"},
    {FW_PVCCIN_VR,  "PVCCIN"},
    {FW_PVCCSA_VR,  "PVCCSA"},
    {FW_DDRAB_VR,   "DDRAB"},
    {FW_DDRDE_VR,   "DDRDE"},
    {FW_PVNNPCH_VR, "PVNNPCH"},
    {FW_P1V05_VR,   "P1V05"}
  };

  for ( auto& vr:list ) {
    try {
      server.ready();
      if (bic_get_fw_ver(slot_id, vr.first, buf)) {
        throw "Error in getting the version of " + vr.second;
      }

      snprintf(ver_str, sizeof(ver_str), "0x%02x%02x, 0x%02x%02x", buf[0], buf[1], buf[2], buf[3]);
      j["VERSION"][vr.second]["version"] = std::string(ver_str);
    } catch (const string& err) {
      if ( err.find("empty") != string::npos )
        j["VERSION"] = "not_present";
      else
        j["VERSION"] = "error_returned";
      return;
    }
  }
}

void VrComponent::get_version_server_nd(json& j)
{
  uint8_t buf[32] = {0};
  char ver_str[16] = {0};
  char rmng_w_str[4] = {0};
  const map<uint8_t, string> list = {
    {FW_PVDDCR_CPU_VR,  "PVDDCR_CPU"},
    {FW_PVDDCR_SOC_VR,  "PVDDCR_SOC"},
    {FW_PVDDIO_ABCD_VR, "PVDDIO_ABCD"},
    {FW_PVDDIO_EFGH_VR, "PVDDIO_EFGH"}
  };

  for ( auto& vr:list ) {
    try {
      server.ready();
      if (bic_get_fw_ver(slot_id, vr.first, buf)) {
        throw "Error in getting the version of " + vr.second;
      }

      snprintf(ver_str, sizeof(ver_str), "%02x%02x%02x%02x", buf[3], buf[2], buf[1], buf[0]);
      snprintf(rmng_w_str, sizeof(rmng_w_str), "%u", buf[4]);
      j["VERSION"][vr.second]["version"] = std::string(ver_str);
      j["VERSION"][vr.second]["rmng_w"] = std::string(rmng_w_str);
    } catch (const string& err) {
      if ( err.find("empty") != string::npos )
        j["VERSION"] = "not_present";
      else
        j["VERSION"] = "error_returned";
      return;
    }
  }
}

VrComponent vr1("slot1", "vr", 1);
VrComponent vr2("slot2", "vr", 2);
VrComponent vr3("slot3", "vr", 3);
VrComponent vr4("slot4", "vr", 4);
