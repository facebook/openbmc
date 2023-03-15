#pragma once

#include <vector>
#include <cstdint>

int oem_pldm_send_recv (uint8_t bus, int eid,
                        const std::vector<uint8_t>& request,
                              std::vector<uint8_t>& response);