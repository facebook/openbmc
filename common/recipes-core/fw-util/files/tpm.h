#ifndef _TPM_H_
#define _TPM_H_
#include <string>
#include "fw-util.h"

class TpmComponent : public Component {
  public:
    TpmComponent(std::string fru, std::string comp)
      : Component(fru, comp) {}
    int print_version();
};

#endif
