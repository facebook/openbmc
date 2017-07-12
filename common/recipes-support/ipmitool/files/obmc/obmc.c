
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

int ipmi_obmc_setup(struct ipmi_intf * intf)
{
	/* set default payload size */
	intf->max_request_data_size = MAX_IPMB_RES_LEN;
	intf->max_response_data_size = MAX_IPMB_RES_LEN;
	return 0;
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
		retval = ipmb_send_buf(
			pal_channel_to_bus(intf->target_channel),
			req->msg.data_len + MIN_IPMB_REQ_LEN);
		if (retval < MIN_IPMB_RES_LEN) {
			return NULL;
		} else {
			rsp.ccode = ipmb_res->cc;
			rsp.data_len = retval - MIN_IPMB_RES_LEN;
			memcpy(rsp.data, ipmb_res->data, rsp.data_len);
			rsp.data[rsp.data_len] = 0;
		}
	} else {
		// IPMI
		tbuf[0] = pal_devnum_to_fruid(intf->devnum);
		tbuf[1] = req->msg.netfn << 2;
		tbuf[1] |= req->msg.lun;
		tbuf[2] = req->msg.cmd;
		memcpy(&tbuf[3], req->msg.data, req->msg.data_len);
		retval = 0;
		lib_ipmi_handle(tbuf, req->msg.data_len + 3, rbuf, &retval);
		if (retval < 3) {
			return NULL;
		} else {
			rsp.ccode = rbuf[2];
			rsp.data_len = retval - 3;
			memcpy(rsp.data, &rbuf[3], rsp.data_len);
			rsp.data[rsp.data_len] = 0;
		}
	}

	return &rsp;
}

struct ipmi_intf ipmi_obmc_intf = {
.name = "obmc",
.desc = "Linux OpenBMC Interface",
.setup = ipmi_obmc_setup,
.sendrecv = ipmi_obmc_send_cmd,
};
