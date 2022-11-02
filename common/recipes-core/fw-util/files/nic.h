#ifndef _NIC_H_
#define _NIC_H_

#include "fw-util.h"

#define NIC_FW_VER_KEY "nic_fw_ver"
#define MLX_MFG_ID 0x19810000
#define BCM_MFG_ID 0x3d110000

class NicComponent : public Component {
  protected:
    std::string _ver_key = NIC_FW_VER_KEY;
    int upgrade_ncsi_util(const std::string& img, int channel=-1);
    virtual int get_key(const std::string& key, std::string& buf);
  public:
    NicComponent(std::string fru, std::string comp)
      : Component(fru, comp) {}
    NicComponent(std::string fru, std::string comp, std::string ver_key_store)
      : Component(fru, comp), _ver_key(ver_key_store) {}
    int get_version(json& j) override;
    virtual int update(std::string img) override; 
};

#endif
