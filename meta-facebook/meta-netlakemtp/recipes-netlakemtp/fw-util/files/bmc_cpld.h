#ifndef _BMC_CPLD_H_
#define _BMC_CPLD_H_
#include "fw-util.h"
#include <openbmc/cpld.h>

using namespace std;

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00200020)
#define ON_CHIP_FLASH_USER_VER           (0x00200028)
#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)

// Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00200000)
#define M04_CFM1_START_ADDR              (0x00027000)
#define M04_CFM1_END_ADDR                (0x00049FFF)
#define M08_CFM1_START_ADDR              (0x0002b000)
#define M08_CFM1_END_ADDR                (0x0004dFFF)
#define M25_CFM1_START_ADDR              (0x00064000)
#define M25_CFM1_END_ADDR                (0x000bFFFF)


typedef struct img_check {
  std::string new_path;
  bool result;
  bool sign;
} image_info;

enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_1,
  CFM_IMAGE_2,
  CFM_IMAGE_1_M04,
};

class BmcCpldComponent : public Component {
  uint8_t pld_type;
  uint8_t bus;
  uint8_t addr;
  altera_max10_attr_t attr;
  private:
    image_info check_image(string image, bool force);
    int update_cpld(string image, bool force, bool sign);
    int get_ver_str(string& s);
    int get_cpld_attr(altera_max10_attr_t& attr);
  public:
    BmcCpldComponent(const string& fru, const string& comp, uint8_t type, uint8_t _bus, uint8_t _addr)
      : Component(fru, comp), pld_type(type), bus(_bus), addr(_addr){}
    int print_version();
    void get_version(json& ver_json);
    int update(string image);
    int fupdate(string image);
    int update_process(string image, image_info image_sts, bool force);
};

#endif
