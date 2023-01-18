#include <string>

namespace signed_info {

  const std::string PLATFORM_NAME = "Grand Teton";

  // board id
  enum {
    ALL_BOARD = 0x00,
    MB_BOARD  = 0x01,
    SCM_BOARD,
    SWB_BOARD,
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