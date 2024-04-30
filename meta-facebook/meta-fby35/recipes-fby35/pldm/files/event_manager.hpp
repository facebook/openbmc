/*
 * Copyright 2024-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

#include <libpldm/base.h>

#include <vector>
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

/**
 * @brief SBMR defines an oemEvent of value 0xFA for the CPEREvent event class, 
 * until a new standard value is defined in the DSP0248 specification.
 */
#define PLDM_OEM_EVENT_CLASS_0xFA 0xFA

/**
 * @brief Satellite MC EID
 */
#define DEFAULT_SATMC_EID 0xF0

#define CPER_DUMP_PATH "/mnt/data/faultlog/cper"

namespace pldm
{
namespace platform_mc
{

using Request = std::vector<uint8_t>;
using Response = std::vector<uint8_t>;

/**
 * @brief EventManager
 *
 * This class manages PLDM events from terminus. The function includes providing
 * the API for process event data and using syslog API to log the event.
 *
 */
class EventManager
{
  public:
    EventManager(const EventManager&) = delete;
    EventManager(EventManager&&) = delete;
    EventManager& operator=(const EventManager&) = delete;
    EventManager& operator=(EventManager&&) = delete;
    virtual ~EventManager() = default;
    explicit EventManager(){};

    /** @brief Send eventMessageBufferSize command to terminus to obtain the 
     * maximum size of an event message from the terminus to the receiver.
     * 
     *  @param[in] payloadId - Payload ID used to know where the command needs to be sent
     *  @param[out] terminusBufferSize - Size of terminus buffer
     * 
     *  @return PLDM completion code
     */
    int eventMessageBufferSize(uint8_t payloadId, uint16_t& terminusBufferSize);

    /** @brief Handle platform event data based on event class
     *
     *  @param[in] payloadId - Payload ID used to know where the command from
     *  @param[in] tid - Terminus ID of the event's originator
     *  @param[in] eventClass - Event class
     *  @param[in] eventData - Event data
     *  @param[in] eventDataSize - Size of event data
     * 
     *  @return PLDM completion code
     *
     */
    
    int handlePlatformEvent(uint8_t payloadId, uint8_t tid, uint8_t eventClass, 
                            const uint8_t* eventData, size_t eventDataSize);

    /** @brief Handler for CPEREvent
     *
     *  @param[in] payloadId - Payload ID used to know where the command from
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request payload length
     *  @param[in] formatVersion - Version of the event format
     *  @param[in] tid - Terminus ID of the event's originator
     *  @param[in] eventDataOffset - Offset of the event data in the request
     *                               message
     *  @return PLDM completion code
     */
    int handleCperEvent(uint8_t payloadId, const pldm_msg* request, 
                        size_t payloadLength, uint8_t formatVersion, 
                        uint8_t tid, size_t eventDataOffset);

    /** @brief Handler for pldmMessagePollEvent
     *
     *  @param[in] payloadId - Payload ID used to know where the command from
     *  @param[in] request - Request message
     *  @param[in] payloadLength - Request payload length
     *  @param[in] formatVersion - Version of the event format
     *  @param[in] tid - Terminus ID of the event's originator
     *  @param[in] eventDataOffset - Offset of the event data in the request
     *                               message
     *  @return PLDM completion code
     */
    int handlePldmMessagePollEvent(uint8_t payloadId, const pldm_msg* request,
                                   size_t payloadLength, uint8_t formatVersion, 
                                   uint8_t tid, size_t eventDataOffset);
  protected:
    /** @brief A task to poll all events from terminus
     *
     *  @param[in] payloadId - ID used to know where the command from
     *  @param[in] tid - the destination TID
     * 
     *  @return PLDM completion code
     */
    int pollForPlatformEventTask(uint8_t payloadId, uint8_t tid);

    /** @brief Send pollForPlatformEventMessage and return response
     *
     *  @param[in] payloadId
     *  @param[in] tid
     *  @param[in] transferOperationFlag
     *  @param[in] dataTransferHandle
     *  @param[in] eventIdToAcknowledge
     *  @param[out] completionCode
     *  @param[out] eventTid
     *  @param[out] eventId
     *  @param[out] nextDataTransferHandle
     *  @param[out] transferFlag
     *  @param[out] eventClass
     *  @param[out] eventDataSize
     *  @param[out] eventData
     *  @param[out] eventDataIntegrityChecksum
     * 
     *  @return PLDM completion code
     *
     */
    int pollForPlatformEventMessage(
      uint8_t payloadId, uint8_t tid, uint8_t transferOperationFlag, 
      uint32_t dataTransferHandle, uint16_t eventIdToAcknowledge, 
      uint8_t& completionCode, uint8_t& eventTid, uint16_t& eventId, 
      uint32_t& nextDataTransferHandle, uint8_t& transferFlag, 
      uint8_t& eventClass, uint32_t& eventDataSize, 
      std::vector<uint8_t>& eventData, uint32_t& eventDataIntegrityChecksum);

    /** @brief The maximum size of the event receiver buffer that 
     * can hold a single chunk of event message. 
     */
    uint16_t maxBufferSize = 256;
};

} // namespace platform_mc
} // namespace pldm