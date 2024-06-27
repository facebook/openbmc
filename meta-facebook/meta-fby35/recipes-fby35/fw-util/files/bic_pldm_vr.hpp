#ifndef _BIC_PLDM_VR_H_
#define _BIC_PLDM_VR_H_

#include "pldm_comp.hpp"
#include "signed_decoder.hpp"
#include "server.h"

#include <string>
#include <map>

constexpr auto VENDOR = "VENDOR";
constexpr auto RMNG_W = "REMAINING WRITES";

class PldmVrComponent : public PldmComponent {
  protected:
    uint8_t slot_id = 0;
    Server server;
    static bool is_printed;
    int update_internal(std::string image, bool force) override;
    int update_version_cache() override;
    std::map<uint8_t, std::string> get_vr_list();
  public:
    PldmVrComponent(const std::string &fru, const std::string &comp,
                    uint8_t comp_id, uint8_t bus, uint8_t eid,
                    const signed_header_t& info,
                    const pldm_image_signed_info_map& info_map,
                    bool has_standard_descriptor = false): 
      PldmComponent(fru, comp, comp_id, bus, eid, info, 
                    info_map, has_standard_descriptor), 
      slot_id(sys().get_fru_id(PldmComponent::fru)), server(slot_id, fru) {}
    int get_version(json& j) override;
    int print_version() override;
};

#endif // _BIC_PLDM_VR_H_