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
  if (payloadLength < POS_IPMI_IANA + 2) {
    fprintf(stderr, "Wrong playload length for PLDM OEM Type (at lease 3 btyes).\n");
    return false;
  }

  uint8_t iana[3] = {0x15, 0xA0, 0x00};
  if (memcmp(iana, data + POS_IPMI_IANA, 3)) {
    fprintf(stderr, "Wrong PLDM OEM Type 0x%02X 0x%02X 0x%02X(0x15, 0xA0, 0x00)."
            , *data, *(data + 1), *(data + 2));
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

int encode_ipmi_resp (uint8_t *request, uint8_t instance_id, uint8_t completion_code,
                      Response & msg, size_t payloadLength)
{
  if (msg.empty()) {
    return PLDM_ERROR_INVALID_DATA;
  }

  // header
  struct pldm_header_info header;
  header.instance = instance_id;
  header.msg_type = PLDM_RESPONSE;
  header.pldm_type = PLDM_OEM;
  header.command  = CMD::IPMI;
  auto resHeader = reinterpret_cast<pldm_msg*>(msg.data());
  uint8_t rc = pack_pldm_header(&header, &(resHeader->hdr));
  if (rc != PLDM_SUCCESS) {
    return rc;
  }

  // completion code
  auto response = reinterpret_cast<pldm_msg_resp*>(msg.data());
  response->completion_code = completion_code;

  //Structure the data
  uint8_t tbuf[IPMI_PKT_MAX_SIZE] = {0};
  ipmi_mn_req_t *p_ipmi_req = (ipmi_mn_req_t*)tbuf;
  int data_size = int(payloadLength) - 5;

  p_ipmi_req->payload_id = 0;
  p_ipmi_req->netfn_lun = *((uint8_t *)request + POS_IPMI_NETFN);
  p_ipmi_req->cmd = *((uint8_t *)request + POS_IPMI_CMD);

  if (data_size >= 0 && data_size < IPMI_PKT_MAX_SIZE) {
    if(data_size != 0) {
      uint8_t *ipmi_data = (uint8_t *)request + POS_IPMI_DATA;
      memcpy(p_ipmi_req->data, ipmi_data, data_size);
    }
  }
  else {
    return PLDM_ERROR_INVALID_DATA;
  }

  std::cout << std::hex;
  std::cout << "netfn = 0x" << (unsigned)p_ipmi_req->netfn_lun << "\n";
  std::cout << "cmd = 0x" << (unsigned)p_ipmi_req->cmd << "\n";

  for (int i = 0; i < data_size; i++) {
    std::cout << "request[" << i << "] = 0x" << unsigned(p_ipmi_req->data[i]) << "\n";
  }
  std::cout << std::dec;

  uint8_t rbuf[IPMI_PKT_MAX_SIZE] = {0};
  uint16_t rlen = 0;

  lib_ipmi_handle(tbuf, payloadLength + 1, rbuf, &rlen);

  //Add Response data
  for (int i = 0; i < rlen; ++i) {
    msg.push_back(*(rbuf + i));
  }

  //Add MCTP destination eid
  msg.insert(msg.begin(), 0x0);

  //Add MCTP message type
  msg.insert(msg.begin(), MCTP_PLDM_TYPE);

  return PLDM_SUCCESS;
}

Response Handler::ipmi (const pldm_msg* request, size_t payloadLength)
{
  if (!is_IANA_valid((uint8_t*)request, payloadLength)) {
    return CmdHandler::ccOnlyResponse(request, PLDM_ERROR_INVALID_DATA);
  }

  Response response(PLDM_RESP_HEADER_SIZE);
  auto rc = encode_ipmi_resp((uint8_t*)request, request->hdr.instance_id, PLDM_SUCCESS,
                             response, payloadLength);

  if (rc != PLDM_SUCCESS) {
    return CmdHandler::ccOnlyResponse(request, rc);
  }

  return response;
}

} // namespace base
} // namespace responder
} // namespace pldm
