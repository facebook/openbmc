#include <string>

namespace signed_info {

  const std::string PLATFORM_NAME = "Grand Teton";

  // board id
  enum {
    MB_BOARD = 0x01,
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
  };

}