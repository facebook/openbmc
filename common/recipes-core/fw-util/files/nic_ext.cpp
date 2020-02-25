#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <openbmc/kv.h>
#include <openbmc/ncsi.h>
#include <openbmc/nl-wrapper.h>
#include <openbmc/pal.h>
#include "nic_ext.h"

using namespace std;

int NicExtComponent::_get_ncsi_vid(void) {
  NCSI_NL_MSG_T *nl_msg;
  NCSI_NL_RSP_T *nl_rsp;
  char value[MAX_VALUE_LEN] = {0};
  Get_Version_ID_Response *vidresp, *vidcache;
  int ret = -1;

  nl_msg = (NCSI_NL_MSG_T *)calloc(1, sizeof(NCSI_NL_MSG_T));
  if (!nl_msg) {
    syslog(LOG_WARNING, "%s: allocate nl_msg buffer failed", __func__);
    return -1;
  }

  sprintf(nl_msg->dev_name, "eth%u", _if_idx);
  nl_msg->channel_id = _ch_id;
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

    ret = kv_get(_vid_key.c_str(), value, NULL, 0);
    vidcache = (Get_Version_ID_Response *)value;
    vidresp = (Get_Version_ID_Response *)((NCSI_Response_Packet *)nl_rsp->msg_payload)->Payload_Data;
    if (ret || memcmp(vidresp->fw_ver, vidcache->fw_ver, sizeof(vidresp->fw_ver))) {
      if (!kv_set(_vid_key.c_str(), (const char *)vidresp, sizeof(Get_Version_ID_Response), 0))
        syslog(LOG_WARNING, "updated %s", _vid_key.c_str());
    }
    ret = 0;
  } while (0);

  free(nl_msg);
  if (nl_rsp)
    free(nl_rsp);

  return ret;
}

int NicExtComponent::print_version() {
  uint8_t prsnt = 1;

  if (pal_is_fru_prsnt(_fru_id, &prsnt) || !prsnt) {
    syslog(LOG_WARNING, "nic%u not present", _if_idx);
    return -1;
  }

  _get_ncsi_vid();
  return NicComponent::print_version();
}
