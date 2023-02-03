#include "fw-util.h"
#include <sstream>
#include <openbmc/pal.h>
#include <openbmc/aries_api.h>
#include <openbmc/libgpio.h>
#include "vr_fw.h"

const char * rev_id0 = "FAB_BMC_REV_ID0";
const char * rev_id1 = "FAB_BMC_REV_ID1";

class RetimerComponent : public Component {
  uint8_t bus = 0x6;
  uint8_t addr = 0xE0;
  uint8_t _channel = 0;

  public:
    RetimerComponent(
      const std::string& fru,
      const std::string& comp,
      const uint8_t channel)
      : Component(fru, comp), _channel(channel) {}

    int update(std::string image) {
      int ret = -1;

      ret = pal_control_mux_to_target_ch(bus, addr, _channel);
      if (ret) {
         return FW_STATUS_FAILURE;
      }

      ret = AriestFwUpdate(image.c_str());
      if (ret) {
         return FW_STATUS_FAILURE;
      }

      return FW_STATUS_SUCCESS;
    }

    int get_version(json& j) {
      int ret = -1;
      uint16_t ver[10] = {0};

      ret = pal_control_mux_to_target_ch(bus, addr, _channel);
      if (ret) {
        j["VERSION"] = "NA";
        return FW_STATUS_FAILURE;
      }

      ret = AriesGetFwVersion(ver);
      if (ret) {
        j["VERSION"] = "NA";
        return FW_STATUS_FAILURE;
      }

      std::stringstream ss;
      ss << +ver[0] << '.'
         << +ver[1] << '.'
         << +ver[2] << +ver[3];
      j["VERSION"] = ss.str();

      return FW_STATUS_SUCCESS;
    }
};

class RetimerSysConfig {
  public:
    RetimerSysConfig() {
      if (gpio_get_value_by_shadow(rev_id0) == 1 &&
          gpio_get_value_by_shadow(rev_id1) == 0) {
        static RetimerComponent rt0_comp("mb", "retimer0", 0);
        static RetimerComponent rt1_comp("mb", "retimer1", 1);
        static RetimerComponent rt2_comp("mb", "retimer2", 2);
        static RetimerComponent rt3_comp("mb", "retimer3", 3);
        static RetimerComponent rt4_comp("mb", "retimer4", 4);
        static RetimerComponent rt5_comp("mb", "retimer5", 5);
        static RetimerComponent rt6_comp("mb", "retimer6", 6);
        static RetimerComponent rt7_comp("mb", "retimer7", 7);
        static VrComponent rt_vr0("mb", "cpu0_rt_p0v9", "VR_CPU0_RETIMER_P0V9");
        static VrComponent rt_vr1("mb", "cpu1_rt_p0v9", "VR_CPU1_RETIMER_P0V9");
      }
      else if (gpio_get_value_by_shadow(rev_id0) == 0 &&
               gpio_get_value_by_shadow(rev_id1) == 1) {
        static RetimerComponent rt0_comp("mb", "retimer0", 0);
        static RetimerComponent rt4_comp("mb", "retimer4", 4);
      }
    }
};

RetimerSysConfig rtConfig;
