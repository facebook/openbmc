#pragma once
#include <string>
#include <unordered_map>
#include "signed_decoder.hpp"

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

enum {
  BIC = 0x02,
  CPUDVDD,
  CPUVDD,
  SOCVDD
};

enum {
  ALL_VENDOR = 0x00,
  ASPEED,
  RENESAS,
  INFINEON,
  MPS,
};

const signed_header_t cpudvdd_signed_info = {
  PLATFORM_NAME,
  JAVAISLAND,
  0x00,
  CPUDVDD,
  MPS
};

const signed_header_t cpuvdd_signed_info = {
  PLATFORM_NAME,
  JAVAISLAND,
  0x00,
  CPUVDD,
  MPS
};

const signed_header_t socvdd_signed_info = {
  PLATFORM_NAME,
  JAVAISLAND,
  0x00,
  SOCVDD,
  MPS
};

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
  }
};

} // namespace javaisland