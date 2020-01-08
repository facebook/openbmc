#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <openbmc/pal.h>
#include "nic.h"

using namespace std;

class NicExtComponent : public NicComponent {
  uint8_t _if_idx;
  int _get_ncsi_vid(void);
  public:
    NicExtComponent(string fru, string comp, string path, uint8_t idx)
      : NicComponent(fru, comp, path), _if_idx(idx) {}
    int print_version();
};

int NicExtComponent::_get_ncsi_vid(void) {
  NCSI_NL_MSG_T *nl_msg;
  NCSI_NL_RSP_T *nl_rsp;
  Get_Version_ID_Response *vidresp;
  uint8_t nic_fw_ver[4] = {0};
  int fd, ret = -1;

  nl_msg = (NCSI_NL_MSG_T *)calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!nl_msg) {
    printf("%s: allocate nl_msg buffer failed\n", __func__);
    return -1;
  }

  sprintf(nl_msg->dev_name, "eth%u", _if_idx);
  nl_msg->channel_id = 0;
  nl_msg->cmd = NCSI_GET_VERSION_ID;
  nl_msg->payload_length = 0;

  do {
    nl_rsp = send_nl_msg_libnl(nl_msg);
    if (!nl_rsp) {
      break;
    }
    if (((NCSI_Response_Packet *)nl_rsp->msg_payload)->Response_Code) {
      break;
    }

    if (access("/tmp/cache_store", F_OK) == -1) {
      mkdir("/tmp/cache_store", 0777);
    }

    fd = open(_ver_path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
      break;
    }

    lseek(fd, (long)&((Get_Version_ID_Response *)0)->fw_ver, SEEK_SET);
    ret = read(fd, nic_fw_ver, sizeof(nic_fw_ver));
    if (ret && (ret != sizeof(nic_fw_ver))) {
      close(fd);
      break;
    }

    vidresp = (Get_Version_ID_Response *)((NCSI_Response_Packet *)nl_rsp->msg_payload)->Payload_Data;
    if (!ret || memcmp(vidresp->fw_ver, nic_fw_ver, sizeof(nic_fw_ver))) {
      lseek(fd, 0, SEEK_SET);
      if (write(fd, vidresp, sizeof(Get_Version_ID_Response)) == sizeof(Get_Version_ID_Response)) {
        syslog(LOG_WARNING, "updated %s", _ver_path.c_str());
      }
    }
    close(fd);
    ret = 0;
  } while (0);

  free(nl_msg);
  if (nl_rsp)
    free(nl_rsp);

  return ret;
}

int NicExtComponent::print_version() {
  uint8_t prsnt = 1;

  if (pal_is_fru_prsnt(FRU_NIC0+_if_idx, &prsnt) || !prsnt) {
    syslog(LOG_WARNING, "nic%u not present", _if_idx);
    return -1;
  }

  _get_ncsi_vid();
  return NicComponent::print_version();
}

NicExtComponent nic0("nic", "nic0", "/tmp/cache_store/nic_fw_ver", 0);
NicExtComponent nic1("nic", "nic1", "/tmp/cache_store/nic1_fw_ver", 1);
