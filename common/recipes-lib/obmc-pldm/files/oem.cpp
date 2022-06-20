#include <libpldm/base.h>
#include <libpldm/bios.h>
#include <libpldm/fru.h>
#include <libpldm/platform.h>
#include <openbmc/ipmi.h>

#include "oem.hpp"
#include "pldm.h"

#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace pldm
{

namespace responder
{

namespace oem
{

bool is_IANA_valid (uint8_t* data, size_t payloadLength)
{
  if (payloadLength < 3) {
    fprintf(stderr, "Wrong playload length for PLDM OEM Type (at lease 3 btyes).\n");
    return false;
  }

  uint8_t iana[3] = {0x15, 0xA0, 0x00};
  if (memcmp(iana, data, 3)) {
    fprintf(stderr, "Wrong PLDM OEM Type 0x%02X 0x%02X 0x%02X(0x15, 0xA0, 0x00)."
            , *data, *(data+1), *(data+2));
    return false;
  }

  return true;
} 

int encode_echo_resp (uint8_t instance_id, uint8_t completion_code, 
                        struct pldm_msg *msg, size_t payloadLength)
  {
  if (msg == NULL) {
    return PLDM_ERROR_INVALID_DATA;
  }

  struct pldm_header_info header;
  header.instance = instance_id;
  header.msg_type = PLDM_RESPONSE;
  header.command  = CMD::ECHO;

  uint8_t rc = pack_pldm_header(&header, &(msg->hdr));
  if (rc != PLDM_SUCCESS) {
    return rc;
  }

  auto response = (struct pldm_msg_resp *) msg;
  response->completion_code = completion_code;
  memcpy(response->payload, msg->payload, payloadLength);

  return PLDM_SUCCESS;
}

Response Handler::echo (const pldm_msg* request, size_t payloadLength)
{
  if (!is_IANA_valid((uint8_t*)request, payloadLength)) {
    return CmdHandler::ccOnlyResponse(request, PLDM_ERROR_INVALID_DATA);
  }

  Response response(PLDM_RESP_HEADER_SIZE + payloadLength, 0);
  auto responsePtr = reinterpret_cast<pldm_msg*>(response.data());
  auto rc = encode_echo_resp(request->hdr.instance_id, PLDM_SUCCESS, 
                            responsePtr, payloadLength);

  if (rc != PLDM_SUCCESS) {
    return CmdHandler::ccOnlyResponse(request, rc);
  }

  return response;
}

int encode_ipmi_resp (uint8_t instance_id, uint8_t completion_code, 
                        Response &msg, size_t payloadLength)
{
  if (msg.empty()) {
    return PLDM_ERROR_INVALID_DATA;
  }

  // header
  struct pldm_header_info header;
  header.instance = instance_id;
  header.msg_type = PLDM_RESPONSE;
  header.command  = CMD::IPMI;
  auto resHeader = reinterpret_cast<pldm_msg*>(msg.data());
  uint8_t rc = pack_pldm_header(&header, &(resHeader->hdr));
  if (rc != PLDM_SUCCESS) {
    return rc;
  }

  // completion code
  auto response = reinterpret_cast<pldm_msg_resp*>(msg.data());
  response->completion_code = completion_code;

  // ipmi payload
  int rlen;
  uint8_t tbuf[255], rbuf[255];
  lib_ipmi_handle(tbuf, (uint8_t)payloadLength, rbuf, (unsigned short*)&rlen);
  for (int i = 0; i < rlen; ++i){
    printf("IPMI data[%2d] = 0x%02X\n", i, *(rbuf+i));
    msg.push_back(*(rbuf+i));
  }

  return PLDM_SUCCESS;
}

Response Handler::ipmi (const pldm_msg* request, size_t payloadLength)
{
  if (!is_IANA_valid((uint8_t*)request, payloadLength)) {
    return CmdHandler::ccOnlyResponse(request, PLDM_ERROR_INVALID_DATA);
  }

  Response response(PLDM_RESP_HEADER_SIZE);
  auto rc = encode_ipmi_resp(request->hdr.instance_id, PLDM_SUCCESS, 
                            response, payloadLength);

  if (rc != PLDM_SUCCESS) {
    return CmdHandler::ccOnlyResponse(request, rc);
  }

  return response;
}

} // namespace base
} // namespace responder
} // namespace pldm
