#include "fw-util.h"
#include "usbdbg.h"
#include <openbmc/ocp-dbg-lcd.h>

using namespace std;

UsbDbgComponent::UsbDbgComponent(string fru, string comp,
    uint8_t bus, uint8_t addr, uint8_t ioexp_addr) : Component(fru, comp)
{
  usb_dbg_init(bus, addr, ioexp_addr);
}

int UsbDbgComponent::update(string image)
{
  return usb_dbg_update_fw((char *)image.c_str(), 1);
}

UsbDbgBlComponent::UsbDbgBlComponent(string fru, string comp,
    uint8_t bus, uint8_t addr, uint8_t ioexp_addr) : Component(fru, comp)
{
  usb_dbg_init(bus, addr, ioexp_addr);
}

int UsbDbgBlComponent::update(string image)
{
  int ret = FW_STATUS_NOT_SUPPORTED;

  ret = usb_dbg_update_boot_loader((char *)image.c_str());
  if (ret > 0) {
    return FW_STATUS_SUCCESS;
  } else {
    return FW_STATUS_FAILURE;
  }
}

