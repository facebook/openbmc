#include "platform.hpp"

namespace pldm
{
namespace responder
{
namespace platform
{

std::string get_sensor_name(uint16_t id)
{
  return "Unknown";
}

std::string get_state_message(uint8_t offset, uint8_t state)
{
  return "UnSupport";
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

} // namespace platform
} // namespace responder
} // namespace pldm
