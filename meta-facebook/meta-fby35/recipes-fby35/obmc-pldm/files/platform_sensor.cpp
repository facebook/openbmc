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
    {0x210, "CPU_THROTTLE"},
    {0x251, "CPU_DEGRADE"},
  };

  if (sensor_map.find(id) !=sensor_map.end())
    return sensor_map.at(id);
  else
    return "Unknown";
}

std::string get_state_message(uint8_t offset, uint8_t state)
{
  static const std::vector<std::vector<std::string>> eventstate_map = {
    {"unknown", "normal", "throttle", "degraded"}, //stateSetID 0x0E
  };

  if (offset >= eventstate_map.size())
    return "offset not supported";
  else if (state >= eventstate_map[offset].size())
    return "state not supported";
  else
    return eventstate_map[offset][state];
}

std::string get_device_type(uint8_t type)
{
  return "Unknown";
}

std::string get_board_info(uint8_t id)
{
  return "Unknown";
}

std::string get_event_type(uint8_t type)
{
  return "Unknown";
}

void set_sensor_state_work(uint16_t id, uint8_t offset, uint8_t state)
{
  return;
}

bool is_record_event(uint16_t id, uint8_t offset, uint8_t state)
{
  return true;
}

} // namespace platform
} // namespace responder
} // namespace pldm
