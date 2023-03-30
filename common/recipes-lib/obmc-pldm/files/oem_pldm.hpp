#pragma once

#include <vector>
#include <cstdint>

int oem_pldm_send (int fd, int eid, const std::vector<uint8_t>& request);

int oem_pldm_recv (int fd, int eid, std::vector<uint8_t>& response);

int oem_pldm_send_recv_w_fd (int fd, int eid,
                        const std::vector<uint8_t>& request,
                              std::vector<uint8_t>& response);

int oem_pldm_send_recv (uint8_t bus, int eid,
                        const std::vector<uint8_t>& request,
                              std::vector<uint8_t>& response);