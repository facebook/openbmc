#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.hpp>
#include "tpm2.h"

using namespace std;

#define MAX_LINE_LENGTH 80
#define MANUFACTURER_NUVOTON 0x4E544300
#define TPM2_IFX 0
#define TPM2_NUVOTON 1

bool Tpm2Component::read_Tpm2_cmd_info(FILE* stream, const std::string& pattern, uint32_t& hexValue) {
  char buffer[MAX_LINE_LENGTH] = {0};
  while (fgets(buffer, sizeof(buffer), stream) != NULL) {
    if (sscanf(buffer, pattern.c_str(), &hexValue) == 1) {
      return true;
    }
  }
  return false;
}

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

    int tpm2_vendor = 0;
    uint32_t manufacturer_id = 0;

    FILE* fp_id = popen(version_vendorID.c_str(), "r");
    FILE* fp_ver1 = popen(version_command[0].c_str(), "r");
    FILE* fp_ver2 = popen(version_command[1].c_str(), "r");

    if ((!fp_id) || (!fp_ver1) || (!fp_ver2)) {
      syslog(LOG_WARNING, "Cannot get TPM2 Information!");
      return FW_STATUS_FAILURE;
    }

    // use TPM2_PT_MANUFACTURER to get TPM Vendor
    if (read_Tpm2_cmd_info(fp_id, "  raw: 0x%x", manufacturer_id)) {
      if (manufacturer_id == MANUFACTURER_NUVOTON) {
        tpm2_vendor = TPM2_NUVOTON;
      } else {
        tpm2_vendor = TPM2_IFX;
      }
    }
    pclose(fp_id);

    // IFX     : TPM Version = TPM2_PT_FIRMWARE_VERSION_1
    // Nuvoton : TPM Version = TPM2_PT_FIRMWARE_VERSION_1 + TPM2_PT_FIRMWARE_VERSION_2
    string tpm_version_1;
    uint32_t fw_ver_1 = 0;
    if (read_Tpm2_cmd_info(fp_ver1, "%*s %x", fw_ver_1)) {
      tpm_version_1 = std::to_string(fw_ver_1 >> 16) + "." + std::to_string(fw_ver_1 & 0xFFFF);
    }
    pclose(fp_ver1);

    string tpm_version_2;
    uint32_t fw_ver_2 = 0;
    if (read_Tpm2_cmd_info(fp_ver2, "%*s %x", fw_ver_2)) {
      tpm_version_2 = std::to_string(fw_ver_2 >> 16) + "." + std::to_string(fw_ver_2 & 0xFFFF);
    }
    pclose(fp_ver2);

    // Set TPM Version
    string tpm_version;
    if (tpm2_vendor) { //for Nuvoton
      tpm_version = tpm_version_1 + "." + tpm_version_2;
    } else { //for IFX
      tpm_version = tpm_version_1;
    }
    j["VERSION"] = tpm_version;
    try {
      kv::set(version_cache, tpm_version);
    } catch (std::exception&) {
      syslog(LOG_WARNING, "Cannot cache TPM2 version!");
    }
  }
  return FW_STATUS_SUCCESS;
}

#ifdef CONFIG_TPM2
Tpm2Component tpm("bmc", "tpm");
#endif
