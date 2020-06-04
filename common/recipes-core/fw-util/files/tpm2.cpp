#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.h>
#include "tpm2.h"

using namespace std;

#define MAX_LINE_LENGTH 80

int tpm2_get_ver(char *ver, Tpm2Component *tpm2) {
  FILE *fp;
  char value[MAX_VALUE_LEN] = {0};
  bool match;
  uint32_t fw_ver;

  if (ver == NULL || tpm2->device.c_str() == NULL ||
      tpm2->version_command.c_str() == NULL ||
      tpm2->verion_cache.c_str() == NULL
     ) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  
  if (access(tpm2->device.c_str(), F_OK)) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  if (kv_get(tpm2->verion_cache.c_str(), value, NULL, 0)) {
    fp = popen(tpm2->version_command.c_str(), "r");
    if (!fp) {
      syslog(LOG_WARNING, "Cannot open TPM2 file");
      return FW_STATUS_FAILURE;
    }

    match = false;
    while (fgets(value, sizeof(value), fp)) {
      if (sscanf(value, "%*s %x", &fw_ver) == 1) {
        sprintf(value, "%d.%d", fw_ver >> 16, fw_ver & 0xFFFF);
        kv_set(tpm2->verion_cache.c_str(), value, 0, 0);
        match = true;
        break;
      }
    }
    pclose(fp);

    if (match != true) {
      return FW_STATUS_FAILURE;
    }
  }

  if (snprintf(ver, MAX_LINE_LENGTH, "%s", value) > (MAX_LINE_LENGTH-1))
    return FW_STATUS_FAILURE;

  return FW_STATUS_SUCCESS;
}

int Tpm2Component::print_version() {
  char ver[MAX_LINE_LENGTH] = {0};

  if (tpm2_get_ver(ver, this)) {
    cout << "TPM Version: NA" << endl;
  } else {
    cout << "TPM Version: " << string(ver) << endl;
  }

  return FW_STATUS_SUCCESS;
}

void Tpm2Component::get_version(json& j) {
  char ver[MAX_LINE_LENGTH] = {0};
  if (tpm2_get_ver(ver, this)) {
    j["VERSION"] = "error_returned";
  } else {
    j["VERSION"] = string(ver);
  }
  return;
}

#ifdef CONFIG_TPM2
Tpm2Component tpm("bmc", "tpm");
#endif
