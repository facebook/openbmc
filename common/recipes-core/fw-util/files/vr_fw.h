#ifndef _VR_FW_H_
#define _VR_FW_H_

#include "fw-util.h"

class VrComponent : public Component {
  std::string dev_name;
  int update(std::string image, bool force);
  public:
    VrComponent(std::string fru, std::string comp, std::string name)
      : Component(fru, comp), dev_name(name) {}
    int get_version(json& j) override;
    int update(std::string image) override;
    int fupdate(std::string image) override;
};

#endif
