#ifndef _NIC_H_
#define _NIC_H_

#include "fw-util.h"

#define NIC_FW_VER_PATH "/tmp/cache_store/nic_fw_ver"

class NicComponent : public Component {
  protected:
    std::string _ver_path = NIC_FW_VER_PATH;
  public:
    NicComponent(std::string fru, std::string comp)
      : Component(fru, comp) {}
    NicComponent(std::string fru, std::string comp, std::string path)
      : Component(fru, comp), _ver_path(path) {}
    int print_version();
};

#endif
