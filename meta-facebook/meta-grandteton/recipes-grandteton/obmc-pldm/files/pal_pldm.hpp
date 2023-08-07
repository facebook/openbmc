#pragma once
#include <cstdint>
#include <string>

int pal_update_swb_ver_cache(uint8_t bus, uint8_t eid);
int pal_pldm_get_active_ver(uint8_t bus, uint8_t eid, std::string& active_ver);
int is_pldm_supported(uint8_t bus, uint8_t eid);