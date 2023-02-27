#include <string>
#include <unordered_map>
#include <openbmc/pal_def.h>

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

namespace pldm_signed_info {
  enum {
    ALL_VENDOR = 0x00,
    ASPEED,
    RENESAS,
    INFINEON,
    MPS,
    LATTICE,
    BROADCOM,
  };

  const std::unordered_map<std::string, uint8_t> board_map = {
    {"SwitchBoard", signed_info::SWB}
  };

  const std::unordered_map<std::string, uint8_t> stage_map = {
    {"POC", signed_info::POC},
    {"EVT", signed_info::EVT},
    {"DVT", signed_info::DVT},
    {"PVT", signed_info::PVT},
    {"MP",  signed_info::MP}
  };

  const std::unordered_map<std::string, uint8_t> vendor_map = {
    {"aspeed",   ASPEED},
    {"renesas",  RENESAS},
    {"Infineon", INFINEON},
    {"mps",      MPS},
    {"lattice",  LATTICE},
    {"broadcom", BROADCOM}
  };

  const signed_header_t gt_swb_comps = {
    signed_info::PLATFORM_NAME,
    signed_info::SWB,
    signed_info::PVT,
    0x00, // component id
    0x00, // vendor id
  };
}