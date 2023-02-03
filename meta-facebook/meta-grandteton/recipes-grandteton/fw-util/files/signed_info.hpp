#include <string>

namespace signed_info {

  const std::string PLATFORM_NAME = "Grand Teton";

  // board id
  enum {
    ALL = 0x00,
    MB  = 0x01,
    SCM,
    SWB,
    VPDB,
  };

  // stage id
  enum {
    POC = 0x00,
    EVT,
    DVT,
    PVT,
    MP,
  };

  // component id
  enum {
    CPLD = 0x00,
    CPU0_VR_VCCIN,
    CPU0_VR_FAON,
    CPU0_VR_VCCD,
    CPU1_VR_VCCIN,
    CPU1_VR_FAON,
    CPU1_VR_VCCD,
    BRICK,
  };

  // vendor id
  enum {
    ALL_VENDOR      = 0x00,
    VR_SOURCE_INDEX = 0x01,
    RENESAS         = VR_SOURCE_INDEX,
    INFINEON,
    MPS,
  };

}