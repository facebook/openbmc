#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>

#define SCMCPLD       "scmcpld"
#define SMBCPLD       "smbcpld"
#define TOPFCMCPLD    "top_fcmcpld"
#define BOTTOMFCMCPLD "bottom_fcmcpld"
#define LEFTPDBCPLD   "left_pdbcpld"
#define RIGHTPDBCPLD  "right_pdbcpld"
#define SPIMUXCPLD    "spimuxcpld"
#define IOBFPGA       "iobfpga"
#define DOMFPGA       "domfpga"

#define PIM_RETRY  1

using namespace std;

class CpldFpgaComponent : public Component {
  string comp;
  uint8_t fru_id = 0;
  public:
  CpldFpgaComponent(string fru, string _comp, uint8_t _fru_id)
    : Component(fru, _comp), comp(_comp) ,fru_id(_fru_id) {}

  int print_version() {
    uint8_t pim_type = PIM_TYPE_UNPLUG, pim_num = 0;
    uint8_t ver[8] = {0};
    string comp_upper(comp);
    size_t find_len = 0;

    transform(comp_upper.begin(), comp_upper.end(),
              comp_upper.begin(), ::toupper);
    find_len = comp_upper.find("_");
    if (find_len != string::npos) {
      comp_upper.replace(find_len, 1, " ");
    }

    pim_num = fru_id - 2;
    if (comp == SPIMUXCPLD) {
      printf("PIM%d %s: NO_VERSION\n", pim_num, comp_upper.c_str());
      return 0;
    } else if (comp == DOMFPGA) {
      pim_type = pal_get_pim_type(fru_id, PIM_RETRY);
      if (pim_type == PIM_TYPE_16Q) {
        printf("PIM%d %s ", pim_num, "16Q");
      } else if (pim_type == PIM_TYPE_16O) {
        printf("PIM%d %s ", pim_num, "16O");
      } else if (pim_type == PIM_TYPE_4DD) {
        printf("PIM%d %s ", pim_num, "4DD");
      } else {
        printf("PIM%d %s: NA (pim is not inserted or domfpga is no firmware)\n",
               pim_num, comp_upper.c_str());
        return 0;
      }
    }

    printf("%s: ", comp_upper.c_str());
    try {
      /* Print CPLD Version */
      if (pal_get_cpld_fpga_fw_ver(fru_id, (char *)comp.c_str(), ver)) {
        printf("NA\n");
      } else {
        printf("%u.%u\n", ver[0], ver[1]);
      }
    } catch (string err) {
      printf("NA (%s)\n", err.c_str());
    }
    return 0;
  }
};

CpldFpgaComponent scmcpld("scm", SCMCPLD, FRU_SCM);
CpldFpgaComponent smbcpld("bmc", SMBCPLD, FRU_SMB);
CpldFpgaComponent topfcmcpld("bmc", TOPFCMCPLD, FRU_SMB);
CpldFpgaComponent bottomfcmcpld("bmc", BOTTOMFCMCPLD, FRU_SMB);
CpldFpgaComponent leftpdbcpld("bmc", LEFTPDBCPLD, FRU_SMB);
CpldFpgaComponent rightpdbcpld("bmc", RIGHTPDBCPLD, FRU_SMB);
CpldFpgaComponent spimuxcpld1("pim1", SPIMUXCPLD, FRU_PIM1);
CpldFpgaComponent spimuxcpld2("pim2", SPIMUXCPLD, FRU_PIM2);
CpldFpgaComponent spimuxcpld3("pim3", SPIMUXCPLD, FRU_PIM3);
CpldFpgaComponent spimuxcpld4("pim4", SPIMUXCPLD, FRU_PIM4);
CpldFpgaComponent spimuxcpld5("pim5", SPIMUXCPLD, FRU_PIM5);
CpldFpgaComponent spimuxcpld6("pim6", SPIMUXCPLD, FRU_PIM6);
CpldFpgaComponent spimuxcpld7("pim7", SPIMUXCPLD, FRU_PIM7);
CpldFpgaComponent spimuxcpld8("pim8", SPIMUXCPLD, FRU_PIM8);
CpldFpgaComponent iobfpga("bmc", IOBFPGA, FRU_SMB);
CpldFpgaComponent domfpga1("pim1", DOMFPGA, FRU_PIM1);
CpldFpgaComponent domfpga2("pim2", DOMFPGA, FRU_PIM2);
CpldFpgaComponent domfpga3("pim3", DOMFPGA, FRU_PIM3);
CpldFpgaComponent domfpga4("pim4", DOMFPGA, FRU_PIM4);
CpldFpgaComponent domfpga5("pim5", DOMFPGA, FRU_PIM5);
CpldFpgaComponent domfpga6("pim6", DOMFPGA, FRU_PIM6);
CpldFpgaComponent domfpga7("pim7", DOMFPGA, FRU_PIM7);
CpldFpgaComponent domfpga8("pim8", DOMFPGA, FRU_PIM8);
