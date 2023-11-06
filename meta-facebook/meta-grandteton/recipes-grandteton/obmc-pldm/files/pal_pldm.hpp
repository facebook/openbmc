#pragma once
#include <cstdint>
#include <string>

int update_pldm_ver_cache(const std::string& fru, uint8_t bus, uint8_t eid);
int get_pldm_active_ver(uint8_t bus, uint8_t eid, std::string& active_ver);
int is_pldm_supported(const std::string& fru, uint8_t bus, uint8_t eid);
