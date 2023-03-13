#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <openbmc/pal.h>
#include <openbmc/pal_common.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/vr.h>
#include "vr_fw.h"
#include "signed_decoder.hpp"
#include "signed_info.hpp"

using namespace std;

class VpdbVrComponent : public VrComponent, public SignComponent {
  public:
    VpdbVrComponent(const string& fru, const string& comp, const string& name,
      signed_header_t sign_info): VrComponent(fru, comp, name), SignComponent(sign_info, fru) {}
    int update(string image) override;
    int fupdate(string image) override;
    int component_update(string image, bool force) { return VrComponent::update(image, force); }
};

int VpdbVrComponent::update(string image) {
  return signed_image_update(image, false);
}
int VpdbVrComponent::fupdate(string image) {
  return signed_image_update(image, true);
}

class vpdb_fw_config {
  public:
    vpdb_fw_config() {
      uint8_t source_id = 0;

      get_comp_source(FRU_VPDB, VPDB_BRICK_SOURCE, &source_id);
      if (source_id == THIRD_SOURCE) {
        signed_header_t vr_info = {
          signed_info::PLATFORM_NAME,
          signed_info::VPDB,
          signed_info::DVT,
          signed_info::BRICK,
          signed_info::ALL_VENDOR
        };
        static VpdbVrComponent vpdb_brick("vpdb", "brick", "VPDB_BRICK", vr_info);
      }
    }
};

vpdb_fw_config _vpdb_fw_config;
