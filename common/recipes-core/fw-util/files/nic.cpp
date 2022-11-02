#include <cstdio>
#include <cstring>
#include <string>
#include <syslog.h>
#include <openbmc/kv.hpp>
#include "nic.h"

#define NCSI_DATA_PAYLOAD 64
#define NCSI_MIN_DATA_PAYLOAD 36

typedef struct {
  char mfg_name[10];  // manufacture name
  uint32_t mfg_id;    // manufacture id
  std::string (*get_nic_fw)(const uint8_t *);
} nic_info_st;

static std::string
get_mlx_fw_version(const uint8_t *buf) {
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
get_bcm_fw_version(const uint8_t *buf) {
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
  {"Mellanox", MLX_MFG_ID, get_mlx_fw_version},
  {"Broadcom", BCM_MFG_ID, get_bcm_fw_version},
};
static int nic_list_size = sizeof(support_nic_list) / sizeof(nic_info_st);

int NicComponent::get_key(const std::string& key, std::string& buf)
{
  try {
    buf = kv::get(key, kv::region::temp);
    if (buf.size() < NCSI_MIN_DATA_PAYLOAD) {
      return FW_STATUS_FAILURE;
    }
  } catch(std::exception& e) {
    return FW_STATUS_FAILURE;
  }
  return FW_STATUS_SUCCESS;
}

int NicComponent::get_version(json& j) {
  std::string vendor{};
  std::string version{};
  uint32_t nic_mfg_id = 0;
  bool is_unknown_mfg_id = true;
  int current_nic;
  std::string _buf;

  if (get_key(_ver_key, _buf) != FW_STATUS_SUCCESS) {
    return FW_STATUS_FAILURE;
  }
  const uint8_t *buf = (uint8_t *)_buf.c_str();
  // get the manufcture id
  nic_mfg_id = (buf[35]<<24) | (buf[34]<<16) | (buf[33]<<8) | buf[32];

  for (current_nic = 0; current_nic < nic_list_size; current_nic++) {
    // check the nic on the system is supported or not
    if (support_nic_list[current_nic].mfg_id == nic_mfg_id) {
      vendor = support_nic_list[current_nic].mfg_name;
      version = support_nic_list[current_nic].get_nic_fw(buf);
      is_unknown_mfg_id = false;
      break;
    }
  }

  if (is_unknown_mfg_id) {
    char mfg_id[9] = {0};
    sprintf(mfg_id, "%04x", nic_mfg_id);
    j["PRETTY_COMPONENT"] = "NIC firmware";
    std::string display_nic_str = "NA (Unknown Manufacture ID: 0x" +
      std::string(mfg_id) + ")";
    j["VERSION"] = display_nic_str;
  }
  else {
    j["vendor"] = vendor;
    j["PRETTY_COMPONENT"] = vendor + " NIC firmware";
    j["VERSION"] = version;
  }
  return FW_STATUS_SUCCESS;
}

int NicComponent::upgrade_ncsi_util(const std::string& img, int channel)
{
  std::string cmd = "/usr/local/bin/ncsi-util";
  syslog(LOG_CRIT, "Component %s upgrade initiated\n", _component.c_str() );

  // If channel is provided, pass it to ncsi-util, else let it use a default
  if (channel >= 0) {
    cmd += " -c " + std::to_string(channel);
  }
  // Double quote the image to support paths with space.
  cmd += " -p \"" + img + "\"";
  int ret = sys().runcmd(cmd);
  if(ret)
    syslog(LOG_CRIT, "Component %s upgrade failed\n", _component.c_str() );
  else
    syslog(LOG_CRIT, "Component %s upgrade completed\n", _component.c_str() );

  return ret;
}

int NicComponent::update(std::string image)
{
  return upgrade_ncsi_util(image);
}
