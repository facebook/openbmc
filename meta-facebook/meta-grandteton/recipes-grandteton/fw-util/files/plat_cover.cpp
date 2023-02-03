#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/pal_common.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/vr.h>
#include <openbmc/kv.h>
#include <syslog.h>
#include "vr_fw.h"
#include "bios.h"
#include "signed_decoder.hpp"
#include "signed_info.hpp"

using namespace std;

class palBiosComponent : public BiosComponent {
  private:
    string key;
    bool get_active_version(string& ver);
  public:
    palBiosComponent(const string &fru, const string &comp, const string &mtd,
                     const string &devpath, const string &dev, const string &shadow,
                     bool level, const string &verp) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp), key("cur_" + fru + "_bios_ver") {}
    int update_finish(void) override;
    int reboot(uint8_t fruid) override;
    int get_version(json& j) override;
};

bool palBiosComponent::get_active_version(string& ver) {
  char value[MAX_VALUE_LEN] = {0};
  bool ret = ((kv_get(key.c_str(), value, NULL, KV_FPERSIST) < 0) && (errno == ENOENT));
  ver = string(value);
  return ret && !ver.empty();
}

int palBiosComponent::update_finish(void) {
  string verstr;
  if (get_active_version(verstr)) { // no update before
    kv_set(key.c_str(), _ver_after_active.c_str(), 0, KV_FPERSIST);
  }
  sys().runcmd(string("/sbin/fw_setenv por_ls on"));
  sys().output << "To complete the upgrade, please perform 'power-util sled-cycle'" << endl;
  return 0;
}

int palBiosComponent::reboot(uint8_t) {
  return 0;
}

int palBiosComponent::get_version(json& j) {
  uint8_t ver[64] = {0};
  uint8_t fruid = 1;
  int end;

  j["PRETTY_COMPONENT"] = "BIOS";
  pal_get_fru_id((char *)_fru.c_str(), &fruid);
  if (!pal_get_sysfw_ver(fruid, ver)) {
    // BIOS version response contains the length at offset 2 followed by ascii string
    if ((end = 3+ver[2]) > (int)sizeof(ver)) {
      end = sizeof(ver);
    }
    string sver((char *)ver + 3);
    j["VERSION"] = sver.substr(0, end - 3);
    j["VERSION_ACTIVE"] = (get_active_version(sver)) ? string(j["VERSION"]):sver;
  } else {
    j["VERISON"] = "NA";
    j["VERSION_ACTIVE"] = "NA";
  }

  return 0;
}

class GTVrComponent : public VrComponent, public SignComponent {
  public:
    GTVrComponent(const string& fru, const string& comp, const string& name,
      signed_header_t sign_info): VrComponent(fru, comp, name), SignComponent(sign_info, fru) {}
    int update(string image) override;
    int fupdate(string image) override;
    int component_update(string image, bool force) { return VrComponent::update(image, force); }
};

int GTVrComponent::update(string image) {
  return signed_image_update(image, false);
}
int GTVrComponent::fupdate(string image) {
  return signed_image_update(image, true);
}

class fw_vr_config {
  public:
    fw_vr_config() {

      uint8_t source_id = 0;
      signed_header_t vr_info = {
        signed_info::PLATFORM_NAME,
        signed_info::MB,
        signed_info::DVT,
        signed_info::CPU0_VR_VCCIN,
        signed_info::VR_SOURCE_INDEX
      };

      get_comp_source(FRU_MB, MB_VR_SOURCE, &source_id);
      vr_info.vendor_id = signed_info::VR_SOURCE_INDEX + source_id;

      static GTVrComponent vr_cpu0_vccin("mb", "cpu0_vccin", "VR_CPU0_VCCIN/VCCFA_FIVRA",
                                        signed_header_t(vr_info, signed_info::CPU0_VR_VCCIN));

      static GTVrComponent vr_cpu0_faon("mb", "cpu0_faon", "VR_CPU0_VCCFAEHV/FAON",
                                        signed_header_t(vr_info, signed_info::CPU0_VR_FAON));

      static GTVrComponent vr_cpu0_vccd("mb", "cpu0_vccd", "VR_CPU0_VCCD",
                                        signed_header_t(vr_info, signed_info::CPU0_VR_VCCD));

      static GTVrComponent vr_cpu1_vccin("mb", "cpu1_vccin", "VR_CPU1_VCCIN/VCCFA_FIVRA",
                                        signed_header_t(vr_info, signed_info::CPU1_VR_VCCIN));

      static GTVrComponent vr_cpu1_faon("mb", "cpu1_faon", "VR_CPU1_VCCFAEHV/FAON",
                                        signed_header_t(vr_info, signed_info::CPU1_VR_FAON));

      static GTVrComponent vr_cpu1_vccd("mb", "cpu1_vccd", "VR_CPU1_VCCD",
                                        signed_header_t(vr_info, signed_info::CPU1_VR_VCCD));

      get_comp_source(FRU_VPDB, VPDB_BRICK_SOURCE, &source_id);
      if (source_id == THIRD_SOURCE) {
        signed_header_t vr_info = {
          signed_info::PLATFORM_NAME,
          signed_info::VPDB,
          signed_info::DVT,
          signed_info::BRICK,
          signed_info::ALL_VENDOR
        };
        static GTVrComponent vpdb_brick("vpdb", "brick", "VPDB_BRICK", vr_info);
      }
    }
};

palBiosComponent bios("mb", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc",
                      "1e631000.spi", "FM_BMC_MUX_CS_SPI_SEL_0", true, "(F0T_)(.*)");
fw_vr_config _fw_vr_config;
