#ifndef _BIC_PLDM_VR_H_
#define _BIC_PLDM_VR_H_

#include "pldm_comp.hpp"
#include "signed_decoder.hpp"
#include <string>

class PldmVrComponent : public PldmComponent {
  protected:
    int update_internal(std::string image, bool force) override;
  public:
    PldmVrComponent(const std::string &fru, const std::string &comp,
                    uint8_t comp_id, uint8_t bus, uint8_t eid,
                    const signed_header_t& info,
                    const pldm_image_signed_info_map& info_map,
                    bool has_standard_descriptor = false): 
      PldmComponent(fru, comp, comp_id, bus, eid, info, 
                    info_map, has_standard_descriptor) {}
    int get_version(json& j) override;
};

#endif // _BIC_PLDM_VR_H_