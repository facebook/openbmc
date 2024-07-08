#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include <syslog.h>
#include "vr_fw.h"
#include "nic_ext.h"
#include "usbdbg.h"
#include <openbmc/aries_common.h>


#define FRU_NIC0   (4)

using namespace std;

//NIC Component
NicExtComponent nic0("nic0", "nic0", "nic0_fw_ver", FRU_NIC0, 0);

//VR Component
//CPU0
VrComponent vr_cpu0_vcore0("mb", "cpu0_vcore0", "VR_CPU0_VCORE0/SOC");
VrComponent vr_cpu0_vcore1("mb", "cpu0_vcore1", "VR_CPU0_VCORE1/PVDDIO");
VrComponent vr_cpu0_pvdd11("mb", "cpu0_pvdd11", "VR_CPU0_PVDD11");

//CPLD Component
class CpldComponent : public Component {
  uint8_t pld_type;
  i2c_attr_t attr;
  int _update(const char *path, i2c_attr_t attr);

  public:
    CpldComponent(const std::string &fru, const std::string &comp, uint8_t type, uint8_t bus, uint8_t addr,
      int (*cpld_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t))
      : Component(fru, comp), pld_type(type), attr{bus, addr, cpld_xfer} {}

    static CpldComponent createCbCpld() {
        if (is_cpld_evt()) {
            return CpldComponent("cb", "cb_cpld", LCMXO3_2100C, 9, 0x40, nullptr);
        } else {
            return CpldComponent("cb", "cb_cpld", LCMXO3_2100C, 11, 0x40, nullptr);
        }
    }

    static int check_cpld_address(uint8_t bus, uint8_t addr);
    static bool is_cpld_evt();

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

int CpldComponent::update(string image) {
  return _update(image.c_str(), attr);
}

int CpldComponent::fupdate(string image) {
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
  j["PRETTY_COMPONENT"] = comp;


  if (CpldComponent::check_cpld_address(attr.bus_id, attr.slv_addr << 1)) {
      sprintf(strbuf, "NA");
      j["VERSION"] = string(strbuf);
      return FW_STATUS_SUCCESS;
  }

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

int CpldComponent::check_cpld_address(uint8_t bus, uint8_t addr)
{
  int fd = 0, ret = -1;
  uint8_t tlen, rlen;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  fd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
  if (fd < 0) {
    return ret;
  }

  tbuf[0] = 0xC0;
  tbuf[1] = 0x00;
  tbuf[2] = 0x00;
  tbuf[3] = 0x00;

  tlen = 4;
  rlen = 4;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  i2c_cdev_slave_close(fd);

  return ret;
}

bool CpldComponent::is_cpld_evt() {
    // Check if the I2C bus 9 has the address 0x40
    if (!CpldComponent::check_cpld_address(9, 0x40 << 1))
        return true;
    else
        return false;
}

CpldComponent mb_cpld("mb", "mb_cpld", LCMXO3LF_4300C, 5, 0x40, nullptr);
CpldComponent scm_cpld("scm", "scm_cpld", LCMXO3_2100C, 15, 0x40, nullptr);

//Retimer Component
class RetimerComponent : public Component {
  int _bus = 0;
  int addr = 0x24;

  public:
    RetimerComponent(
      const std::string& fru,
      const std::string& comp,
      const int bus)
      : Component(fru, comp), _bus(bus) {}

    int update(std::string image) {
      int ret = -1;

      ret = AriestFwUpdate(_bus, addr, image.c_str());

      if (ret) {
        return FW_STATUS_FAILURE;
      }
      std::cout << "To active the upgrade of Retimer, please perform 'power-util mb cycle'" << std::endl;
      return FW_STATUS_SUCCESS;
    }

    int get_version(json& j) {
      int slaveaddr = 24;
      std::stringstream pathStream;
      pathStream << "/sys/kernel/debug/pt5161l/" << _bus
                                                 << "-"
                                                 << std::setw(4)
                                                 << std::setfill('0')
                                                 << slaveaddr << "/fw_ver";
      std::string filePath = pathStream.str();
    //  std::cout << "File Path: " << filePath << std::endl;

      // Open the file
      std::ifstream file(filePath);

      // Check if the file is open
      if (file.is_open()) {
          std::string version;
          std::getline(file, version);
          if( !version.empty() || !file.bad() ) {
            file.close();
            j["VERSION"] = version;
            return FW_STATUS_SUCCESS;
          }
          file.close();
      }

      j["VERSION"] = "NA";
      return FW_STATUS_SUCCESS;
    }
};

RetimerComponent rt0_comp("mb", "retimer0", 12);
RetimerComponent rt1_comp("mb", "retimer1", 21);


class component_config {
public:
    component_config() {
        static CpldComponent cb_cpld = CpldComponent::createCbCpld();
    }
};

component_config comp_config;

