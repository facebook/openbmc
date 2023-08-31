#include "platform.hpp"

namespace pldm
{
namespace responder
{
namespace platform
{

std::string get_sensor_name(uint16_t id)
{
  static const std::map<uint16_t, const std::string> sensor_map = {
    {0x01, "CB_SENSOR_ACCL_1"},
    {0x02, "CB_SENSOR_ACCL_2"},
    {0x03, "CB_SENSOR_ACCL_3"},
    {0x04, "CB_SENSOR_ACCL_4"},
    {0x05, "CB_SENSOR_ACCL_5"},
    {0x06, "CB_SENSOR_ACCL_6"},
    {0x07, "CB_SENSOR_ACCL_7"},
    {0x08, "CB_SENSOR_ACCL_8"},
    {0x09, "CB_SENSOR_ACCL_9"},
    {0x0A, "CB_SENSOR_ACCL_10"},
    {0x0B, "CB_SENSOR_ACCL_11"},
    {0x0C, "CB_SENSOR_ACCL_12"},
    {0x0D, "CB_SENSOR_ACCL_POWER_CABLE_1"},
    {0x0E, "CB_SENSOR_ACCL_POWER_CABLE_2"},
    {0x0F, "CB_SENSOR_ACCL_POWER_CABLE_3"},
    {0x10, "CB_SENSOR_ACCL_POWER_CABLE_4"},
    {0x11, "CB_SENSOR_ACCL_POWER_CABLE_5"},
    {0x12, "CB_SENSOR_ACCL_POWER_CABLE_6"},
    {0x13, "CB_SENSOR_ACCL_POWER_CABLE_7"},
    {0x14, "CB_SENSOR_ACCL_POWER_CABLE_8"},
    {0x15, "CB_SENSOR_ACCL_POWER_CABLE_9"},
    {0x16, "CB_SENSOR_ACCL_POWER_CABLE_10"},
    {0x17, "CB_SENSOR_ACCL_POWER_CABLE_11"},
    {0x18, "CB_SENSOR_ACCL_POWER_CABLE_12"},
  };

  if (sensor_map.find(id) !=sensor_map.end())
    return sensor_map.at(id);
  else
    return "Unknown";
}

std::string get_state_message(uint8_t offset, uint8_t state)
{
  static const std::vector <std::vector<std::string>> eventstate_map = {
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
