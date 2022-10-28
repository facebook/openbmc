#ifndef _BMC_CPLD_H_
#define _BMC_CPLD_H_
#include "fw-util.h"
#include <openbmc/cpld.h>
#include "bic_fw.h"

using namespace std;

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00200020)
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG  (ON_CHIP_FLASH_IP_CSR_BASE + 0x0)
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG    (ON_CHIP_FLASH_IP_CSR_BASE + 0x4)
#define ON_CHIP_FLASH_USER_VER           (0x00200028)
#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)

// Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00200000)
#define CFM1_START_ADDR                  (0x00027000)
#define CFM1_END_ADDR                    (0x00049FFF)
#define CFM2_START_ADDR                  (0x00008000)
#define CFM2_END_ADDR                    (0x00063FFF)

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
    image_info check_image(const string& image, bool force);
    int update_cpld(const string& image, bool force);
    int get_ver_str(string& s);
  public:
    BmcCpldComponent(const string& fru, const string& comp, uint8_t type, uint8_t _bus, uint8_t _addr)
      : Component(fru, comp), pld_type(type), bus(_bus), addr(_addr),
        attr{bus, addr, CFM_IMAGE_1_M04, CFM1_START_ADDR, CFM1_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(string image);
    int fupdate(string image);
    int get_version(json& j) override;
};

#endif
