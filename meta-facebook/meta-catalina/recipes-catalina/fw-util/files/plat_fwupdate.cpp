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

//NIC Component
NicExtComponent nic0("nic0", "nic0", "nic0_fw_ver", FRU_NIC0, 0, 0x00);
// NicExtComponent nic1("nic1", "nic1", "nic1_fw_ver", FRU_NIC1, 1, 0x00);

//VR Component
VrComponent pdb_vr_n1("pdb", "vr_n1", "PDB_P12V_N1_VR");
VrComponent pdb_vr_n2("pdb", "vr_n2", "PDB_P12V_N2_VR");
VrComponent pdb_vr_aux("pdb", "vr_aux", "PDB_P12V_AUX_VR");
