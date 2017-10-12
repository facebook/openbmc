#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <openbmc/pal.h>
#include <facebook/exp.h>

using namespace std;

class SccIocComponent : public Component {
  public:
    SccIocComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version()
    {
      uint8_t ver[32] = {0};      
      uint8_t status = 0;
      int ret = FW_STATUS_SUCCESS;

      //Have to check HOST power, if is off, shows NA
      pal_get_server_power(FRU_SLOT1, &status);
      if (status != SERVER_POWER_ON) {
        printf("SCC IOC Version: NA (Server Power-off)\n");
      } else {
        ret = exp_get_ioc_fw_ver(ver);
        if (ret == FW_STATUS_SUCCESS) {
          printf("SCC IOC Version: %x.%x.%x.%x\n", ver[3], ver[2], ver[1], ver[0]);
        }
      }
      return ret;
    }
};

class IomIocComponent : public Component {
  public:
    IomIocComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version()
    {
      uint8_t ver[32] = {0};
      uint8_t status = 0;
      int ret = FW_STATUS_SUCCESS;

      //Have to check HOST power, if is off, shows NA
      pal_get_server_power(FRU_SLOT1, &status);
      if (status != SERVER_POWER_ON) {
        printf("IOM IOC Version: NA (Server Power-off)\n");
      } else {
        // Read Firwmare Versions of IOM IOC viacd MCTP
        ret = pal_get_iom_ioc_ver(ver);
        if (ret == FW_STATUS_SUCCESS) {
          printf("IOM IOC Version: %x.%x.%x.%x\n", ver[3], ver[2], ver[1], ver[0]);
        }
      }
      return ret;
    }
};

class SccConfig {
  public:
    SccConfig() {
      uint8_t sku = 0;

      static SccIocComponent sccioc("scc", "ioc");
      sku = pal_get_iom_type();
      if (sku == IOM_IOC) {
        static IomIocComponent iomioc("iom", "ioc");
      }
    }
};

SccConfig SCCconf;
