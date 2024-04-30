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

#include "platform.hpp"
#include "event_manager.hpp"

#include <memory>
#include <optional>

namespace pldm
{
namespace responder
{
namespace platform
{

std::optional<EventMap> get_add_on_platform_event_handlers()
{
  auto eventManager = std::make_shared<pldm::platform_mc::EventManager>();

  EventMap addOnHandlersMap{
    {PLDM_OEM_EVENT_CLASS_0xFA,
      {[&eventManager](uint8_t payloadId, const pldm_msg* request, 
                       size_t payloadLength, uint8_t formatVersion, 
                       uint8_t tid, size_t eventDataOffset) {
          return eventManager->handleCperEvent(
              payloadId, request, payloadLength, 
              formatVersion, tid, eventDataOffset);
    }}},
    {PLDM_MESSAGE_POLL_EVENT,
      {[&eventManager](uint8_t payloadId, const pldm_msg* request, 
                       size_t payloadLength, uint8_t formatVersion, 
                       uint8_t tid, size_t eventDataOffset) {
          return eventManager->handlePldmMessagePollEvent(
              payloadId, request, payloadLength, 
              formatVersion, tid, eventDataOffset);
    }}}
  };

  return addOnHandlersMap;
}

} // namespace platform
} // namespace responder
} // namespace pldm