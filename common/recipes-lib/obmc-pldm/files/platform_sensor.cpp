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

} // namespace platform
} // namespace responder
} // namespace pldm
