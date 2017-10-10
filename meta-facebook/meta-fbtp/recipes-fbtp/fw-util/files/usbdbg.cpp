#include "fw-util.h"
#include <openbmc/ocp-dbg-lcd.h>

using namespace std;

class UsbDbgComponent : public Component {
  public:
    UsbDbgComponent(string fru, string comp)
      : Component(fru, comp) {}
    int update(string image) {
      return usb_dbg_update_fw((char *)image.c_str());
    }
};

class UsbDbgBlComponent : public Component {
  public:
    UsbDbgBlComponent(string fru, string comp)
      : Component(fru, comp) {}
    int update(string image) {
      return usb_dbg_update_boot_loader((char *)image.c_str());
    }
};

UsbDbgComponent usbdbg("mb", "usbdbgfw");
UsbDbgBlComponent usbdbgbl("mb", "usbdbgbl");
