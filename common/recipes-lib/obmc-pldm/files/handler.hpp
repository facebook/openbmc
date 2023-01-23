#pragma once

#include "libpldm/base.h"
#include <cstdio>
#include <cassert>
#include <functional>
#include <map>
#include <vector>

namespace pldm
{

using Command = uint8_t;

class pldm_response
{
  private:
    std::vector<uint8_t> response;
  public:
    explicit pldm_response(const size_t& resp_size):
      response(std::vector<uint8_t>(sizeof(pldm_msg_hdr) + resp_size, 0)) {}
    std::vector<uint8_t>& get() { return response; }
    pldm_msg* msg() { return reinterpret_cast<pldm_msg*>(response.data()); }
};

namespace responder
{

using Response = std::vector<uint8_t>;
class CmdHandler;
using HandlerFunc =
    std::function<Response(const pldm_msg* request, size_t reqMsgLen)>;

class CmdHandler
{
  public:
    /** @brief Invoke a PLDM command handler
     *
     *  @param[in] pldmCommand - PLDM command code
     *  @param[in] request - PLDM request message
     *  @param[in] reqMsgLen - PLDM request message size
     *  @return PLDM response message
     */
    Response handle(Command pldmCommand, const pldm_msg* request,
                    size_t reqMsgLen)
    {
      Response res = {};
      if (handlers.find(pldmCommand) == handlers.end()) {
        fprintf(stderr, "Command(%02X) not supported.\n", pldmCommand);
      } else {
        res = handlers.at(pldmCommand)(request, reqMsgLen);
      }
      return res;
    }

    /** @brief Create a response message containing only cc
     *
     *  @param[in] request - PLDM request message
     *  @param[in] cc - Completion Code
     *  @return PLDM response message
     */
    static Response ccOnlyResponse(const pldm_msg* request, uint8_t cc)
    {
        Response response(sizeof(pldm_msg), 0);
        auto ptr = reinterpret_cast<pldm_msg*>(response.data());
        auto rc =
            encode_cc_only_resp(request->hdr.instance_id, request->hdr.type,
                                request->hdr.command, cc, ptr);
        assert(rc == PLDM_SUCCESS);
        return response;
    }

  protected:
    /** @brief map of PLDM command code to handler - to be populated by derived
     *         classes.
     */
    std::map<Command, HandlerFunc> handlers;
};

} // namespace responder
} // namespace pldm
