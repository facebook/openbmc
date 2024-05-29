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

#include "event_manager.hpp"
#include "oem_pldm.hpp"

#include <libpldm/utils.h>
#include <libpldm/platform.h>
#include <openbmc/cper.hpp>

#include <syslog.h>
#include <unistd.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>

namespace pldm
{
namespace platform_mc
{

namespace fs = std::filesystem;

int EventManager::eventMessageBufferSize(uint8_t payloadId, 
																				 uint16_t& terminusBufferSize)
{
	Request request(sizeof(pldm_msg_hdr) +
									PLDM_EVENT_MESSAGE_BUFFER_SIZE_REQ_BYTES);
	auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());
	auto rc = encode_event_message_buffer_size_req(0, maxBufferSize, requestMsg);
	if (rc)
	{
		return rc;
	}

	Response response{};
	rc = oem_pldm_send_recv(payloadId, DEFAULT_SATMC_EID, request, response);
	if (rc)
	{
		return rc;
	}

	uint8_t completionCode;
	auto responseMsg = reinterpret_cast<pldm_msg*>(response.data());
	auto payloadLength = response.size() - sizeof(responseMsg->hdr);
	rc = decode_event_message_buffer_size_resp(
			responseMsg, payloadLength, &completionCode, &terminusBufferSize);
	if (rc)
	{
		return rc;
	}
	return completionCode;
}

int EventManager::handlePlatformEvent(uint8_t payloadId, uint8_t tid, 
																			uint8_t eventClass,
                                      const uint8_t* eventData,
                                      size_t eventDataSize)
{
	if (eventClass == PLDM_MESSAGE_POLL_EVENT)
	{
		syslog(LOG_INFO, "Received poll event. payloadId=%u tid=%u", payloadId, tid);
		std::thread pollEventTask(
			[this, payloadId, tid](){pollForPlatformEventTask(payloadId, tid);});
		pollEventTask.detach();
	}
	else if (eventClass == PLDM_OEM_EVENT_CLASS_0xFA)
	{
		auto rc = cper::createCperDumpEntry(payloadId, eventData, eventDataSize);
		if (rc != cper::CPER_HANDLE_SUCCESS)
		{
			return PLDM_ERROR;
		}
	}
	else
	{
		syslog(LOG_INFO, "Unhandled event, event class=%u", eventClass);
	}
	return PLDM_SUCCESS;
}

int EventManager::handleCperEvent(uint8_t payloadId, const pldm_msg* request, 
																	size_t payloadLength, 
                                  uint8_t /* formatVersion */, uint8_t tid, 
                                  size_t eventDataOffset)
{
	auto eventData = reinterpret_cast<const uint8_t*>(request->payload) +
										eventDataOffset;
	auto eventDataSize = payloadLength - eventDataOffset;
	handlePlatformEvent(payloadId, tid, PLDM_OEM_EVENT_CLASS_0xFA, 
											eventData, eventDataSize);
	return PLDM_SUCCESS;
}

int EventManager::handlePldmMessagePollEvent(uint8_t payloadId,
																						 const pldm_msg* request, 
                                             size_t payloadLength, 
                                             uint8_t /* formatVersion */, 
                                             uint8_t tid, size_t eventDataOffset)
{
	auto eventData = reinterpret_cast<const uint8_t*>(request->payload) +
										eventDataOffset;
	auto eventDataSize = payloadLength - eventDataOffset;
	uint8_t eventDataFormatVersion;
	uint16_t eventId;
	uint32_t dataTransferHandle;
	auto rc = decode_pldm_message_poll_event_data(
			eventData, eventDataSize, &eventDataFormatVersion, &eventId,
			&dataTransferHandle);
	if (rc != PLDM_SUCCESS)
	{
			return PLDM_ERROR;
	}

	if (eventDataFormatVersion != 0x01)
	{
			return PLDM_ERROR_INVALID_DATA;
	}

	handlePlatformEvent(payloadId, tid, PLDM_MESSAGE_POLL_EVENT, 
											eventData, eventDataSize);
	return PLDM_SUCCESS;
}

int EventManager::pollForPlatformEventTask(uint8_t payloadId, uint8_t tid)
{
	uint8_t rc = 0;
	uint8_t transferOperationFlag = PLDM_GET_FIRSTPART;
	uint32_t dataTransferHandle = 0;
	uint32_t eventIdToAcknowledge = 0;

	uint8_t completionCode = 0;
	uint8_t eventTid = 0;
	uint16_t eventId = 0xffff;
	uint32_t nextDataTransferHandle = 0;
	uint8_t transferFlag = 0;
	uint8_t eventClass = 0;
	std::vector<uint8_t> eventMessage{};
	uint32_t eventDataSize = maxBufferSize;
	std::vector<uint8_t> eventData(eventDataSize);
	uint32_t eventDataIntegrityChecksum = 0;

	/*
	TODO: Is it necessary to introduce a delay to wait for a "PLDM_SUCCESS" 
	response to be sent to satMC before retrieving the data chunk from satMC 
	using the "pollForPlatformEventMessage" command?
	*/
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while (eventId != 0)
	{
		rc = pollForPlatformEventMessage(
					payloadId, tid, transferOperationFlag, dataTransferHandle,
					eventIdToAcknowledge, completionCode, eventTid, eventId, 
					nextDataTransferHandle, transferFlag, eventClass, eventDataSize,
					eventData, eventDataIntegrityChecksum);
		if (rc || completionCode != PLDM_SUCCESS)
		{
			syslog(LOG_ERR, "pollForPlatformEventMessage failed. " 
						"payloadId=%u tid=%u transferOpFlag=%u rc=%u", 
						payloadId, tid, transferOperationFlag, rc);
			return rc ? rc : completionCode;
		}

		if (eventDataSize > 0)
		{
			eventMessage.insert(eventMessage.end(), eventData.begin(),
													eventData.begin() + eventDataSize);
		}

		if (transferOperationFlag == PLDM_ACKNOWLEDGEMENT_ONLY)
		{
			if (eventId == 0xffff)
			{
					transferOperationFlag = PLDM_GET_FIRSTPART;
					dataTransferHandle = 0;
					eventIdToAcknowledge = 0;
					eventMessage.clear();
			}
		}
		else
		{
			if (transferFlag == PLDM_START || transferFlag == PLDM_MIDDLE)
			{
				transferOperationFlag = PLDM_GET_NEXTPART;
				dataTransferHandle = nextDataTransferHandle;
				eventIdToAcknowledge = 0xffff;
			}
			else
			{
				if (transferFlag == PLDM_START_AND_END)
				{
					handlePlatformEvent(payloadId, eventTid, eventClass, 
															eventMessage.data(), eventMessage.size());
				}
				else if (transferFlag == PLDM_END)
				{
					if (eventDataIntegrityChecksum ==
							crc32(eventMessage.data(), eventMessage.size()))
					{
						handlePlatformEvent(payloadId, eventTid, eventClass, 
																eventMessage.data(), eventMessage.size());
					}
					else
					{
						syslog(LOG_ERR, "pollForPlatformEventMessage checksum error, " 
									"payloadId=%u tid=%u eventId=%u eventClass=%u", 
									payloadId, tid, eventId, eventClass);
					}
				}

				transferOperationFlag = PLDM_ACKNOWLEDGEMENT_ONLY;
				dataTransferHandle = 0;
				eventIdToAcknowledge = eventId;
			}
		}
	}

	return PLDM_SUCCESS;
}

int EventManager::pollForPlatformEventMessage(
	uint8_t payloadId, uint8_t tid, uint8_t transferOperationFlag, 
	uint32_t dataTransferHandle, uint16_t eventIdToAcknowledge, 
	uint8_t& completionCode, uint8_t& eventTid, uint16_t& eventId, 
	uint32_t& nextDataTransferHandle, uint8_t& transferFlag, uint8_t& eventClass, 
	uint32_t& eventDataSize, std::vector<uint8_t>& eventData, 
	uint32_t& eventDataIntegrityChecksum)
{
	Request request(sizeof(pldm_msg_hdr) +
									PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
	auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());

	auto rc = encode_poll_for_platform_event_message_req(
							0, 0x01, transferOperationFlag, dataTransferHandle,
							eventIdToAcknowledge, requestMsg, 
							PLDM_POLL_FOR_PLATFORM_EVENT_MESSAGE_REQ_BYTES);
	if (rc)
	{
		syslog(LOG_ERR, "encode_poll_for_platform_event_message_req failed. " 
					 "payloadId=%u tid=%u rc=%d", payloadId, tid, rc);
		return rc;
	}

	Response response{};
	rc = oem_pldm_send_recv(payloadId, DEFAULT_SATMC_EID, request, response);
	if (rc)
	{
		return rc;
	}

	uint8_t* eventDataPtr = nullptr;
	auto responseMsg = reinterpret_cast<pldm_msg*>(response.data());
	auto payloadLength = response.size() - sizeof(responseMsg->hdr);
	rc = decode_poll_for_platform_event_message_resp(
				responseMsg, payloadLength, &completionCode, &eventTid, &eventId,
				&nextDataTransferHandle, &transferFlag, &eventClass, &eventDataSize,
				reinterpret_cast<void**>(&eventDataPtr), &eventDataIntegrityChecksum);
	if (rc)
	{
		syslog(LOG_ERR, "decode_poll_for_platform_event_message_resp failed. " 
					 "payloadId=%u tid=%u rc=%d responseLen=%zu", 
					 payloadId, tid, rc, response.size());
		return rc;
	}

	if (completionCode == PLDM_SUCCESS && eventDataPtr && eventDataSize > 0)
	{
		eventData.resize(eventDataSize);
		std::memcpy(eventData.data(), eventDataPtr, eventDataSize);
	}
	return completionCode;
}

} // namespace platform_mc
} // namespace pldm