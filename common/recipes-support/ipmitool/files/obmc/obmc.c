
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <ipmitool/ipmi.h>
#include <ipmitool/ipmi_intf.h>
#include <ipmitool/ipmi_sel.h>
#include <ipmitool/helper.h>
#include <ipmitool/log.h>

#include <openbmc/ipmb.h>
#include <openbmc/ipmi.h>

extern int verbose;
#define IPMI_OBMC_RETRY 4

int ipmi_obmc_setup(struct ipmi_intf * intf)
{
	/* set default payload size */
	intf->max_request_data_size = MAX_IPMB_RES_LEN;
	intf->max_response_data_size = MAX_IPMB_RES_LEN;
	return 0;
}

int ipmi_obmc_open(struct ipmi_intf * intf)
{
	struct ipmi_session_params *params;

	params = &intf->ssn_params;
	if (!params->retry)
		params->retry = IPMI_OBMC_RETRY;

	intf->opened = 1;

	return 0;
}

void ipmi_obmc_close(struct ipmi_intf * intf)
{
	intf->opened = 0;
}

static struct ipmi_rs *
ipmi_obmc_send_cmd(struct ipmi_intf * intf, struct ipmi_rq * req)
{
	static struct ipmi_rs rsp;
	ipmb_req_t *ipmb_req = ipmb_txb();
	ipmb_res_t *ipmb_res = ipmb_rxb();
	unsigned char *tbuf = (unsigned char *)ipmb_req;
	unsigned char *rbuf = (unsigned char *)ipmb_res;
	int retval = 0;
	int bus;
	int try = 0;

	if (intf == NULL || req == NULL)
		return NULL;

	if (req->msg.data_len + MIN_IPMB_REQ_LEN >  MAX_IPMB_RES_LEN)
		return NULL;

	if (intf->target_addr != 0 &&
	    intf->target_addr != intf->my_addr) {
		// IPMB
		ipmb_req->res_slave_addr = intf->target_addr;
		ipmb_req->netfn_lun = req->msg.netfn << 2;
		ipmb_req->netfn_lun |= req->msg.lun;
		ipmb_req->cmd = req->msg.cmd;
		memcpy(ipmb_req->data, req->msg.data, req->msg.data_len);
		for (try=0; try < intf->ssn_params.retry; try++) {
			retval = ipmb_send_buf(
				pal_channel_to_bus(intf->target_channel),
				req->msg.data_len + MIN_IPMB_REQ_LEN);
			if (retval >= MIN_IPMB_RES_LEN) {
				rsp.ccode = ipmb_res->cc;
				rsp.data_len = retval - MIN_IPMB_RES_LEN;
				memcpy(rsp.data, ipmb_res->data, rsp.data_len);
				rsp.data[rsp.data_len] = 0;
				return &rsp;
			}
		}
	} else {
		// IPMI
		tbuf[0] = pal_devnum_to_fruid(intf->devnum);
		tbuf[1] = req->msg.netfn << 2;
		tbuf[1] |= req->msg.lun;
		tbuf[2] = req->msg.cmd;
		memcpy(&tbuf[3], req->msg.data, req->msg.data_len);
		for (try=0; try < intf->ssn_params.retry; try++) {
      unsigned short reslen = 0;
			lib_ipmi_handle(tbuf, req->msg.data_len + 3, rbuf, &reslen);
			if (reslen >= 3) {
				rsp.ccode = rbuf[2];
				rsp.data_len = reslen - 3;
				memcpy(rsp.data, &rbuf[3], rsp.data_len);
				rsp.data[rsp.data_len] = 0;
				return &rsp;
			}
		}
	}

	return NULL;
}

struct ipmi_intf ipmi_obmc_intf = {
.name = "obmc",
.desc = "Linux OpenBMC Interface",
.setup = ipmi_obmc_setup,
.open = ipmi_obmc_open,
.close = ipmi_obmc_close,
.sendrecv = ipmi_obmc_send_cmd,
};
