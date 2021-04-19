#ifndef _BMC_CPLD_H_
#define _BMC_CPLD_H_
#include <string>
#include "fw-util.h"
#include <openbmc/cpld.h>
#include <facebook/bic.h>

using namespace std;

enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_1,
  CFM_IMAGE_2,
};

typedef struct image_check {
  std::string new_path;
  bool result;
} image_info;

class BmcCpldComponent : public Component {
  uint8_t pld_type;
  uint8_t bus;
  uint8_t addr;
  altera_max10_attr_t attr;
  private:
    image_info check_image(string image, bool force);
    int update_cpld(string image);
    int get_ver_str(string& s);
  public:
    BmcCpldComponent(string fru, string comp, uint8_t type, uint8_t _bus, uint8_t _addr)
      : Component(fru, comp), pld_type(type), bus(_bus), addr(_addr), 
        attr{bus, addr, CFM_IMAGE_1, CFM1_START_ADDR, CFM1_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(string image);
    int fupdate(string image);
    void get_version(json& j);
};

#endif
