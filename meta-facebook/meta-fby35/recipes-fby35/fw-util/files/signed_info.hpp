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
