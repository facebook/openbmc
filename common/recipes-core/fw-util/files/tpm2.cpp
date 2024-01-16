#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.hpp>
#include "tpm2.h"

using namespace std;

#define MAX_LINE_LENGTH 80

int Tpm2Component::get_version(json& j) {

  j["PRETTY_COMPONENT"] = "TPM";
  j["VERSION"] = "NA";
  if (device.empty() ||
      version_command.size() == 0 ||
      version_cache.empty() ||
      access(device.c_str(), F_OK) != 0
     ) {
    return FW_STATUS_SUCCESS;
  }
  try {
    j["VERSION"] = kv::get(version_cache);
  } catch (std::exception&) {
    bool match = false;
    uint32_t fw_ver = 0;
    for (size_t i = 0; !match && i < version_command.size(); i++) {
      FILE *fp = popen(version_command[i].c_str(), "r");
      if (!fp) {
        syslog(LOG_WARNING, "Cannot open TPM2 file");
        return FW_STATUS_SUCCESS;
      }
      char value[MAX_LINE_LENGTH] = {0};
      while (fgets(value, sizeof(value), fp)) {
        if (sscanf(value, "%*s %x", &fw_ver) == 1) {
          string vers = std::to_string(fw_ver >> 16) + "."
                  + std::to_string(fw_ver & 0xFFFF);
          j["VERSION"] = vers;
          try {
            kv::set(version_cache, vers);
          } catch (std::exception&) {
            syslog(LOG_WARNING, "Cannot cache TPM2 version");
          }
          match = true;
          break;
        }
      }
      pclose(fp);
    }
  }
  return FW_STATUS_SUCCESS;
}

#ifdef CONFIG_TPM2
Tpm2Component tpm("bmc", "tpm");
#endif
