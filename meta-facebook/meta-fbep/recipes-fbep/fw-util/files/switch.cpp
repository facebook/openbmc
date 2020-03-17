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
    int fupdate(string image) override;
};

int PAXComponent::print_version()
{
  int ret;
  char img_ver[MAX_VALUE_LEN] = {0};
  char cfg_ver[MAX_VALUE_LEN] = {0};

  cout << "PAX" << (int)_paxid;
  ret = pal_get_pax_version(_paxid, img_ver, cfg_ver);
  if (ret < 0) {
    cout << " IMG Version: NA, CFG Version: NA" << endl;;
  }
  else {
    cout << " IMG Version: " << string(img_ver)
         << ", CFG Version: " << string(cfg_ver) << endl;
  }

  return ret;
}

int PAXComponent::update(string image)
{
  return pal_pax_fw_update(_paxid, image.c_str());
}

int PAXComponent::fupdate(string image)
{
  return GPIOSwitchedSPIMTDComponent::update(image);
}

PAXComponent pax0("mb", "pax0", 0, "SEL_FLASH_PAX0");
PAXComponent pax1("mb", "pax1", 1, "SEL_FLASH_PAX1");
PAXComponent pax2("mb", "pax2", 2, "SEL_FLASH_PAX2");
PAXComponent pax3("mb", "pax3", 3, "SEL_FLASH_PAX3");
