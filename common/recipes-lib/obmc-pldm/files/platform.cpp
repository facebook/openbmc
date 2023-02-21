#include "platform.hpp"
#include "pldm.h"

#include <stdexcept> // std::out_of_range
#include <stdio.h>   // printf
#include <syslog.h>

namespace pldm
{
namespace responder
{
namespace platform
{

Response Handler::platformEventMessage(const pldm_msg* request,
                                       size_t payloadLength)
{
  uint8_t formatVersion{};
  uint8_t tid{};
  uint8_t eventClass{};
  size_t offset{};

  auto rc = decode_platform_event_message_req(
    request, payloadLength, &formatVersion, &tid, &eventClass, &offset);

  if (rc != PLDM_SUCCESS)
    return CmdHandler::ccOnlyResponse(request, rc);

  if (eventClass == PLDM_HEARTBEAT_TIMER_ELAPSED_EVENT) {
    rc = PLDM_SUCCESS;
    // if (oemPlatformHandler)
    // {
    //     oemPlatformHandler->resetWatchDogTimer();
    // }
  } else {
    try {
      const auto& handlers = eventHandlers.at(eventClass);
      for (const auto& handler : handlers) {
        rc = handler(request, payloadLength, formatVersion, tid, offset);
        if (rc != PLDM_SUCCESS)
          return CmdHandler::ccOnlyResponse(request, rc);
      }
    } catch (const std::out_of_range& e) {
      return CmdHandler::ccOnlyResponse(request, PLDM_ERROR_INVALID_DATA);
    } catch (...) {
      return CmdHandler::ccOnlyResponse(request, 0xFF);
    }
  }
  pldm_response pldm_resp(PLDM_PLATFORM_EVENT_MESSAGE_RESP_BYTES);

  rc = encode_platform_event_message_resp(request->hdr.instance_id, rc,
                                          PLDM_EVENT_LOGGED, pldm_resp.msg());
  if (rc != PLDM_SUCCESS)
    return ccOnlyResponse(request, rc);

  return pldm_resp.get();
}

int Handler::sensorEvent(const pldm_msg* request, size_t payloadLength,
                         uint8_t /*formatVersion*/, uint8_t tid,
                         size_t eventDataOffset)
{
  uint16_t sensorId{};
  uint8_t eventClass{};
  size_t eventClassDataOffset{};
  auto eventData =
      reinterpret_cast<const uint8_t*>(request->payload) + eventDataOffset;
  auto eventDataSize = payloadLength - eventDataOffset;

  auto rc = decode_sensor_event_data(eventData, eventDataSize, &sensorId,
                                      &eventClass, &eventClassDataOffset);

  if (rc != PLDM_SUCCESS)
    return rc;

  auto eventClassData = reinterpret_cast<const uint8_t*>(request->payload) +
                        eventDataOffset + eventClassDataOffset;
  auto eventClassDataSize =
      payloadLength - eventDataOffset - eventClassDataOffset;

  if (eventClass == PLDM_STATE_SENSOR_STATE) {

    uint8_t sensorOffset{};
    uint8_t eventState{};
    uint8_t previousEventState{};

    rc = decode_state_sensor_data(eventClassData, eventClassDataSize,
                                  &sensorOffset, &eventState,
                                  &previousEventState);

    if (rc != PLDM_SUCCESS)
      return PLDM_ERROR;

    // handle PDR
    if (get_sensor_name(sensorId) != "UnSupport") {
      syslog(LOG_CRIT,
        "State Sensor: %s, %s",
        get_sensor_name(sensorId).c_str(),
        get_state_message(sensorOffset, eventState).c_str());
    }

  } else
    return PLDM_ERROR_INVALID_DATA;

  return PLDM_SUCCESS;
}

int Handler::pldmPDRRepositoryChgEvent(const pldm_msg* request,
                                       size_t payloadLength,
                                       uint8_t /*formatVersion*/,
                                       uint8_t /*tid*/, size_t eventDataOffset)
{
  uint8_t eventDataFormat{};
  uint8_t numberOfChangeRecords{};
  size_t dataOffset{};

  auto eventData =
      reinterpret_cast<const uint8_t*>(request->payload) + eventDataOffset;
  auto eventDataSize = payloadLength - eventDataOffset;

  auto rc = decode_pldm_pdr_repository_chg_event_data(
      eventData, eventDataSize, &eventDataFormat, &numberOfChangeRecords,
      &dataOffset);

  if (rc != PLDM_SUCCESS)
    return rc;

  // TODO: PDRRecordHandles
  //

  // TODO: fetchPDR(std::move(pdrRecordHandles))
  //

  return PLDM_SUCCESS;
}

} // namespace platform
} // namespace responder
} // namespace pldm
