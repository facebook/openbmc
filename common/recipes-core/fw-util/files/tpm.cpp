#include "tpm.h"
#include <syslog.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>

using namespace std;

#define MAX_LINE_LENGTH 80
#define TPM_DEV "/sys/class/tpm/tpm0"
#define TPM_VERSION_LOCATION "/sys/class/tpm/tpm0/device/caps"
#define TPM_VERSION_LOCATION_TMP "/tmp/cache_store/tpm_version_tmp"
#define TPM_FW_VER_MATCH_STR "Firmware version: "

int
get_tpm_ver(char *ver) {
  FILE *fp = NULL;
  FILE *fp_2 = NULL;
  char str[MAX_LINE_LENGTH] = {0};
  char *match = NULL;

  // Check is TPM is present/supported by the platform
  if(access(TPM_DEV, F_OK) == -1) {

    return FW_STATUS_NOT_SUPPORTED;
  }

  //If TPM_VERSION_LOCATION_TMP has no TPM version cache file, then copy it from TPM driver node
  //Read TPM version from driver node takes over 10 times longer than read from cached file
  //That's why we cache the TPM Version at the frist time of the query
  if(access(TPM_VERSION_LOCATION_TMP, F_OK) == -1) {

    //Open the TPM version node
    fp = fopen(TPM_VERSION_LOCATION, "r");
    if (fp == NULL){
      syslog(LOG_WARNING, "TPM File:%s, Open Fail for Read.", TPM_VERSION_LOCATION);
      return -1;
    }

    //Open the TPM version cache file
    fp_2 = fopen(TPM_VERSION_LOCATION_TMP, "w");
    if (fp_2 == NULL){
      syslog(LOG_WARNING, "TPM File:%s, Open Fail for Write.", TPM_VERSION_LOCATION_TMP);
      return -1;
    }

    //Copy the TPM version to cache line by line
    while (fgets(str, sizeof(str), fp) != NULL) {
      fputs(str, fp_2);
    }
    fclose(fp);
    fclose(fp_2);
  }

  fp = fopen(TPM_VERSION_LOCATION_TMP, "r");
  if (fp == NULL){
      syslog(LOG_WARNING, "TPM File:%s, Open Fail for Read.", TPM_VERSION_LOCATION_TMP);
      return -1;
  }

  //Search for "Firmware version" string in TMP version cache file
  while (fgets(str, sizeof(str), fp) != NULL) {
    match = strstr(str, TPM_FW_VER_MATCH_STR);
    if (match != NULL) {
      break;
    }
  }

  //Doesn't find match for "Firmware version" string in TMP version cache file
  //The reasone could be TPM version node is containing wrong data, or cache file is empty
  //Remove the cache file for next query
  if (match == NULL){
    syslog(LOG_WARNING, "Doesn't find TPM Firmware version at: %s", TPM_VERSION_LOCATION_TMP);
    remove(TPM_VERSION_LOCATION_TMP);
    return -1;
  }

  //Offset "TPM_FW_VER_MATCH_STR" units, only return TPM version number
  memcpy(ver, (match + strlen(TPM_FW_VER_MATCH_STR)), (strlen(str) - strlen(TPM_FW_VER_MATCH_STR)) );
  fclose(fp);

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
