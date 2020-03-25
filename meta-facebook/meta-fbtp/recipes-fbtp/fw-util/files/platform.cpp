#include <string>
#include "fw-util.h"
#include "bios.h"
#include "usbdbg.h"
#include "nic.h"
#include <linux/version.h>
#include <openbmc/pal.h>
#include <openbmc/misc-utils.h>

class FBTPBiosComponent : public BiosComponent {
  private:
    // SPI driver of 4.1 has SPI numbered starting from 1
    // while the upstream 5.x drivers have them numbered
    // from 0 like they should. :-(
    std::string get_spi_dev()
    {
      k_version_t cur_ver;
      cur_ver = get_kernel_version();
      if (cur_ver <= KERNEL_VERSION(4, 1, 51)) {
        return std::string("spi1.0");
      }
      return std::string("spi0.0");
    }

    int check_image(const char *path) override
    {
      std::string vers;
      uint8_t board_info, sku;
      const char *proj_tags[2] = {
        "F08_",
        "TP"
      };

      if (pal_get_platform_id(&board_info) < 0) {
        return -1;
      }
      sku = board_info & 0x1;
      if (extract_signature(path, vers)) {
        return -1;
      }
      if (vers.find(proj_tags[sku]) == 0) {
        return 0;
      }
      // Temporary workaround TODO. Remove once no longer required.
      // If BIOS is for SKU == 1, just make sure that the project
      // tag does not match sku == 0.
      if (sku == 1 && vers.find(proj_tags[0]) != 0) {
        return 0;
      }
      return -1;
    }
  public:
    FBTPBiosComponent(std::string fru, std::string comp) :
      BiosComponent(fru, comp, "bios0", get_spi_dev(), "BMC_BIOS_FLASH_CTRL", true, "") {}
};

// Set aliases for BMC components to MB components
AliasComponent bmc("mb", "bmc", "bmc", "bmc");
AliasComponent rom("mb", "rom", "bmc", "rom");
AliasComponent mb_fscd("mb", "fscd", "bmc", "fscd");
UsbDbgComponent usbdbg("mb", "usbdbgfw", "FBTP", 9, 0x60, false);
UsbDbgBlComponent usbdbgbl("mb", "usbdbgbl", 9, 0x60, 0x02);
FBTPBiosComponent bios("mb", "bios");
NicComponent nic("nic", "nic");
