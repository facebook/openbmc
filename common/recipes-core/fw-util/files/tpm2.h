#ifndef _TPM2_H_
#define _TPM2_H_

#include "fw-util.h"

class Tpm2Component : public Component {
  public:
    Tpm2Component(std::string fru, std::string comp)
      : Component(fru, comp) {}
    int print_version();
};

#endif
