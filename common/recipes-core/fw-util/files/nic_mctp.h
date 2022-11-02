#ifndef _NIC_MCTP_H_
#define _NIC_MCTP_H_

#include "nic.h"

class MCTPNicComponent : public NicComponent {
  protected:
    uint8_t _eid;
  public:
    MCTPNicComponent(std::string fru, std::string comp, std::string key, uint8_t eid)
      : NicComponent(fru, comp, key), _eid(eid) {}
};

class MCTPOverSMBusNicComponent : public MCTPNicComponent {
  protected:
    uint8_t _bus_id;
    uint8_t _addr;
  public:
    MCTPOverSMBusNicComponent(std::string fru, std::string comp, std::string key, uint8_t eid, uint8_t bus)
      : MCTPNicComponent(fru, comp, key, eid), _bus_id(bus) {}
    int get_version(json& j);
    int update(std::string image);
};
#endif
