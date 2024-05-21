#ifndef _TPM2_H_
#define _TPM2_H_

#include "fw-util.h"

class Tpm2Component : public Component {
  public:
    std::string device{};
    std::vector<std::string> version_command{};
    std::string version_cache{};
    std::string version_vendorID{};
    Tpm2Component(std::string fru, std::string comp,
                  std::string dev = "/sys/class/tpm/tpm0",
                  std::vector<std::string> ver_cmd = {
                    "/usr/bin/tpm2_getcap properties-fixed 2>/dev/null | grep -A1 TPM2_PT_FIRMWARE_VERSION_1",
                    "/usr/bin/tpm2_getcap properties-fixed 2>/dev/null | grep -A1 TPM2_PT_FIRMWARE_VERSION_2"},
                  std::string ver_cache = "tpm2_version",
                  std::string ver_vendor =
                    "/usr/bin/tpm2_getcap properties-fixed 2>/dev/null | grep -A1 TPM2_PT_MANUFACTURER")
      : Component(fru, comp), device(dev), version_command(ver_cmd), version_cache(ver_cache), version_vendorID(ver_vendor) {}
    int get_version(json& j) override;
    bool read_Tpm2_cmd_info(FILE* stream, const std::string& pattern, uint32_t& hexValue);
};

#endif
