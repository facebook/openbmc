#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.h>
#include "tpm2.h"

using namespace std;

#define MAX_LINE_LENGTH 80
#define TPM_DEV "/sys/class/tpm/tpm0"
#define TPM2_VERSION_CMD "/usr/bin/tpm2_getcap -c properties-fixed 2>/dev/null | grep TPM_PT_FIRMWARE_VERSION_1"
#define TPM2_VERSION_CACHE "tpm2_version"

int tpm2_get_ver(char *ver) {
  FILE *fp;
  char value[MAX_VALUE_LEN] = {0};
  bool match;
  uint32_t fw_ver;

  if (access(TPM_DEV, F_OK)) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  if (kv_get(TPM2_VERSION_CACHE, value, NULL, 0)) {
    fp = popen(TPM2_VERSION_CMD, "r");
    if (!fp) {
      syslog(LOG_WARNING, "Cannot open TPM2 file");
      return FW_STATUS_FAILURE;
    }

    match = false;
    while (fgets(value, sizeof(value), fp)) {
      if (sscanf(value, "%*s %x", &fw_ver) == 1) {
        sprintf(value, "%d.%d", fw_ver >> 16, fw_ver & 0xFFFF);
        kv_set(TPM2_VERSION_CACHE, value, 0, 0);
        match = true;
        break;
      }
    }
    fclose(fp);

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

  if (tpm2_get_ver(ver)) {
    cout << "TPM Version: NA" << endl;
  } else {
    cout << "TPM Version: " << string(ver) << endl;
  }

  return 0;
}
