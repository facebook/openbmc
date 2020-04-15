#include "fw-util.h"
#include "spiflash.h"
#include <openbmc/pal.h>

using namespace std;

class PAXComponent : public GPIOSwitchedSPIMTDComponent {
  uint8_t _paxid;
  public:
    PAXComponent(string fru, string comp, uint8_t paxid, std::string shadow)
      : GPIOSwitchedSPIMTDComponent(fru, comp, "switch0", "spi1.0", shadow, true), _paxid(paxid) {}
    int print_version() override;
    int update(string image) override;
};

int PAXComponent::print_version()
{
  int ret;
  char ver[MAX_VALUE_LEN] = {0};

  if (_component.find("flash") != string::npos)
    return 0;

  cout << "PAX" << (int)_paxid;
  if (_component.find("bl2") != string::npos) {
    cout << " BL2 Version: ";
    ret = pal_get_pax_bl2_ver(_paxid, ver);
  } else if (_component.find("img") != string::npos) {
    cout << " IMG Version: ";
    ret = pal_get_pax_fw_ver(_paxid, ver);
  } else if (_component.find("cfg") != string::npos) {
    cout << " CFG Version: ";
    ret = pal_get_pax_cfg_ver(_paxid, ver);
  } else {
    return -1;
  }

  if (ret < 0)
    cout << "NA" << endl;
  else
    cout << string(ver) << endl;

  return ret;
}

int PAXComponent::update(string image)
{
  int ret;

  if (_component.find("flash") != string::npos) {
    ret = GPIOSwitchedSPIMTDComponent::update(image);
  } else {

    if (_component.find("bl2") != string::npos)
      ret = pal_check_pax_fw_type(PAX_BL2, image.c_str());
    else if (_component.find("img") != string::npos)
      ret = pal_check_pax_fw_type(PAX_IMG, image.c_str());
    else if (_component.find("cfg") != string::npos)
      ret = pal_check_pax_fw_type(PAX_CFG, image.c_str());
    else
      return -1;

    if (ret < 0) {
      cout << image << " is not a valid file for " << _component << endl;
      return -1;
    }
    ret = pal_pax_fw_update(_paxid, image.c_str());
  }

  if (ret == 0)
    pal_clear_pax_cache(_paxid);

  return ret;
}

PAXComponent pax0_fw("mb", "pax0-bl2", 0, "");
PAXComponent pax0_bl("mb", "pax0-img", 0, "");
PAXComponent pax0_cfg("mb", "pax0-cfg", 0, "");
PAXComponent pax0_flash("mb", "pax0-flash", 0, "SEL_FLASH_PAX0");
PAXComponent pax1_fw("mb", "pax1-bl2", 1, "");
PAXComponent pax1_bl("mb", "pax1-img", 1, "");
PAXComponent pax1_cfg("mb", "pax1-cfg", 1, "");
PAXComponent pax1_flash("mb", "pax1-flash", 1, "SEL_FLASH_PAX1");
PAXComponent pax2_fw("mb", "pax2-bl2", 2, "");
PAXComponent pax2_bl("mb", "pax2-img", 2, "");
PAXComponent pax2_cfg("mb", "pax2-cfg", 2, "");
PAXComponent pax2_flash("mb", "pax2-flash", 2, "SEL_FLASH_PAX2");
PAXComponent pax3_fw("mb", "pax3-bl2", 3, "");
PAXComponent pax3_bl("mb", "pax3-img", 3, "");
PAXComponent pax3_cfg("mb", "pax3-cfg", 3, "");
PAXComponent pax3_flash("mb", "pax3-flash", 3, "SEL_FLASH_PAX3");
