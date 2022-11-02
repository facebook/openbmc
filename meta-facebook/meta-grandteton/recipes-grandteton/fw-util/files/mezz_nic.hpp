#ifndef _MEZZ_NIC_H_
#define _MEZZ_NIC_H_

#include "nic_mctp.h"

#ifdef __cplusplus
extern "C" {
#endif

class PLDMNicComponent : public MCTPNicComponent {
  protected:
    uint8_t _bus_id;
  public:
    PLDMNicComponent(std::string fru, std::string comp, std::string key, uint8_t eid, uint8_t bus)
      : MCTPNicComponent(fru, comp, key, eid), _bus_id(bus){}
    int get_version(json& j) override;
    int update(std::string image) override;
};

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
