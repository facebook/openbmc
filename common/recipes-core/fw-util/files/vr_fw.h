#ifndef _VR_FW_H_
#define _VR_FW_H_

#include "fw-util.h"

class VrComponent : public Component {
  std::string dev_name;
  public:
    VrComponent(std::string fru, std::string comp, std::string name)
      : Component(fru, comp), dev_name(name) {}
    int print_version();
    int update(std::string image);
};

#endif
