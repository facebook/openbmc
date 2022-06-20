#pragma once

#include "libpldm/base.h"
#include "handler.hpp"

#include <stdint.h>
#include <vector>

using namespace pldm::responder;

namespace pldm
{
namespace responder
{
namespace base
{

class Handler : public CmdHandler
{
  public:
    Handler()
    {
        handlers.emplace(PLDM_GET_PLDM_TYPES,
                        [this](const pldm_msg* request, size_t payloadLength) {
                            return this->getPLDMTypes(request, payloadLength);
                        });

        handlers.emplace(PLDM_GET_PLDM_COMMANDS,
                        [this](const pldm_msg* request, size_t payloadLength) {
                            return this->getPLDMCommands(request, payloadLength);
                        });

        handlers.emplace(PLDM_GET_PLDM_VERSION,
                        [this](const pldm_msg* request, size_t payloadLength) {
                            return this->getPLDMVersion(request, payloadLength);
                        });

        handlers.emplace(PLDM_GET_TID,
                        [this](const pldm_msg* request, size_t payloadLength) {
                            return this->getTID(request, payloadLength);
                        });
    }

    /** @brief Handler for getPLDMTypes
     *
     *  @param[in] request - Request message payload
     *  @param[in] payload_length - Request message payload length
     *  @param[return] Response - PLDM Response message
     */
    Response getPLDMTypes(const pldm_msg* request, size_t payloadLength);

    /** @brief Handler for getPLDMCommands
     *
     *  @param[in] request - Request message payload
     *  @param[in] payload_length - Request message payload length
     *  @param[return] Response - PLDM Response message
     */
    Response getPLDMCommands(const pldm_msg* request, size_t payloadLength);

    /** @brief Handler for getPLDMCommands
     *
     *  @param[in] request - Request message payload
     *  @param[in] payload_length - Request message payload length
     *  @param[return] Response - PLDM Response message
     */
    Response getPLDMVersion(const pldm_msg* request, size_t payloadLength);

    /** @brief Handler for getTID
     *
     *  @param[in] request - Request message payload
     *  @param[in] payload_length - Request message payload length
     *  @param[return] Response - PLDM Response message
     */
    Response getTID(const pldm_msg* request, size_t payloadLength);
};

} // namespace base
} // namespace responder
} // namespace pldm
