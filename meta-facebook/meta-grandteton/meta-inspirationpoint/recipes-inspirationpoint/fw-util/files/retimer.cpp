#include "fw-util.h"
#include <sstream>
#include <openbmc/pal.h>
#include <openbmc/aries_api.h>

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

RetimerComponent rt0_comp("mb", "retimer0", 0);
RetimerComponent rt1_comp("mb", "retimer1", 4);
