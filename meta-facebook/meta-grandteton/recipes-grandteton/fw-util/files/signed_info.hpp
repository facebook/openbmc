#pragma once
#include <string>
#include <unordered_map>
#include <openbmc/pal_def.h>
#include <openbmc/pal_swb_sensors.h>
#include "signed_decoder.hpp"

namespace signed_info {

  const std::string PLATFORM_NAME = "Grand Teton";
  const std::string GTA_PLATFORM_NAME = "Artemis";

  // board id
  enum {
    ALL = 0x00,
    MB  = 0x01,
    SCM,
    SWB,
    VPDB,
  };

  // GTA board id
  enum {
    GTA_MB  = 0x00,
    GTA_MEB,
    GTA_ACB,
  };

  // stage id
  enum {
    POC = 0x00,
    EVT,
    DVT,
    PVT,
    MP,
  };

  // GTA stage id
  enum {
    GTA_POC = 0x00,
    GTA_EVT1,
    GTA_EVT2,
    GTA_DVT,
    GTA_PVT,
    GTA_MP,
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
    {"SwitchBoard", signed_info::SWB},
    {"ColterBay", signed_info::GTA_ACB},
    {"MooseCreek", signed_info::GTA_MEB}
  };

  const std::unordered_map<std::string, uint8_t> stage_map = {
    {"POC", signed_info::POC},
    {"EVT", signed_info::EVT},
    {"DVT", signed_info::DVT},
    {"PVT", signed_info::PVT},
    {"MP",  signed_info::MP},
    {"GTA_POC", signed_info::GTA_POC},
    {"GTA_EVT1", signed_info::GTA_EVT1},
    {"GTA_EVT2", signed_info::GTA_EVT2},
    {"GTA_DVT", signed_info::GTA_DVT},
    {"GTA_PVT", signed_info::GTA_PVT},
    {"GTA_MP",  signed_info::GTA_MP}
  };

  const std::unordered_map<std::string, uint8_t> vendor_map = {
    {"ast1030",      ASPEED},
    {"isl69259",     RENESAS},
    {"xdpe12284c",   INFINEON},
    {"mp2971",       MPS},
    {"LCMXO3-9400C", LATTICE},
    {"LCMXO3-4300C", LATTICE},
    {"pex89000",     BROADCOM},
    {"xdpe15284",    INFINEON}
  };

  const std::unordered_map<uint8_t, std::string> comp_str_t = {
    {VR0_COMP, "vr0"},
    {VR1_COMP, "vr1"},
    {BIC_COMP, "bic"},
    {PEX0_COMP, "pex0"},
    {PEX1_COMP, "pex1"},
    {PEX2_COMP, "pex2"},
    {PEX3_COMP, "pex3"},
    {CPLD_COMP, "cpld"}
  };

  const signed_header_t gt_swb_comps = {
    signed_info::PLATFORM_NAME,
    signed_info::SWB,
    signed_info::PVT,
    0x00, // component id
    0x00, // vendor id
  };

  const signed_header_t gta_acb_bic_comps = {
    signed_info::GTA_PLATFORM_NAME,
    signed_info::GTA_ACB,
    signed_info::GTA_DVT,
    0x00, // component id
    0x00, // vendor id
  };

  const signed_header_t gta_meb_bic_comps = {
    signed_info::GTA_PLATFORM_NAME,
    signed_info::GTA_MEB,
    signed_info::GTA_DVT,
    0x00, // component id
    0x00, // vendor id
  };

  const std::unordered_map<std::string, std::vector<uint8_t>> swb_nic_t = {
    {"SWB_NIC0", {SWB_SENSOR_TEMP_NIC_0, SWB_SENSOR_TEMP_NIC_OPTIC_0}},
    {"SWB_NIC1", {SWB_SENSOR_TEMP_NIC_1, SWB_SENSOR_TEMP_NIC_OPTIC_1}},
    {"SWB_NIC2", {SWB_SENSOR_TEMP_NIC_2, SWB_SENSOR_TEMP_NIC_OPTIC_2}},
    {"SWB_NIC3", {SWB_SENSOR_TEMP_NIC_3, SWB_SENSOR_TEMP_NIC_OPTIC_3}},
    {"SWB_NIC4", {SWB_SENSOR_TEMP_NIC_4, SWB_SENSOR_TEMP_NIC_OPTIC_4}},
    {"SWB_NIC5", {SWB_SENSOR_TEMP_NIC_5, SWB_SENSOR_TEMP_NIC_OPTIC_5}},
    {"SWB_NIC6", {SWB_SENSOR_TEMP_NIC_6, SWB_SENSOR_TEMP_NIC_OPTIC_6}},
    {"SWB_NIC7", {SWB_SENSOR_TEMP_NIC_7, SWB_SENSOR_TEMP_NIC_OPTIC_7}}
  };
}
