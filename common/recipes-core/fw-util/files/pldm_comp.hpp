#pragma once
#include <libpldm/firmware_update.h>
#include "signed_decoder.hpp"

#define PLDM_VER_MAX_LENGTH 256
#define PLDM_COMPONENT_IDENTIFIER_DEFAULT 0
#define PLDM_APPLY_DELAY_TIME_DAFAULT 0

class PldmComponent : public InfoChecker
{
  protected:
    // Basic info to be record
    std::string fru;
    std::string component;
    uint8_t bus, eid, component_identifier;
    int wait_apply_time;
    int find_image_index(uint8_t /*target_id*/) const;
    int get_raw_image(const std::string& /*image*/, std::string& /*raw_image*/);
    int del_raw_image() const;
    const signed_header_t unuse_info;

  public:
    PldmComponent(const signed_header_t& info, const std::string &fru, const std::string &comp,
                  uint8_t bus, uint8_t eid): InfoChecker(info),
                  fru(fru), component(comp), bus(bus), eid(eid), component_identifier(PLDM_COMPONENT_IDENTIFIER_DEFAULT),
                  wait_apply_time(PLDM_APPLY_DELAY_TIME_DAFAULT) {}
    PldmComponent(const std::string &fru, const std::string &comp, uint8_t bus, uint8_t eid, 
		  uint8_t component_identifier, int wait_apply_time): InfoChecker(unuse_info),
                  fru(fru), component(comp), bus(bus), eid(eid), component_identifier(component_identifier),
                  wait_apply_time(wait_apply_time) {}
    virtual ~PldmComponent() = default;
    int pldm_update(const std::string& /*image*/, bool is_standard_descriptor, uint8_t specified_comp = 0xFF);
    virtual int pldm_version(json& /*json*/) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int comp_update(const std::string& /*image*/) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int comp_fupdate(const std::string& /*image*/) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int comp_version(json& /*json*/) { return FW_STATUS_NOT_SUPPORTED; }
};
