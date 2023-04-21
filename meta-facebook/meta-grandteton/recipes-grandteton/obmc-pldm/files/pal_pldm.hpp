#pragma once
#include <cstdint>

int pal_pldm_get_firmware_parameter(uint8_t bus, uint8_t eid);
int is_pldm_supported(uint8_t bus, uint8_t eid);