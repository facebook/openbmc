#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <openbmc/cpld.h>
#include <openbmc/kv.h>
#include <syslog.h>

using namespace std;

//CPLD fwupdate
class CpldComponent : public Component {
  uint8_t pld_type;
  i2c_attr_t attr;
  int _update(const char *path, i2c_attr_t attr);

  public:
    CpldComponent(const std::string &fru, const std::string &comp, uint8_t type, uint8_t bus, uint8_t addr,
      int (*cpld_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t))
      : Component(fru, comp), pld_type(type), attr{bus, addr, cpld_xfer} {}
    int update(std::string image) override;
    int fupdate(std::string image) override;
    int get_version(json& j) override;
};

int CpldComponent::_update(const char *path, i2c_attr_t attr ) {
  int ret = -1;
  string comp = this->component();
  string fru  = this->fru();

  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    cerr << "Cannot open i2c!" << endl;
    goto error_exit;
  }

  ret = cpld_program((char *)path, NULL, false);
  cpld_intf_close();
  if (ret) {
    cerr << "Error Occur at updating CPLD FW!" << endl;
    goto error_exit;
  }

error_exit:
  return ret;
}

int CpldComponent::update(const string image) {
  return _update(image.c_str(), attr);
}

int CpldComponent::fupdate(const string image) {
  return _update(image.c_str(), attr);
}


int CpldComponent::get_version(json& j) {
  int ret = -1;
  uint8_t ver[4];
  char strbuf[MAX_VALUE_LEN];
  string comp = this->component();
  string fru  = this->fru();
  transform(comp.begin(), comp.end(),comp.begin(), ::toupper);
  transform(fru.begin(), fru.end(),fru.begin(), ::toupper);
  j["PRETTY_COMPONENT"] = fru + " " + comp;

  if (!cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    ret = cpld_get_ver((uint32_t *)ver);
    cpld_intf_close();
  }

  if (ret) {
    sprintf(strbuf, "NA");
  } else {
    sprintf(strbuf, "%02X%02X%02X%02X", ver[0], ver[1], ver[2], ver[3]);
    kv_set(_component.c_str(), strbuf, 0, 0);
  }
  j["VERSION"] = string(strbuf);
  return FW_STATUS_SUCCESS;
}

//CPLD Component
CpldComponent cmm_cpld("cmm", "cpld", LCMXO3LF_4300C, 1, 0x44, nullptr);
CpldComponent scm_cpld("scm", "cpld", LCMXO3_2100C, 15, 0x40, nullptr);
CpldComponent fcb_top1_cpld("fcb", "top1_cpld", LCMXO3_9400C, 16, 0x70, nullptr);
CpldComponent fcb_top0_cpld("fcb", "top0_cpld", LCMXO3_9400C, 17, 0x70, nullptr);
CpldComponent fcb_mid1_cpld("fcb", "mid1_cpld", LCMXO3_9400C, 18, 0x70, nullptr);
CpldComponent fcb_mid0_cpld("fcb", "mid0_cpld", LCMXO3_9400C, 19, 0x70, nullptr);
CpldComponent fcb_bot1_cpld("fcb", "bot1_cpld", LCMXO3_9400C, 20, 0x70, nullptr);
CpldComponent fcb_bot0_cpld("fcb", "bot0_cpld", LCMXO3_9400C, 21, 0x70, nullptr);
