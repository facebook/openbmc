/*
 * fw-util
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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
    size_t find_len = 0;

    transform(comp_upper.begin(), comp_upper.end(),
              comp_upper.begin(), ::toupper);
    find_len = comp_upper.find("_");
    if (find_len != string::npos) {
      comp_upper.replace(find_len, 1, " ");
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

// Register CPLD FW components
CpldFpgaComponent scmcpld("cpld", SCM_CPLD, FRU_CPLD);
CpldFpgaComponent smbcpld("cpld", SMB_CPLD, FRU_CPLD);
CpldFpgaComponent fcmcpld("cpld", FCM_CPLD, FRU_CPLD);
CpldFpgaComponent pwrcpld("cpld", PWR_CPLD, FRU_CPLD);
// Register FPGA FW components
CpldFpgaComponent domfpga1("fpga", DOM_FPGA1, FRU_FPGA);
CpldFpgaComponent domfpga2("fpga", DOM_FPGA2, FRU_FPGA);
