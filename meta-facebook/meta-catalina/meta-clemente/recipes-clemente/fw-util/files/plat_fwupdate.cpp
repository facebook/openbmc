#include <cstdio>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>
#include <openbmc/obmc-i2c.h>
#include "vr_fw.h"
#include "nic_ext.h"
#include "usbdbg.h"

#define FRU_NIC0   (0)
#define FRU_NIC1   (1)

using namespace std;

int pal_get_fru_id(char *str, uint8_t *fru)
{
  const char *list[] = {"all", "bmc", "hdd", "hmc", "nic0", "nic1", "pdb", "scm"};
  uint8_t id;

  if (!str || !fru)
  {
    return -1;
  }

  for (id = 0; id < sizeof(list)/sizeof(list[0]); id++)
  {
    if (strncmp(str, list[id], 16) == 0)
    {
      *fru = id;
      return 0;
    }
  }

  return -1;
}

class FwComponentConfig
{
public:
  FwComponentConfig()
  {
    char value[MAX_VALUE_LEN] = {0};

    // NIC Component
    kv_get("pdb_hw_rev", value, NULL, 0);
    if (std::string(value) == "EVT")
    {
      static NicExtComponent nic1_evt("nic1", "nic1", "nic1_fw_ver", FRU_NIC1, 1, 0x00);
    }
    else
    {
      static NicExtComponent nic0("nic0", "nic0", "nic0_fw_ver", FRU_NIC0, 0, 0x00);
      static NicExtComponent nic1("nic1", "nic1", "nic1_fw_ver", FRU_NIC1, 1, 0x20);
    }

    // VR Component
    static VrComponent pdb_vr_n1("pdb", "vr_n1", "PDB_P12V_N1_VR");
    static VrComponent pdb_vr_n2("pdb", "vr_n2", "PDB_P12V_N2_VR");
    static VrComponent pdb_vr_aux("pdb", "vr_aux", "PDB_P12V_AUX_VR");
  }
};

FwComponentConfig _fw_comp_conf;
