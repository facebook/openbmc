#pragma once
#include <libpldm/firmware_update.h>
#include "signed_decoder.hpp"

#define PLDM_VER_MAX_LENGTH 256

class PldmComponent : public InfoChecker
{
  protected:
    // Basic info to be record
    std::string fru;
    std::string component;
    uint8_t bus, eid;
    int find_image_index(uint8_t /*target_id*/) const;
    int get_raw_image(const std::string& /*image*/, std::string& /*raw_image*/);
    int del_raw_image() const;

  public:
    PldmComponent(const signed_header_t& info, const std::string &fru, const std::string &comp,
                  uint8_t bus, uint8_t eid): InfoChecker(info),
                  fru(fru), component(comp), bus(bus), eid(eid) {}
    virtual ~PldmComponent() = default;
    int pldm_update(const std::string& /*image*/, uint8_t specified_comp = 0xFF);
    virtual int pldm_version(json& /*json*/) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int comp_update(const std::string& /*image*/) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int comp_fupdate(const std::string& /*image*/) { return FW_STATUS_NOT_SUPPORTED; }
    virtual int comp_version(json& /*json*/) { return FW_STATUS_NOT_SUPPORTED; }
};