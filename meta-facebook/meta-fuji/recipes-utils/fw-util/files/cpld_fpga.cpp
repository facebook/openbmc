#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>

using namespace std;

class CpldFpgaComponent : public Component {
  string comp;
  uint8_t fru_id = 0;
  public:
  CpldFpgaComponent(string fru, string _comp, uint8_t _fru_id)
    : Component(fru, _comp), comp(_comp) ,fru_id(_fru_id) {}

  int print_version() {
    uint8_t ver[8] = {0};
    string comp_upper(comp);
    std::string::size_type find_len;

    transform(comp_upper.begin(), comp_upper.end(),
              comp_upper.begin(), ::toupper);
    find_len = comp_upper.find("_");
    if (find_len != string::npos) {
      comp_upper[find_len] = ' ';
    }

    printf("%s: ", comp_upper.c_str());
    try {
      /* Print CPLD Version */
      if (pal_get_cpld_fpga_fw_ver(fru_id, (char *)comp.c_str(), ver)) {
        printf("NA\n");
      } else {
        if(fru_id == FRU_CPLD) {
          printf("%u.%u.%u\n", ver[0], ver[1], ver[2]);
        } else {
          printf("%u.%u\n", ver[0], ver[1]);
        }

      }
    } catch (string err) {
      printf("NA (%s)\n", err.c_str());
    }
    return 0;
  }
};

// Register CPLD FW components
CpldFpgaComponent scmcpld("cpld", SCM_CPLD, FRU_CPLD);
CpldFpgaComponent smbcpld("cpld", SMB_CPLD, FRU_CPLD);
CpldFpgaComponent fcmcpld_t("cpld", FCM_CPLD_T, FRU_CPLD);
CpldFpgaComponent fcmcpld_b("cpld", FCM_CPLD_B, FRU_CPLD);
CpldFpgaComponent pwrcpld_l("cpld", PWR_CPLD_L, FRU_CPLD);
CpldFpgaComponent pwrcpld_r("cpld", PWR_CPLD_R, FRU_CPLD);
// Register FPGA FW components
CpldFpgaComponent pim1_domfpga("fpga", PIM1_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent pim2_domfpga("fpga", PIM2_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent pim3_domfpga("fpga", PIM3_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent pim4_domfpga("fpga", PIM4_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent pim5_domfpga("fpga", PIM5_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent pim6_domfpga("fpga", PIM6_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent pim7_domfpga("fpga", PIM7_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent pim8_domfpga("fpga", PIM8_DOM_FPGA, FRU_FPGA);
CpldFpgaComponent iobfpga("fpga", IOB_FPGA, FRU_FPGA);
