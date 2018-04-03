#include "fw-util.h"
#include "usbdbg.h"
#include <cstdio>
#include <cstring>
#include <openbmc/ocp-dbg-lcd.h>
#include <openbmc/pal.h>

using namespace std;

class UsbDbgExtComponent : public UsbDbgComponent {
  public:
    UsbDbgExtComponent(string fru, string comp, uint8_t bus, uint8_t addr, uint8_t ioexp_addr)
      : UsbDbgComponent(fru, comp, bus, addr, ioexp_addr) {}
    int update(string image);
    int print_version();
};

class UsbDbgBlExtComponent : public UsbDbgBlComponent {
  public:
    UsbDbgBlExtComponent(string fru, string comp, uint8_t bus, uint8_t addr, uint8_t ioexp_addr)
      : UsbDbgBlComponent(fru, comp, bus, addr, ioexp_addr) {}
    int print_version();
};

int UsbDbgExtComponent::update(string image)
{
  return usb_dbg_update_fw((char *)image.c_str(), pal_is_mcu_working());
}

int UsbDbgExtComponent::print_version() {
  uint8_t ver[8];

  try {
    // Print MCU Version
    if (!pal_is_mcu_working() || usb_dbg_get_fw_ver(0x00, ver)) {
      printf("MCU Version: NA\n");
    }
    else {
      printf("MCU Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("MCU Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

int UsbDbgBlExtComponent::print_version() {
  uint8_t ver[8];

  try {
    // Print MCU Bootloader Version
    if (!pal_is_mcu_working() || usb_dbg_get_fw_ver(0x01, ver)) {
      printf("MCU Bootloader Version: NA\n");
    }
    else {
      printf("MCU Bootloader Version: v%x.%02x\n", ver[0], ver[1]);
    }
  } catch(string err) {
    printf("MCU Bootloader Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

// Register USB Debug Card components
UsbDbgExtComponent usbdbg("ocpdbg", "mcu", 13, 0x60, 0x4E);
UsbDbgBlExtComponent usbdbgbl("ocpdbg", "mcubl", 13, 0x60, 0x4E);
