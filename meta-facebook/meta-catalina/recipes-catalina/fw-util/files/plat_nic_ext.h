#ifndef _PLAT_NIC_EXT_H_
#define _PLAT_NIC_EXT_H_

#include "nic_ext.h"

class PlatformNicExtComponent : public NicExtComponent {
  public:
    PlatformNicExtComponent(const std::string& fru, const std::string& comp, const std::string& key, uint8_t fruid, uint8_t idx, uint8_t chid = 0x00)
      : NicExtComponent(fru, comp, key, fruid, idx, chid)
      {}

    int fupdate(const std::string& image);
};

#endif
