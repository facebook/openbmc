#include "platform.hpp"
#include "pldm.h"

#include <stdexcept> // std::out_of_range
#include <stdio.h>   // printf
#include <syslog.h>
#include <cstring>

enum EFFECTER_ID {
  EFFECTER_ID_NOTIFY_TO_ADDSEL = 0x0005,
};

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

Response Handler::set_state_effecter_states(const pldm_msg* request,
                                            size_t payloadLength)
{
  uint8_t effecter_id = (request->payload[1] << 8 | request->payload[0]);
  uint8_t comp_effecter_count = request->payload[2];
  set_effecter_state_field field[8]{};
  memcpy(field, &request->payload[3], sizeof(set_effecter_state_field) * 8);

  if (comp_effecter_count < 0x01 || comp_effecter_count > 0x08) {
    return CmdHandler::ccOnlyResponse(request, PLDM_ERROR_INVALID_DATA);
  }

  switch (effecter_id) {
    case EFFECTER_ID_NOTIFY_TO_ADDSEL:
      {
        if (comp_effecter_count != 0x03) {
          return CmdHandler::ccOnlyResponse(request, PLDM_ERROR_INVALID_DATA);
        }

        bool is_alert_event = ((field[2].effecter_state & 0x80) ? true : false);
        uint8_t device_type = field[0].effecter_state;
        uint8_t board_info = field[1].effecter_state;
        uint8_t event_type = field[2].effecter_state & 0x7F;
        if (get_device_type(device_type) != "Unknown" && get_board_info(board_info) != "Unknown" &&
			get_event_type(event_type) != "Unknown") {
	  if (get_event_type(event_type) == "UNUSE") {
	    syslog(LOG_CRIT, "Device type: %s (0x%x), board info: %s (0x%x) %s event", get_device_type(device_type).c_str(),
			    device_type, get_board_info(board_info).c_str(), board_info, (is_alert_event ? "Assert" : "Deassert"));
	  } else {
            syslog(LOG_CRIT, "Device type: %s (0x%x), board info: %s (0x%x), event type: %s (0x%x) %s event",
			    get_device_type(device_type).c_str(), device_type, get_board_info(board_info).c_str(),
			    board_info, get_event_type(event_type).c_str(), event_type, (is_alert_event ? "Assert" : "Deassert"));
	  }
        }
      }
      break;
    default:
      return CmdHandler::ccOnlyResponse(request, PLDM_ERROR_INVALID_DATA);
  }

  return CmdHandler::ccOnlyResponse(request, PLDM_SUCCESS);
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
  } else if (eventClass == PLDM_NUMERIC_SENSOR_STATE) {

    uint8_t eventState{};
    uint8_t previousEventState{};
    uint8_t sensorDataSize{};
    uint32_t presentReading{};

    rc = decode_numeric_sensor_data(eventClassData, eventClassDataSize,
                                    &eventState, &previousEventState,
                                    &sensorDataSize, &presentReading);

    if (rc != PLDM_SUCCESS)
      return PLDM_ERROR;

    // handle PDR
    if (get_sensor_name(sensorId) != "UnSupport") {
      syslog(LOG_CRIT,
        "Numberic Sensor: %s, eventState: 0x%X, sensorData: 0x%X",
        get_sensor_name(sensorId).c_str(),
        eventState, presentReading);
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
