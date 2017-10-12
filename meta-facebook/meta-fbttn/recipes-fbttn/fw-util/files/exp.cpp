#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <facebook/exp.h>

using namespace std;

class ExpanderComponent : public Component {
  public:
    ExpanderComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version()
    {
      uint8_t ver[32] = {0};
      int ret = FW_STATUS_SUCCESS;

      // Read Firwmare Versions of Expander via IPMB
      // ID: exp is 0, ioc is 1
      ret = exp_get_fw_ver(ver);
      if (ret == FW_STATUS_SUCCESS) {
        printf("Expander Version: 0x%02x%02x%02x%02x\n", ver[0], ver[1], ver[2], ver[3]);
      }

      return ret;
    }
};

ExpanderComponent exp("scc", "exp");

