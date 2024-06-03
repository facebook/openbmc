#pragma once
#include <libpldm/firmware_update.h>
#include "signed_decoder.hpp"
#include <fmt/format.h>

#define PLDM_VER_MAX_LENGTH 256
#define PLDM_COMPONENT_IDENTIFIER_DEFAULT 0
#define PLDM_APPLY_DELAY_TIME_DAFAULT 0

constexpr auto ACTIVE_VERSION = "Active Version";
constexpr auto PENDING_VERSION = "Pending Version";
constexpr auto INVALID_VERSION = "NA";

class PldmComponent : public Component, public InfoChecker
{
  protected:
    // Basic info to be record
    std::string fru;
    std::string component;
    uint8_t bus, eid, component_identifier;
    bool has_standard_descriptor;
    int wait_apply_time;
    std::string activeVersionKey, pendingVersionKey;
    int find_image_index(uint8_t /*target_id*/) const;
    int get_raw_image(const std::string& /*image*/, std::string& /*raw_image*/);
    int del_raw_image() const;
    virtual int check_image(std::string image);
    virtual int update_internal(std::string image, bool force);
    virtual int update_version_cache();
    const signed_header_t unuse_info;
    const pldm_image_signed_info_map signed_info_map;

  public:
    PldmComponent(const std::string &fru, const std::string &comp, 
                  uint8_t component_identifier, uint8_t bus, uint8_t eid, 
                  const signed_header_t& info, const pldm_image_signed_info_map& info_map,
                  bool has_standard_descriptor = false): Component(fru, comp), InfoChecker(info),
                  fru(fru), component(comp), bus(bus), eid(eid), 
                  component_identifier(component_identifier), 
                  has_standard_descriptor(has_standard_descriptor), 
                  wait_apply_time(PLDM_APPLY_DELAY_TIME_DAFAULT), 
                  activeVersionKey(fmt::format("{}_{}_active_ver", fru, comp)), 
                  pendingVersionKey(fmt::format("{}_{}_pending_ver", fru, comp)), 
                  signed_info_map(info_map) {}
    PldmComponent(const signed_header_t& info, const std::string &fru, const std::string &comp,
                  uint8_t bus, uint8_t eid): Component(fru, comp), InfoChecker(info),
                  fru(fru), component(comp), bus(bus), eid(eid), component_identifier(PLDM_COMPONENT_IDENTIFIER_DEFAULT),
                  wait_apply_time(PLDM_APPLY_DELAY_TIME_DAFAULT) {}
    PldmComponent(const std::string &fru, const std::string &comp, uint8_t bus, uint8_t eid, 
		  uint8_t component_identifier, int wait_apply_time): Component(fru, comp), InfoChecker(unuse_info),
                  fru(fru), component(comp), bus(bus), eid(eid), component_identifier(component_identifier),
                  wait_apply_time(wait_apply_time) {}
    virtual ~PldmComponent() = default;
    int update(std::string image) override;
    int fupdate(std::string image) override;
    int get_version(json& j) override;
    int print_version() override;
    int pldm_update(const std::string& /*image*/, bool is_standard_descriptor, uint8_t specified_comp = 0xFF);
};
