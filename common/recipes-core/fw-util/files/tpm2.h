#ifndef _TPM2_H_
#define _TPM2_H_

#include "fw-util.h"

class Tpm2Component : public Component {
  public:
    std::string device;
    std::string version_command;
    std::string verion_cache;
    Tpm2Component(std::string fru, std::string comp,
                  std::string dev = "/sys/class/tpm/tpm0",
                  std::string ver_cmd = "/usr/bin/tpm2_getcap -c properties-fixed 2>/dev/null" \
                  " | grep TPM_PT_FIRMWARE_VERSION_1",
                  std::string ver_cache = "tpm2_version")
      : Component(fru, comp), device(dev), version_command(ver_cmd), verion_cache(ver_cache) {}
    int print_version();
    void get_version(json& j);
};

#endif
