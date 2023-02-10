#pragma once
#include "fw-util.h"
#include "mezz_nic.hpp"
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
  private:
    uint8_t id;
  public:
    SwbPexFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target, uint8_t id)
        :SwbBicFwComponent(fru, comp, bus, eid, target), id(id) {}
    int get_version(json& j) override;
    int update(string image);
    int fupdate(string image);
};

/*
class SwbAllPexFwComponent : public Component {
  private:
    uint8_t bus, eid;
    uint8_t target = PEX0_COMP;
  public:
    SwbAllPexFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid)
        :Component(fru, comp), bus(bus), eid(eid) {}
    int update(string image);
    int fupdate(string image);
};
*/

//SWB NIC Component
class SwbNicFwComponent : public SwbBicFwComponent {
  private:
    uint8_t id;
  public:
    SwbNicFwComponent(const string& fru, const string& comp, uint8_t bus, uint8_t eid, uint8_t target, uint8_t id)
        :SwbBicFwComponent(fru, comp, bus, eid, target), id(id) {}
    int get_version(json& j) override;
};

