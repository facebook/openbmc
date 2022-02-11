#include "tpm.h"
#include <syslog.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <openbmc/kv.hpp>

using namespace std;

#define MAX_LINE_LENGTH 80
#define TPM_DEV "/sys/class/tpm/tpm0"
#define TPM_VERSION_LOCATION "/sys/class/tpm/tpm0/device/caps"
#define TPM_FW_VER_MATCH_STR "Firmware version: "

int
get_tpm_ver(char *ver) {
  FILE *fp = NULL;
  char str[MAX_LINE_LENGTH] = {0};
  char *match = NULL;
  std::string ver_str;
  const std::string ver_key = "tpm_version_tmp";

  // Check is TPM is present/supported by the platform
  if(access(TPM_DEV, F_OK) == -1) {

    return FW_STATUS_NOT_SUPPORTED;
  }

  try {
    ver_str = kv::get(ver_key);
  } catch (std::exception& e) {
    //If kv has no TPM version cache file, then copy it from TPM driver node
    //Read TPM version from driver node takes over 10 times longer than read from cached file
    //That's why we cache the TPM Version at the frist time of the query

    //Open the TPM version node
    fp = fopen(TPM_VERSION_LOCATION, "r");
    if (fp == NULL){
      syslog(LOG_WARNING, "TPM File:%s, Open Fail for Read.", TPM_VERSION_LOCATION);
      return -1;
    }
    //Copy the TPM version to cache line by line
    while (fgets(str, sizeof(str), fp) != NULL) {
      match = strstr(str, TPM_FW_VER_MATCH_STR);
      if (match != NULL) {
        break;
      }
    }
    if (match == NULL) {
      syslog(LOG_WARNING, "Doesn't find TPM Firmware version");
      fclose(fp);
      return -1;
    }
    fclose(fp);
    ver_str.assign(match + strlen(TPM_FW_VER_MATCH_STR));
    // Set cache so next time we do not access the device.
    kv::set(ver_key, ver_str);
  }
  strncpy(ver, ver_str.c_str(), MAX_LINE_LENGTH - 1);
  return 0;
}

int TpmComponent::print_version()
{
  char ver[MAX_LINE_LENGTH] = {0};

  if (get_tpm_ver(ver) == FW_STATUS_SUCCESS) {
    printf("TPM Version: %s", ver);
  } else if (get_tpm_ver(ver) == FW_STATUS_NOT_SUPPORTED) {
    printf("TPM Version: TPM Not Supported\n");
  } else {
    printf("TPM Version: NA\n");
  }
  return 0;
}

#ifdef CONFIG_TPM1
TpmComponent tpm("bmc", "tpm");
#endif
