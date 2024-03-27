#pragma once
#include "fw-util.h"
#include "mezz_nic.hpp"
#include "pldm_comp.hpp"
#include "signed_info.hpp"
#include <facebook/bic.h>
#include <sstream>
#include <string>
#include <openbmc/pal.h>
#include <openbmc/vr.h>
#include "vr_fw.h"
#include "cpld.hpp"

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

//SWB VR Component
class SwbVrComponent : public VrComponent {
    std::string name;
    int update_proc(string image, bool force);
  public:
    SwbVrComponent(const string &fru, const string &comp, const string &dev_name)
        :VrComponent(fru, comp, dev_name), name(dev_name) {}
    int fupdate(string image);
    int update(string image);
};

// Artemis ACB PCIE SWITCH Component
class AcbPeswFwComponent : public SwbPexFwComponent {
  public:
    AcbPeswFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        :SwbPexFwComponent(fru, comp, bus, eid, target) {}
    int get_version(json& j) override;
};

class GTPldmComponent : public PldmComponent {
  protected:
    std::string _fru;
    std::string _comp;
    string comp_verify_str{};
    int is_pldm_info_valid();

  public:
    GTPldmComponent(const signed_header_t& info, const string& fru, const std::string &comp,
                  uint8_t bus, uint8_t eid): PldmComponent(info, fru, comp, bus, eid), _fru(fru), _comp(comp) {}
    int try_pldm_update(const std::string& /*image*/, bool /*force*/, uint8_t specified_comp = 0xFF);
    int gt_get_version(json& /*j*/, const string& /*fru*/, const string& /*comp*/, uint8_t /*target*/);
    virtual int comp_version(json& /*j*/) { return -1; }
    virtual int comp_verify() { return INFO_ERR::EP_SUCCESS; };
};

class GTSwbBicFwComponent : public SwbBicFwComponent, public GTPldmComponent {
  public:
    GTSwbBicFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid,
                        uint8_t target, const signed_header_t& info) :
                        SwbBicFwComponent(fru, comp, bus, eid, target),
                        GTPldmComponent(info, fru, comp, bus, eid){}
    int update(string /*image*/) override;
    int fupdate(string /*image*/) override;
    int get_version(json& j) override;
    int comp_update(const string& image) override { return SwbBicFwComponent::update(image); }
    int comp_fupdate(const string& image) override { return SwbBicFwComponent::fupdate(image); }
    int comp_version(json& j) override { return SwbBicFwComponent::get_version(j); }
};

class GTSwbPexFwComponent : public SwbPexFwComponent, public GTPldmComponent {
  public:
    GTSwbPexFwComponent(const string& fru, const string& comp, uint8_t bus,
                        uint8_t eid, uint8_t target, const signed_header_t& info):
                        SwbPexFwComponent(fru, comp, bus, eid, target),
                        GTPldmComponent(info, fru, comp, bus, eid){}
    int update(string /*image*/) override;
    int fupdate(string /*image*/) override;
    int get_version(json& /*json*/) override;
    int comp_update(const string& image) override { return SwbPexFwComponent::update(image); }
    int comp_fupdate(const string& image) override { return SwbPexFwComponent::fupdate(image); }
    int comp_version(json& j) override { return SwbPexFwComponent::get_version(j); }
};

class GTSwbVrComponent : public SwbVrComponent, public GTPldmComponent {
  private:
    uint8_t target;
  public:
    GTSwbVrComponent(const string& fru, const string& comp, const string &dev_name,
                    uint8_t bus, uint8_t eid, uint8_t target, const signed_header_t& info):
                    SwbVrComponent(fru, comp, dev_name),
                    GTPldmComponent(info, fru, comp, bus, eid), target(target)
                    {
                      uint8_t source_id;
		      if (get_comp_source(FRU_SWB, SWB_VR_SOURCE, &source_id) == 0) {
                        switch (source_id) {
                          case MAIN_SOURCE:
                            comp_info.vendor_id = pldm_signed_info::RENESAS;
                            break;
                          case SECOND_SOURCE:
                            comp_info.vendor_id = pldm_signed_info::INFINEON;
                            break;
                          case THIRD_SOURCE:
                            comp_info.vendor_id = pldm_signed_info::MPS;
                            break;
                        }
                      }
                    }
    int update(string /*image*/) override;
    int fupdate(string /*image*/) override;
    int get_version(json& /*json*/) override;
    int comp_update(const string& image) override { return SwbVrComponent::update(image); }
    int comp_fupdate(const string& image) override { return SwbVrComponent::fupdate(image); }
    int comp_version(json& j) override { return SwbVrComponent::get_version(j); }
    int comp_verify();
};

class GTSwbCpldComponent : public CpldComponent, public GTPldmComponent {
  private:
    uint8_t target;
  public:
    GTSwbCpldComponent(const std::string &fru, const std::string &comp,
                      uint8_t type, uint8_t bus, uint8_t addr,
                      int (*cpld_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t),
                      uint8_t eid, uint8_t target, const signed_header_t& info):
                      CpldComponent(fru, comp, type, bus, addr, cpld_xfer),
                      GTPldmComponent(info, fru, comp, bus, eid), target(target){}
    int update(string /*image*/) override;
    int fupdate(string /*image*/) override;
    int get_version(json& /*json*/) override;
    int comp_update(const string& image) override { return CpldComponent::update(image); }
    int comp_fupdate(const string& image) override { return CpldComponent::fupdate(image); }
    int comp_version(json& j) override { return CpldComponent::get_version(j); }
};

class SwbPLDMNicComponent : public PLDMNicComponent {
  public:
    SwbPLDMNicComponent(const std::string& fru, const std::string& comp,
                        const std::string& key, uint8_t eid, uint8_t bus):
      PLDMNicComponent(fru, comp, key, eid, bus) {}
    int update(string /*image*/) override;
};

//Artemis VR Component
class GTAVrComponent : public GTSwbVrComponent {
  public:
    GTAVrComponent(const string& fru, const string& comp, const string &dev_name,
      uint8_t bus, uint8_t eid, uint8_t target, const signed_header_t& info):
      GTSwbVrComponent(fru, comp, dev_name, bus, eid, target, info)
      {
        uint8_t source_id;
        if (fru == "cb") {
          if (get_acb_vr_source(&source_id) == 0) {
            switch (source_id) {
              case MAIN_SOURCE:
                comp_info.vendor_id = pldm_signed_info::INFINEON;
                break;
              case SECOND_SOURCE:
                comp_info.vendor_id = pldm_signed_info::MPS;
                break;
            }
          }
        }
      }
};

//Artemis Meb Cxl Component
class MebCxlFwComponent : public SwbBicFwComponent {
  public:
    MebCxlFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        :SwbBicFwComponent(fru, comp, bus, eid, target) {}
    int get_version(json& j) override;
};

class GtaBicFwRecoveryComponent : public Component {
  protected:
    uint8_t bus, eid, target;

  public:
    GtaBicFwRecoveryComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target)
        :Component(fru, comp), bus(bus), eid(eid), target(target) {}

  int update(string image) override;
  int fupdate(string image) override;
};

class GTAASICComponent : public PldmComponent, public Component {
  public:
    GTAASICComponent (const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t component_identifier, 
        int wait_apply_time)
	: PldmComponent(fru, comp, bus, eid, component_identifier, wait_apply_time), Component(fru, comp) {}
    int get_version(json& /*json*/);
    int update(std::string image); // firmware update
    int fupdate(std::string image); // force firmware update
};
