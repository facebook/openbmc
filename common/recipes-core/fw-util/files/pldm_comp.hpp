#pragma once
#include <libpldm/firmware_update.h>
#include "signed_decoder.hpp"

class PldmComponent : public InfoChecker
{
  private:
    std::string component;
    uint8_t bus, eid;
    size_t offset = 0;
    pldm_package_header_information pkgHeader{};
    int read_pkg_header_info(int /*fd*/);
    int read_device_id_record(int /*fd*/);
    int read_comp_image_info(int /*fd*/);
    int pldm_update(const std::string& /*image*/);

  protected:
    // Basic info to be record
    std::vector<uint8_t> img_uuid{};
    signed_header_t img_info{};
    int check_UUID() const;
    virtual void store_fw_package_hdr(pldm_package_header_information& /*hdr*/) {}
    virtual void store_device_id_record(pldm_firmware_device_id_record& /*id_record*/,
                                        uint16_t& /*descriper_type*/,
                                        variable_field& /*descriper_data*/) {};
    virtual void store_comp_img_info(pldm_component_image_information& /*comp_info*/,
                                        variable_field& /*comp_verstr*/) {};

  public:
    PldmComponent(const signed_header_t& info, const std::string &comp,
                  uint8_t bus, uint8_t eid): InfoChecker(info), component(comp),
                  bus(bus), eid(eid) {}
    virtual ~PldmComponent() = default;
    int isPldmImageValid(const std::string& /*image*/);
    int try_pldm_update(const std::string& /*image*/, bool /*force*/);
    virtual int comp_update(const std::string& /*image*/) { return FW_STATUS_NOT_SUPPORTED; }
};