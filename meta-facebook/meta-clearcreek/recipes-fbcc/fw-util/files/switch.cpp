#include "fw-util.h"
#include "spiflash.h"
#include <syslog.h>
#include <openbmc/pal.h>
#include <switch.h>

using namespace std;

int PAXComponent::print_version()
{
  int ret;
  char ver[MAX_VALUE_LEN] = {0};
  string comp = this->component();
  uint8_t board_type = 0;

  pal_get_platform_id(&board_type);

  if((board_type & 0x07) == 0x3) {
    pal_print_pex_ver(_paxid);
    return 0;
  }
  else {
    if (comp.find("flash") != string::npos)
      return 0;

    cout << "PAX" << (int)_paxid;
    if (comp.find("bl2") != string::npos) {
      cout << " BL2 Version: ";
      ret = pal_get_pax_bl2_ver(_paxid, ver);
    } else if (comp.find("img") != string::npos) {
      cout << " IMG Version: ";
      ret = pal_get_pax_fw_ver(_paxid, ver);
    } else if (comp.find("cfg") != string::npos) {
      cout << " CFG Version: ";
      ret = pal_get_pax_cfg_ver(_paxid, ver);
    } else {
    return -1;
    }
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
  string comp = this->component();

  if (comp.find("flash") != string::npos) {
    ret = GPIOSwitchedSPIMTDComponent::update(image);
  } else {

    if (comp.find("bl2") != string::npos)
      ret = pal_check_pax_fw_type(PAX_BL2, image.c_str());
    else if (comp.find("img") != string::npos)
      ret = pal_check_pax_fw_type(PAX_IMG, image.c_str());
    else if (comp.find("cfg") != string::npos)
      ret = pal_check_pax_fw_type(PAX_CFG, image.c_str());
    else
      return -1;

    if (ret < 0) {
      cout << image << " is not a valid file for " << comp << endl;
      return -1;
    }
    syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
    ret = pal_pax_fw_update(_paxid, image.c_str());
    if (ret == 0)
      syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
  }

  return ret;
}
