#pragma once

#include "libpldm/platform.h"
#include "libpldm/states.h"
#include "handler.hpp"
#include <stdint.h>
#include <string>
#include <map>

namespace pldm
{
namespace responder
{
namespace platform
{

// using EffecterId = uint16_t;
using EventType = uint8_t;
using EventHandler = std::function<int(
    const pldm_msg* request, size_t payloadLength, uint8_t formatVersion,
    uint8_t tid, size_t eventDataOffset)>;
using EventHandlers = std::vector<EventHandler>;
using EventMap = std::map<EventType, EventHandlers>;

class Handler : public CmdHandler
{
  public:
    Handler() {

      handlers.emplace(PLDM_PLATFORM_EVENT_MESSAGE,
        [this](const pldm_msg* request, size_t payloadLength) {
          return this->platformEventMessage(request, payloadLength);
        });

      handlers.emplace(PLDM_SET_STATE_EFFECTER_STATES,
        [this](const pldm_msg* request, size_t payloadLength) {
          return this->set_state_effecter_states(request, payloadLength);
        });

      // Default handler for PLDM Events
      eventHandlers[PLDM_SENSOR_EVENT].emplace_back(
        [this](const pldm_msg* request, size_t payloadLength,
              uint8_t formatVersion, uint8_t tid, size_t eventDataOffset) {
          return this->sensorEvent(request, payloadLength, formatVersion, tid, eventDataOffset);
        });
      eventHandlers[PLDM_PDR_REPOSITORY_CHG_EVENT].emplace_back(
        [this](const pldm_msg* request, size_t payloadLength,
              uint8_t formatVersion, uint8_t tid, size_t eventDataOffset) {
          return this->pldmPDRRepositoryChgEvent(request, payloadLength,
                                                formatVersion, tid, eventDataOffset);
        });
    }

    /** @brief Handler for PlatformEventMessage
     *
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request payload length
     *  @return Response - PLDM Response message
     */
    Response platformEventMessage(const pldm_msg* request, size_t payloadLength);

    /** @brief Handler for Set_state_effecter_states
     *
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request payload length
     *  @return Response - PLDM Response message
     */
    Response set_state_effecter_states(const pldm_msg* request, size_t payloadLength);

  protected:
    /** @brief Handler for event class Sensor event
     *
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request payload length
     *  @param[in] formatVersion - Version of the event format
     *  @param[in] tid - Terminus ID of the event's originator
     *  @param[in] eventDataOffset - Offset of the event data in the request
     *                               message
     *  @return PLDM completion code
     */
    int sensorEvent(const pldm_msg* request, size_t payloadLength,
                    uint8_t formatVersion, uint8_t tid, size_t eventDataOffset);

    /** @brief Handler for pldmPDRRepositoryChgEvent
     *
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request payload length
     *  @param[in] formatVersion - Version of the event format
     *  @param[in] tid - Terminus ID of the event's originator
     *  @param[in] eventDataOffset - Offset of the event data in the request
     *                               message
     *  @return PLDM completion code
     */
    int pldmPDRRepositoryChgEvent(const pldm_msg* request, size_t payloadLength,
                                  uint8_t formatVersion, uint8_t tid,
                                  size_t eventDataOffset);

    /** @brief map of PLDM event type to EventHandlers
     *
     */
    EventMap eventHandlers;
};

std::string get_sensor_name(uint16_t);
std::string get_state_message(uint8_t, uint8_t);
std::string get_device_type(uint8_t);
std::string get_board_info(uint8_t);
std::string get_event_type(uint8_t);

} // namespace platform
} // namespace responder
} // namespace pldm
