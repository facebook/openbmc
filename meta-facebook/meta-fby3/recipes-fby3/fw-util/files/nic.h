#ifndef _NIC_H_
#define _NIC_H_
#include "fw-util.h"

#define NIC_FW_VER_PATH "/tmp/cache_store/nic_fw_ver"

using namespace std;

class NicComponent : public Component {
  protected:
    std::string _ver_path = NIC_FW_VER_PATH;
  public:
    NicComponent(string fru, string board, string comp)
      : Component(fru, board, comp) {}
    int print_version();
};

#endif
