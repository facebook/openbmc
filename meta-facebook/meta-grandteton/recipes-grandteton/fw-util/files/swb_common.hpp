#pragma once
#include "fw-util.h"
#include "mezz_nic.hpp"
#include "pldm_comp.hpp"
#include "signed_info.hpp"
#include <facebook/bic.h>
#include <sstream>
#include <string>
#include <openbmc/pal.h>

using namespace std;
//SWB BIC Component
class SwbBicFwComponent : public Component {
  protected:
    uint8_t bus, eid, target;
  public:
    SwbBicFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        : Component(fru, comp), bus(bus), eid(eid), target(target) {}
    int update(string image) override;
    int fupdate(string image) override;
    int get_version(json& j) override;
};

class SwbBicFwRecoveryComponent : public Component {
  protected:
    uint8_t bus, eid, target;

  public:
    SwbBicFwRecoveryComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        :Component(fru, comp), bus(bus), eid(eid), target(target) {}

  int update(string image) override;
  int fupdate(string image) override;
};


//SWB PEX Component
class SwbPexFwComponent : public SwbBicFwComponent {
  public:
    SwbPexFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        :SwbBicFwComponent(fru, comp, bus, eid, target) {}
    int get_version(json& j) override;
    int update(string image);
    int fupdate(string image);
};

class GTPldmComponent : public PldmComponent {
  protected:
    void store_device_id_record(pldm_firmware_device_id_record& /*id_record*/,
                                uint16_t& /*descriper_type*/,
                                variable_field& /*descriper_data*/) override;
    void store_comp_img_info(pldm_component_image_information& /*comp_info*/,
                                variable_field& /*comp_verstr*/) override;
  public:
    GTPldmComponent(const signed_header_t& info, const std::string &comp,
                  uint8_t bus, uint8_t eid): PldmComponent(info, comp, bus, eid) {}
    void print();
};

class GTSwbBicFwComponent : public SwbBicFwComponent, public GTPldmComponent {
  public:
    GTSwbBicFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid,
                        uint8_t target, const signed_header_t& info) :
                        SwbBicFwComponent(fru, comp, bus, eid, target),
                        GTPldmComponent(info, comp, bus, eid){}
    int update(string /*image*/) override;
    int fupdate(string /*image*/) override;
    int comp_update(const string& image) { return SwbBicFwComponent::update(image); }
};

class GTSwbPexFwComponent : public SwbPexFwComponent, public GTPldmComponent {
  public:
    GTSwbPexFwComponent(const string& fru, const string& comp, uint8_t bus,
                        uint8_t eid, uint8_t target, const signed_header_t& info):
                        SwbPexFwComponent(fru, comp, bus, eid, target),
                        GTPldmComponent(info, comp, bus, eid){}
    int update(string /*image*/) override;
    int fupdate(string /*image*/) override;
    int comp_update(const string& image) { return SwbPexFwComponent::update(image); }
};