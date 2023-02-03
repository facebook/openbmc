#include "platform.hpp"

namespace pldm
{
namespace responder
{
namespace platform
{

std::string get_sensor_name(uint16_t id)
{
  static const std::map<uint16_t, std::string> sensor_map = {
    {0xFF, "SWB_SENSOR_BIC"},
    {0x10, "SWB_SENSOR_NIC_0"},
    {0x11, "SWB_SENSOR_NIC_1"},
    {0x12, "SWB_SENSOR_NIC_2"},
    {0x13, "SWB_SENSOR_NIC_3"},
    {0x14, "SWB_SENSOR_NIC_4"},
    {0x15, "SWB_SENSOR_NIC_5"},
    {0x16, "SWB_SENSOR_NIC_6"},
    {0x17, "SWB_SENSOR_NIC_7"},
    {0x18, "SWB_SENSOR_NIC_0_7"},
    {0x20, "SWB_SENSOR_E1S_0"},
    {0x21, "SWB_SENSOR_E1S_1"},
    {0x22, "SWB_SENSOR_E1S_2"},
    {0x23, "SWB_SENSOR_E1S_3"},
    {0x24, "SWB_SENSOR_E1S_4"},
    {0x25, "SWB_SENSOR_E1S_5"},
    {0x26, "SWB_SENSOR_E1S_6"},
    {0x27, "SWB_SENSOR_E1S_7"},
    {0x28, "SWB_SENSOR_E1S_8"},
    {0x29, "SWB_SENSOR_E1S_9"},
    {0x2A, "SWB_SENSOR_E1S_10"},
    {0x2B, "SWB_SENSOR_E1S_11"},
    {0x2C, "SWB_SENSOR_E1S_12"},
    {0x2D, "SWB_SENSOR_E1S_13"},
    {0x2E, "SWB_SENSOR_E1S_14"},
    {0x2F, "SWB_SENSOR_E1S_15"},
    {0x40, "SWB_SENSOR_PEX_0"},
    {0x41, "SWB_SENSOR_PEX_1"},
    {0x42, "SWB_SENSOR_PEX_2"},
    {0x43, "SWB_SENSOR_PEX_3"},
    {0x44, "SWB_SENSOR_PEX"},
    {0x50, "SWB_SENSOR_VR_0"},
    {0x51, "SWB_SENSOR_VR_1"},
    {0x52, "SWB_SENSOR_VR"},
    {0x60, "SWB_SENSOR_CPLD"},
    {0x62, "SWB_SENSOR_HSC"},
  };

  if (sensor_map.find(id) !=sensor_map.end())
    return sensor_map.at(id);
  else
    return "Unknown";
}

std::string get_state_message(uint8_t offset, uint8_t state)
{
  static const std::vector<std::vector<std::string>> eventstate_map = {
    {"unknown", "present", "not present"}, // sensorOffset = 0: presentence
    {"unknown", "normal",  "alert"},       // sensorOffset = 1: status
  };

  if (offset >= eventstate_map.size())
    return "offset not supported";
  else if (state >= eventstate_map[offset].size())
    return "state not supported";
  else
    return eventstate_map[offset][state];
}

} // namespace platform
} // namespace responder
} // namespace pldm
