#ifndef _BMC_CPLD_H_
#define _BMC_CPLD_H_
#include <string>
#include "fw-util.h"
#include <openbmc/cpld.h>

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00200020)
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG  (ON_CHIP_FLASH_IP_CSR_BASE + 0x0)
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG    (ON_CHIP_FLASH_IP_CSR_BASE + 0x4)
#define ON_CHIP_FLASH_USER_VER           (0x00200028)
#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)

// Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00200000)
#define CFM0_START_ADDR                  (0x00064000)
#define CFM0_END_ADDR                    (0x000BFFFF)

enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_0,
  CFM_IMAGE_1,
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
  int get_cpld_version(uint8_t *ver);
  private:
    image_info check_image(std::string image, bool force);
  public:
    BmcCpldComponent(std::string fru, std::string board, std::string comp, uint8_t type, uint8_t _bus, uint8_t _addr)
      : Component(fru, board, comp), pld_type(type), bus(_bus), addr(_addr), attr{bus, addr, CFM_IMAGE_0, CFM0_START_ADDR, CFM0_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(std::string image);
    int fupdate(std::string image);
};

#endif
