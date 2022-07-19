#include <switch.h>

using namespace std;

int PAXComponent::print_version()
{
  int ret;
  char ver[MAX_VALUE_LEN] = {0};
  string comp = this->component();

  if (comp.find("flash") != string::npos) {
    if(pal_check_switch_config()) {
      pal_print_pex_ver(_paxid);
    }
    return 0;
  }

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

  if (ret < 0)
    cout << "NA" << endl;
  else
    cout << string(ver) << endl;

  return ret;
}

int PAXComponent::update(string image)
{
  int ret;
  int i, max_retry = 3;
  bool use_gpio = false;

  string comp = this->component();

  if (comp.find("flash") != string::npos) {
    for (i = 1; i <= max_retry; i++) {
      if (i == max_retry) {
        ret = system("modprobe -r spi_aspeed > /dev/null");
        ret |= system("modprobe -r spi_gpio > /dev/null");
        ret |= system("modprobe spi_gpio > /dev/null");
	if (ret != 0) {
          syslog(LOG_WARNING, "Failed to switch SPI drivers");
	  goto bail;
	}

        spidev = "spi3.0";
        use_gpio = true;
      }
      ret = GPIOSwitchedSPIMTDComponent::update(image);
      if (ret == 0)
	break;
    }
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
      if(ret == -2) {
        cout << " Firmware update on " << comp << " is not supported  " << endl;
        return -1;
      } else {
        cout << image << " is not a valid file for " << comp << endl;
        return -1;
      }
    }
    syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());
    ret = pal_pax_fw_update(_paxid, image.c_str());
    if (ret == 0)
      syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
  }

bail:
  if (use_gpio) {
    ret = system("modprobe -r spi_gpio > /dev/null");
    ret |= system("modprobe spi_aspeed > /dev/null");
    ret |= system("modprobe spi_gpio > /dev/null");
    if (ret != 0)
      syslog(LOG_WARNING, "Failed to reload SPI drivers");
  }
  return ret;
}
