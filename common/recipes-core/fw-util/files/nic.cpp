#include <cstdio>
#include <cstring>
#include <string>
#include <openbmc/kv.hpp>
#include "nic.h"

#define NCSI_DATA_PAYLOAD 64
#define NCSI_MIN_DATA_PAYLOAD 36

typedef struct {
  char mfg_name[10];  // manufacture name
  uint32_t mfg_id;    // manufacture id
  std::string (*get_nic_fw)(uint8_t *);
} nic_info_st;

static std::string
get_mlx_fw_version(uint8_t *buf) {
  int ver_index_based = 20;
  int major = 0;
  int minor = 0;
  int revision = 0;

  major = buf[ver_index_based++];
  minor = buf[ver_index_based++];
  revision = buf[ver_index_based++] << 8;
  revision += buf[ver_index_based++];

  return std::to_string(major) + "." + std::to_string(minor) + "." +
      std::to_string(revision);
}

static std::string
get_bcm_fw_version(uint8_t *buf) {
  int ver_start_index = 8;  // the index is defined in the NC-SI spec
  int i = 0;
  const int ver_end_index = 19;
  char ver[32] = {0};

  for ( ; ver_start_index <= ver_end_index; ver_start_index++, i++) {
    if (!buf[ver_start_index]) {
      break;
    }
    ver[i] = buf[ver_start_index];
  }

  return ver;
}

static const nic_info_st support_nic_list[] = {
  {"Mellanox", 0x19810000, get_mlx_fw_version},
  {"Broadcom", 0x3d110000, get_bcm_fw_version},
};
static int nic_list_size = sizeof(support_nic_list) / sizeof(nic_info_st);

int NicComponent::print_version() {
  std::string display_nic_str{};
  std::string vendor{};
  std::string version{};
  uint32_t nic_mfg_id = 0;
  bool is_unknown_mfg_id = true;
  int current_nic;
  std::string buf;

  try {
    buf = kv::get(_ver_key, kv::region::temp);
    if (buf.size() < NCSI_MIN_DATA_PAYLOAD) {
      return FW_STATUS_FAILURE;
    }
  } catch(std::exception& e) {
    return FW_STATUS_FAILURE;
  }

  // get the manufcture id
  nic_mfg_id = (buf[35]<<24) | (buf[34]<<16) | (buf[33]<<8) | buf[32];

  for (current_nic = 0; current_nic < nic_list_size; current_nic++) {
    // check the nic on the system is supported or not
    if (support_nic_list[current_nic].mfg_id == nic_mfg_id) {
      vendor = support_nic_list[current_nic].mfg_name;
      version = support_nic_list[current_nic].get_nic_fw((uint8_t *)buf.data());
      is_unknown_mfg_id = false;
      break;
    }
  }

  if (is_unknown_mfg_id) {
    char mfg_id[5] = {0};
    sprintf(mfg_id, "%04x", nic_mfg_id);
    display_nic_str = "NIC firmware version: NA (Unknown Manufacture ID: 0x" +
      std::string(mfg_id) + ")";
  }
  else {
    display_nic_str = vendor + "NIC firmware version: " + version;
  }
  std::cout << display_nic_str << std::endl;

  return FW_STATUS_SUCCESS;
}
