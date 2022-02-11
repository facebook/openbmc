/*
 * kcs-test
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>

/*
 * NetFNs
 */
#define IPMI_CHASSIS_NETFN		0x00
#define IPMI_BRIDGE_NETFN		0x02
#define IPMI_SENSOR_EVENT_NETFN		0x04
#define IPMI_APP_NETFN			0x06
#define IPMI_FIRMWARE_NETFN		0x08
#define IPMI_STORAGE_NETFN		0x0a
#define IPMI_TRANSPORT_NETFN		0x0c
#define IPMI_GROUP_EXTENSION_NETFN	0x2c
#define IPMI_OEM_GROUP_NETFN		0x2e

//#define IPMI_APP_NETFN	6
#define IPMI_GET_DEVICE_ID_CMD			0x01
#define IPMI_BROADCAST_GET_DEVICE_ID_CMD	0x01
#define IPMI_COLD_RESET_CMD			0x02
#define IPMI_WARM_RESET_CMD			0x03
#define IPMI_GET_SELF_TEST_RESULTS_CMD		0x04
#define IPMI_MANUFACTURING_TEST_ON_CMD		0x05
#define IPMI_SET_ACPI_POWER_STATE_CMD		0x06
#define IPMI_GET_ACPI_POWER_STATE_CMD		0x07
#define IPMI_GET_DEVICE_GUID_CMD		0x08
#define IPMI_RESET_WATCHDOG_TIMER_CMD		0x22
#define IPMI_SET_WATCHDOG_TIMER_CMD		0x24
#define IPMI_GET_WATCHDOG_TIMER_CMD		0x25
#define IPMI_SET_BMC_GLOBAL_ENABLES_CMD		0x2e
#define IPMI_GET_BMC_GLOBAL_ENABLES_CMD		0x2f
#define IPMI_CLEAR_MSG_FLAGS_CMD		0x30
#define IPMI_GET_MSG_FLAGS_CMD			0x31
#define IPMI_ENABLE_MESSAGE_CHANNEL_RCV_CMD	0x32
#define IPMI_GET_MSG_CMD			0x33
#define IPMI_SEND_MSG_CMD			0x34
#define IPMI_READ_EVENT_MSG_BUFFER_CMD		0x35
#define IPMI_GET_BT_INTERFACE_CAPABILITIES_CMD	0x36
#define IPMI_GET_SYSTEM_GUID_CMD		0x37
#define IPMI_GET_CHANNEL_AUTH_CAPABILITIES_CMD	0x38
#define IPMI_GET_SESSION_CHALLENGE_CMD		0x39
#define IPMI_ACTIVATE_SESSION_CMD		0x3a
#define IPMI_SET_SESSION_PRIVILEGE_CMD		0x3b
#define IPMI_CLOSE_SESSION_CMD			0x3c
#define IPMI_GET_SESSION_INFO_CMD		0x3d

#define IPMI_GET_AUTHCODE_CMD			0x3f
#define IPMI_SET_CHANNEL_ACCESS_CMD		0x40
#define IPMI_GET_CHANNEL_ACCESS_CMD		0x41
#define IPMI_GET_CHANNEL_INFO_CMD		0x42
#define IPMI_SET_USER_ACCESS_CMD		0x43
#define IPMI_GET_USER_ACCESS_CMD		0x44
#define IPMI_SET_USER_NAME_CMD			0x45
#define IPMI_GET_USER_NAME_CMD			0x46
#define IPMI_SET_USER_PASSWORD_CMD		0x47
#define IPMI_ACTIVATE_PAYLOAD_CMD		0x48
#define IPMI_DEACTIVATE_PAYLOAD_CMD		0x49
#define IPMI_GET_PAYLOAD_ACTIVATION_STATUS_CMD	0x4a
#define IPMI_GET_PAYLOAD_INSTANCE_INFO_CMD	0x4b
#define IPMI_SET_USER_PAYLOAD_ACCESS_CMD	0x4c
#define IPMI_GET_USER_PAYLOAD_ACCESS_CMD	0x4d
#define IPMI_GET_CHANNEL_PAYLOAD_SUPPORT_CMD	0x4e
#define IPMI_GET_CHANNEL_PAYLOAD_VERSION_CMD	0x4f
#define IPMI_GET_CHANNEL_OEM_PAYLOAD_INFO_CMD	0x50

#define IPMI_MASTER_READ_WRITE_CMD		0x52

#define IPMI_GET_CHANNEL_CIPHER_SUITES_CMD	0x54
#define IPMI_SUSPEND_RESUME_PAYLOAD_ENCRYPTION_CMD 0x55
#define IPMI_SET_CHANNEL_SECURITY_KEY_CMD	0x56
#define IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_CMD 0x57


/* Setion 9.2 KCS Request Message Format */
typedef struct
{
  unsigned char netfn_lun;  //byte 1 : netfn [8:2], lun[1:0]
  unsigned char cmd;		//byte 2
  unsigned char data[];		//byte 3:N
} ipmi_req_t;

// IPMI response Structure (IPMI/Section 9.3)
typedef struct
{
  unsigned char netfn_lun;
  unsigned char cmd;
  unsigned char cc;
  unsigned char data[];
} ipmi_res_t;

static void
usage(FILE *fp, int argc, char **argv)
{
	fprintf(fp,
		"Usage: %s [options]\n\n"
		"Options:\n"
		" -h | --help                 Print this message\n"
		" -n | --node                  device node [/dev/ast-kcs.2]\n"
		" -a | --addr                 address [CA2]\n"
		" -d | --debug                  debug mode\n"
		"",
		argv[0]);
}

static const char short_options [] = "hdn:a:";

static const struct option
long_options [] = {
        { "help",       no_argument,            	NULL,   	'h' },
        { "node",      required_argument,     	NULL,   	'n' },
        { "addr",	 required_argument,      NULL,   	'a' },
        { "debug",		no_argument,		NULL,	'd' },
        { 0, 0, 0, 0 }
};


static unsigned char devid_data[] = {
	0x20, 0x01, 0x00, 0x48, 0x02, 0x9f, 0x22, 0x03, 0x00, 0x11, 0x43,
	0x00, 0x11, 0x00, 0x04
};

static unsigned char guid_data[] = {
	0x00, 0x01, 0x00, 0x48, 0x02, 0x9f, 0xaa, 0x01,
	0x00, 0x23, 0x00, 0x00, 0x11, 0x00, 0x04, 0x99
};

static unsigned char bt_interface_data[] = {
	0x01, 0x40, 0x40, 0x05, 0x03
};

static unsigned char test_data[] = {
	0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0C, 0x0D, 0x0E,
	0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
	0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
	0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36
};

#if 0
	0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
	0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
	0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,
	0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0xff
#endif


static unsigned char self_test_data[] = {
	0x55, 0x00
};

int debug = 0;

int ipmi_app_handler(ipmi_req_t *req_buf, unsigned char *req_len, ipmi_res_t* res_buf, unsigned char *res_len)
{
	int ret = 0;
	switch(req_buf->cmd) {
		//for performace test
		case 0xF:
			if(debug) printf("performance test\n");
			res_buf->netfn_lun = 0x1c;
			res_buf->cmd = 0xf;
			res_buf->cc = 0;
			*res_len += sizeof(test_data);
			memcpy(&res_buf->data, test_data, sizeof(test_data));
			break;
		case IPMI_GET_DEVICE_ID_CMD:
			if(debug) printf("IPMI_GET_DEVICE_ID_CMD\n");
			res_buf->netfn_lun = 0x1c;
			res_buf->cmd = IPMI_GET_DEVICE_ID_CMD;
			res_buf->cc = 0;
			*res_len += sizeof(devid_data);
			memcpy(&res_buf->data, devid_data, sizeof(devid_data));
			break;
		case IPMI_GET_SELF_TEST_RESULTS_CMD:
			if(debug) printf("IPMI_GET_SELF_TEST_RESULTS_CMD\n");
			res_buf->netfn_lun = 0x1c;
			res_buf->cmd = IPMI_GET_SELF_TEST_RESULTS_CMD;
			res_buf->cc = 0;
			*res_len += sizeof(self_test_data);
			memcpy(&res_buf->data, self_test_data, sizeof(self_test_data));
			break;
		case IPMI_GET_DEVICE_GUID_CMD:
			if(debug) printf("IPMI_GET_DEVICE_GUID_CMD\n");
			res_buf->netfn_lun = 0x1c;
			res_buf->cmd = IPMI_GET_DEVICE_GUID_CMD;
			res_buf->cc = 0;
			*res_len += sizeof(guid_data);
			memcpy(&res_buf->data, guid_data, sizeof(guid_data));
			break;
		case IPMI_GET_BMC_GLOBAL_ENABLES_CMD:
			if(debug) printf("IPMI_GET_BMC_GLOBAL_ENABLES_CMD\n");
			res_buf->netfn_lun = 0x1c;
			res_buf->cmd = IPMI_GET_BMC_GLOBAL_ENABLES_CMD;
			res_buf->cc = 0;
			*res_len += 1;
			res_buf->data[0] = 0x00;
			break;
		case IPMI_CLEAR_MSG_FLAGS_CMD:
			if(debug) printf("IPMI_CLEAR_MSG_FLAGS_CMD\n");
			res_buf->netfn_lun = 0x1c;
			res_buf->cmd = IPMI_CLEAR_MSG_FLAGS_CMD;
			res_buf->cc = 0;
			*res_len += 1;
			res_buf->data[0] = 0x00;
			break;
		default:
			printf("app unsupport cmd %x \n", req_buf->cmd);
			res_buf->netfn_lun = 0x1c;
			res_buf->cmd = req_buf->cmd;
			res_buf->cc = 0xC1;
			break;
	}
	return ret;
}

unsigned char req_buf[256];
unsigned char res_buf[256];

void *kcs_thread(void *data) {

	ipmi_req_t *req = (ipmi_req_t *) req_buf;
	ipmi_res_t *res = (ipmi_res_t *) res_buf;

	unsigned char req_len, res_len;
	int i = 0;
	int kcs_fd = (int) data;
	while(1) {
get_req:
		req_len = read(kcs_fd, req_buf, sizeof(req_buf));
		if (req_len > 0) {
			//dump read data
			if(debug) {
				printf("Req [%d] : ", req_len);
				for(i=0;i<req_len;i++) {
					printf("%x ", req_buf[i]);
				}
				printf("\n");
			}
			res_len = 3;
			switch(req->netfn_lun >> 2) {
				case IPMI_APP_NETFN:
					ipmi_app_handler(req, &req_len, res, &res_len);
					break;
				default:
					printf("other unsupport cmd %x \n", req->cmd);
					res->netfn_lun = ((req->netfn_lun >> 2) + 1) << 2;
					res->cmd = req->cmd;
					res->cc = 0xC1;
					break;
			}
		} else {
			continue;
		}

		res_len = write(kcs_fd, res_buf, res_len);
	}


}

int main(int argc, char *argv[])
{
	int kcs_fd;
	int i;
	long retVal;

	pthread_t thread;

	char option;
	char dev_node[100]="", kcs_addr[100]="";
	unsigned long node_no = 2;
	unsigned long addr = 0xca2;


	while((option=getopt_long(argc, argv, short_options, long_options, NULL))!=(char)-1){
//		printf("option is c %c\n", option);
		switch(option){
			case 'h':
				usage(stdout, argc, argv);
				exit(EXIT_SUCCESS);
				break;
			case 'n':
				node_no = strtoul(optarg, 0, 0);
				break;
			case 'a':
				strcpy(kcs_addr, optarg);
				if(!strcmp(kcs_addr, "")){
					printf("No input kcs addr\n");
					usage(stdout, argc, argv);
					exit(EXIT_FAILURE);
				}
				break;
			case 'd':
				debug = 1;
//				printf("debug is %d\n",debug);
				break;
			default:
				usage(stdout, argc, argv);
				exit(EXIT_FAILURE);
		}
	}

	if(debug) printf("kcs node: %d \n", node_no);
	switch(node_no) {
		case 0:
			system("echo 1 > /sys/devices/platform/ast-kcs.0/enable");
			break;
		case 1:
			system("echo 1 > /sys/devices/platform/ast-kcs.1/enable");
			break;
		case 2:
			system("echo 1 > /sys/devices/platform/ast-kcs.2/enable");
			break;
		case 3:
			system("echo 1 > /sys/devices/platform/ast-kcs.3/enable");
			break;
		case 4:
			system("echo 1 > /sys/devices/platform/ast-kcs.4/enable");
			break;
	}

	sprintf(dev_node, "/dev/ast-kcs.%d", node_no);
	printf("ast-kcs.%d, addr : %x\n", node_no, addr);
	if(!strcmp(dev_node, "")){
		printf("No input device node!\n");
		usage(stdout, argc, argv);
		exit(EXIT_FAILURE);
	}

	kcs_fd = open(dev_node, O_RDWR);

	sleep(1);

	pthread_create(&thread, NULL, kcs_thread, kcs_fd);

	sleep(1);
	pthread_join(thread, NULL);

	printf("Done.\n");	/* User press Ctrl + C */

	close(kcs_fd);
	return 0;
}

