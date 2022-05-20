#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>
#include <openbmc/kv.h>
#include "fw-util.h"

using namespace std;

class CpldComponent : public Component {
  uint8_t pld_type;
  i2c_attr_t attr;
  int _update(const char *path, uint8_t is_signed);

  public:
    CpldComponent(const string &fru, const string &comp, uint8_t type, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_type(type), attr{bus, addr} {}
    int update(string image);
    int fupdate(string image);
    int print_version();
};

int CpldComponent::_update(const char *path, uint8_t is_signed) {
  int ret = -1;
  string comp = this->component();

  syslog(LOG_CRIT, "Component %s%s upgrade initiated", comp.c_str(), is_signed? "": " force");
  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    printf("Cannot open i2c!\n");
    return ret;
  }

  ret = cpld_program((char *)path, NULL, false);
  cpld_intf_close();
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
    return ret;
  }

  syslog(LOG_CRIT, "Component %s%s upgrade completed", comp.c_str(), is_signed? "": " force");
  return ret;
}

int CpldComponent::update(string image) {
  return _update(image.c_str(), 1);
}

int CpldComponent::fupdate(string image) {
  return _update(image.c_str(), 0);
}

int CpldComponent::print_version() {
  int ret = -1;
  uint8_t ver[4];
  char strbuf[MAX_VALUE_LEN];
  string comp = this->component();

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

  transform(comp.begin(), comp.end(),comp.begin(), ::toupper);
  sys().output << comp << " Version: " << string(strbuf) << endl;

  return 0;
}

CpldComponent mb_cpld("mb", "cpld", LCMXO3_9400C, 7, 0x40);
