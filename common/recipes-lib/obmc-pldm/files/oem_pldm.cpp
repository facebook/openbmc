#include "oem_pldm.hpp"
#include "pldm.h"
#include <cstdlib>

int oem_pldm_send_recv (uint8_t bus, int eid,
                        const std::vector<uint8_t>& request,
                              std::vector<uint8_t>& response)
{
  int ret = 0;
  uint8_t* resp = nullptr;
  size_t resp_len;

  ret = oem_pldm_send_recv(bus, eid, request.data(), request.size(), &resp, &resp_len);
  if (ret == 0 && resp != nullptr) {
    response = std::vector<uint8_t>(resp, resp+resp_len);
    free(resp);
  }

  return ret;
}