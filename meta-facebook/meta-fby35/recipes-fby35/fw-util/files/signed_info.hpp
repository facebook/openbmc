#pragma once
#include <string>
#include <unordered_map>
#include "signed_decoder.hpp"
#include <facebook/fby35_common.h>

const std::string PLATFORM_NAME = "Yosemite V3.5";

// board id
enum {
  ALL = 0x00,
  JAVAISLAND = 0x01,
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
  BIC_COMP = 0x02,
};

// vendor id
enum {
  ALL_VENDOR = 0x00,
  ASPEED,
  RENESAS,
  INFINEON,
  MPS,
  LATTICE,
  BROADCOM,
};

namespace pldm_signed_info {

  const std::unordered_map<std::string, uint8_t> board_map = {
    {"Java Island", JAVAISLAND}
  };

  const std::unordered_map<std::string, uint8_t> stage_map = {
    {"POC", POC},
    {"EVT", EVT},
    {"DVT", DVT},
    {"PVT", PVT},
    {"MP",  MP},
  };

  const std::unordered_map<std::string, uint8_t> vendor_map = {
    {"ast1030",      ASPEED},
  };

  const std::unordered_map<uint8_t, std::string> comp_str_t = {
    {BIC_COMP, "bic"}
  };

}

const signed_header_t ji_bic_comps = {
  PLATFORM_NAME,
  JAVAISLAND,
  POC,
  BIC_COMP, // component id
  ASPEED, // vendor id
};


namespace javaisland {

// component id
enum {
  BIC = 0x02,
  VR_CPUDVDD,
  VR_CPUVDD,
  VR_SOCVDD,
  RETIMER,
  VR_FBVDDP2,
  VR_1V2,
};

// vendor id
enum {
  ALL_VENDOR = 0x00,
  ASPEED,
  MPS,
  INFINEON,
  ASTERA,
  TI,
};

const signed_header_t base_signed_info = {
  PLATFORM_NAME,
  JAVAISLAND,
  0x00,
  0x00,
  0x00,
};

const signed_header_t mps_cpudvdd_signed_info(base_signed_info, VR_CPUDVDD, MPS);
const signed_header_t mps_cpuvdd_signed_info(base_signed_info, VR_CPUVDD, MPS);
const signed_header_t mps_socvdd_signed_info(base_signed_info, VR_SOCVDD, MPS);
const signed_header_t inf_cpudvdd_signed_info(base_signed_info, VR_CPUDVDD, INFINEON);
const signed_header_t mps_fbvddp2_signed_info(base_signed_info, VR_FBVDDP2, MPS);
const signed_header_t mps_1v2_signed_info(base_signed_info, VR_1V2, MPS);
const signed_header_t astera_retimer_signed_info(base_signed_info, RETIMER, ASTERA);
const signed_header_t ti_retimer_signed_info(base_signed_info, RETIMER, TI);

// CPLD address: 7'h0F, register: 0x07 (Board REV ID)
// bit4 determines retimer vendor, bit5 determines hsc/vr vendor
constexpr auto RETIMER_VENDOR_BIT = 4;
constexpr auto HSC_VR_VENDOR_BIT = 5;

inline signed_header_t get_retimer_signed_info(const uint8_t& fru) {
  int ret = fby35_common_get_sb_rev(fru); 
  if (ret < 0) {
    ret = 0;
  }
  if (GETBIT(ret, RETIMER_VENDOR_BIT)) {
    return ti_retimer_signed_info;
  } else {
    return astera_retimer_signed_info;
  }
}

const pldm_image_signed_info_map signed_info_map = {
  .board_id = {
    {"Java Island", JAVAISLAND}
  },
  .stage = {
    {"POC", POC},
    {"EVT", EVT},
    {"DVT", DVT},
    {"PVT", PVT},
    {"MP",  MP},
  },
  .component_id = {},
  .vendor_id = {
    {"ast1030", ASPEED},
    {"mp2894",  MPS},
    {"mp2898",  MPS},
    {"mpq8746", MPS},
    {"mp2988",  MPS},
    {"tda38741", INFINEON},
    {"pt4080l", ASTERA},
    {"ds160pt801", TI},
  }
};

} // namespace javaisland