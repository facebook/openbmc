#pragma once

#include "libpldm/base.h"
#include "handler.hpp"

#include <stdint.h>
#include <vector>

#define IPMI_PKT_MAX_SIZE 300

using namespace pldm::responder;

namespace pldm
{
namespace responder
{
namespace oem
{

enum CMD {
    ECHO = 0x00,
    IPMI = 0x01,
};

enum PLDM_FORMAT_POS {
    POS_PLDM_INSTANCE_ID = 0,
    POS_PLDM_TYPE,
    POS_PLDM_COMMAND_CODE,
    POS_IPMI_IANA,
    POS_IPMI_NETFN = 6,
    POS_IPMI_CMD,
    POS_IPMI_DATA
};

struct pldm_msg_resp {
    struct pldm_msg_hdr hdr; //!< PLDM message header
    uint8_t completion_code;
    uint8_t payload[1]; //!< &payload[0] is the beginning of the payload
} __attribute__((packed));

class Handler : public CmdHandler
{
  public:
    Handler()
    {
        handlers.emplace(ECHO,
                        [this](const pldm_msg* request, size_t payloadLength) {
                            return this->echo(request, payloadLength);
                        });
        handlers.emplace(IPMI,
                        [this](const pldm_msg* request, size_t payloadLength) {
                            return this->ipmi(request, payloadLength);
                        });
    }

  protected:
    /** @brief Handler for oem(ipmi) commands
     *
     *  @param[in] request - Request message payload
     *  @param[in] payload_length - Request message payload length
     *  @param[return] Response - PLDM Response message
     */
    Response echo (const pldm_msg* request, size_t payloadLength);
    Response ipmi (const pldm_msg* request, size_t payloadLength);
};

} // namespace base
} // namespace responder
} // namespace pldm
