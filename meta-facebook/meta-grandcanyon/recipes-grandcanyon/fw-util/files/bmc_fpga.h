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

// Grand Canyon 2.0 Altera FPGA Size is not same
// Support Different Altera FPGA Size attr setting
#define MAX10M25_RPD_SIZE                (0x5C000)
#define MAX10M25_CFM1_START_ADDR         (0x00064000)
#define MAX10M25_CFM1_END_ADDR           (0x000BFFFF)

#define MAX10M04_RPD_SIZE                (0x23000)
#define MAX10M04_CFM1_START_ADDR         (0x00027000)
#define MAX10M04_CFM1_END_ADDR           (0x00049FFF)

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
  CFM_IMAGE_1_M04,
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
    uint32_t rpd_image_size;
    bool is_valid_image(string image, bool force);
    int create_update_image(const string& image, string update_image);
    int update_fpga(string image, string update_image);
    int update_wrapper(const string& image, bool force);
    int get_ver_str(string& s);
  public:
    BmcFpgaComponent(const string& fru, const string& comp, uint8_t type, uint8_t _bus, uint8_t _addr, uint8_t _location)
      : Component(fru, comp), pld_type(type), bus(_bus), addr(_addr), location(_location), attr{}, rpd_image_size(0)
      {
        if(pld_type == MAX10_10M04) //[GC2.0]fpga chip type: MAX10M04
        {
          attr = { bus, addr, CFM_IMAGE_1_M04, 
                  MAX10M04_CFM1_START_ADDR,
                  MAX10M04_CFM1_END_ADDR,
                  ON_CHIP_FLASH_IP_CSR_BASE,
                  ON_CHIP_FLASH_IP_DATA_REG,
                  DUAL_BOOT_IP_BASE,
                  I2C_LITTLE_ENDIAN };
          rpd_image_size = MAX10M04_RPD_SIZE;
        }
        else //[GC1.0]fpga Default chip type: MAX10M25
        {
          attr = { bus, addr, CFM_IMAGE_1, 
                  MAX10M25_CFM1_START_ADDR,
                  MAX10M25_CFM1_END_ADDR,
                  ON_CHIP_FLASH_IP_CSR_BASE,
                  ON_CHIP_FLASH_IP_DATA_REG,
                  DUAL_BOOT_IP_BASE,
                  I2C_LITTLE_ENDIAN };
          rpd_image_size = MAX10M25_RPD_SIZE;
        }
      }
    int print_version();
    int update(const string& image);
    int fupdate(const string& image);
    int get_version(json& j) override;
};

#endif
