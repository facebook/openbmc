#ifndef _BMC_FPGA_H_
#define _BMC_FPGA_H_
#include <string>
#include "fw-util.h"
#include <openbmc/cpld.h>

using namespace std;

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00200020)
#define ON_CHIP_FLASH_USER_VER           (0x00200028)
#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)

// Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00200000)

#define CFM1_START_ADDR                  (0x00064000)
#define CFM1_END_ADDR                    (0x000BFFFF)

#define BS_FPGA_ID                       (0x01)
#define UIC_FPGA_ID                      (0x02)

#define MAX10_RPD_SIZE                   (0x5C000)
#define MD5_OFFSET                       (0x0)
#define SIGNATURE_OFFSET                 (0x10)
#define IDENTIFY_OFFSET                  (0x20)

enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_1,
  CFM_IMAGE_2,
};

// indicate the FPGA fw location
enum {
  UIC_FPGA_LOCATION = 0,
  BS_FPGA_LOCATION = 1,
};

class BmcFpgaComponent : public Component {
  private:
    uint8_t pld_type;
    uint8_t bus;
    uint8_t addr;
    uint8_t location;
    altera_max10_attr_t attr;
    bool is_valid_image(string image, bool force);
    int create_update_image(string image, string update_image);
    int update_fpga(string image, string update_image);
    int update_wrapper(string image, bool force);
    int get_ver_str(string& s);
  public:
    BmcFpgaComponent(string fru, string comp, uint8_t type, uint8_t _bus, uint8_t _addr, uint8_t _location)
      : Component(fru, comp), pld_type(type), bus(_bus), addr(_addr), location(_location),
        attr{bus, addr, CFM_IMAGE_1, CFM1_START_ADDR, CFM1_END_ADDR, ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int print_version();
    int update(string image);
    int fupdate(string image);
    int get_version(json& j) override;
};

#endif
