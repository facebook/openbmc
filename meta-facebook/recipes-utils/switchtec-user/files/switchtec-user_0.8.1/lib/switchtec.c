/*
 * Microsemi Switchtec(tm) PCIe Management Library
 * Copyright (c) 2017, Microsemi Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "switchtec_priv.h"

#include "switchtec/switchtec.h"
#include "switchtec/mrpc.h"
#include "switchtec/errors.h"
#include "switchtec/log.h"
#include "switchtec/gas.h"

#include <linux/switchtec_ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <endian.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/sysmacros.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

static const char *sys_path = "/sys/class/switchtec";

static int dev_to_sysfs_path(struct switchtec_dev *dev, const char *suffix,
			     char *buf, size_t buflen)
{
	int ret;
	struct stat stat;

	ret = fstat(dev->fd, &stat);
	if (ret < 0)
		return ret;

	snprintf(buf, buflen,
		 "/sys/dev/char/%d:%d/%s",
		 major(stat.st_rdev), minor(stat.st_rdev), suffix);

	return 0;
}

static int sysfs_read_str(const char *path, char *buf, size_t buflen)
{
	int ret;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	ret = read(fd, buf, buflen);

	close(fd);

	return ret;
}

static int sysfs_read_int(const char *path, int base)
{
	int ret;
	char buf[64];

	ret = sysfs_read_str(path, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	return strtol(buf, NULL, base);
}

static int check_switchtec_device(struct switchtec_dev *dev)
{
	int ret;
	char syspath[PATH_MAX];
	char *p;

	ret = 0;
	p = strstr(dev->name, "/dev/i2c-");
	if (p) {
		char delims[] = ":";
		char *result = NULL;
		result = strtok(dev->name, delims);
		while(result != NULL)
			result = strtok( NULL, delims );

		dev->gas_chan = SWITCHTEC_GAS_CHAN_TWI;
		ret = dev_to_sysfs_path(dev, "device", syspath,
				sizeof(syspath));
		if (ret)
			return ret;

		ret = access(syspath, F_OK);
		if (ret)
			errno = ENOTTY;

		return ret;
	}

	p = strstr(dev->name, "/dev/switchtec");
	if (p) {
		dev->gas_chan = SWITCHTEC_GAS_CHAN_INBAND;
		ret = dev_to_sysfs_path(dev, "device/switchtec", syspath,
				sizeof(syspath));
		if (ret)
			return ret;

		ret = access(syspath, F_OK);
		if (ret)
			errno = ENOTTY;

		return ret;
	}

	errno = ENOTTY;
	return -1;
}

static int get_partition(struct switchtec_dev *dev)
{
	int ret;
	uint32_t offset;

	offset = SWITCHTEC_GAS_SYS_INFO_OFFSET +
		offsetof(struct sys_info_regs, partition_id);

	ret = switchtec_gas_read(dev,
			(uint8_t *)&(dev->partition), offset, 4);

	if (ret)
		return ret;

	return 0;
}

int switchtec_dev_prepare(struct switchtec_dev *dev, const char *path, char *real_path)
{
	char *slave;
	uint8_t twi_slave;
	int ret;
	char *p;

	p = strstr(path, "/dev/i2c-");
	if (p) {
		char delims[] = ":";
		p = strtok((char *)path, delims);
		if (p != NULL) {
			slave = strtok(NULL,delims);
			if (slave == NULL) {
				errno = EINVAL;
				return -1;
			}
			ret = sscanf(slave, "%hhx", &twi_slave);
			if ((ret == 0) || (ret == EOF)) {
				errno = EINVAL;
				return -1;
			}
		} else {
			errno = EINVAL;
			return -1;
		}

		dev->gas_chan = SWITCHTEC_GAS_CHAN_TWI;
		dev->twi_slave = twi_slave;
		strcpy(real_path, p);
	}

	p = strstr(path, "/dev/switchtec");
	if (p) {
		dev->gas_chan = SWITCHTEC_GAS_CHAN_INBAND;
		strcpy(real_path, path);
	}

	return 0;
}

struct switchtec_dev *switchtec_open(const char * path)
{
	struct switchtec_dev *dev;
	char real_path[64];
	int ret;

	dev = malloc(sizeof(*dev));
	if (dev == NULL)
		return dev;

	ret = switchtec_dev_prepare(dev, path, real_path);
	if (ret)
		goto err_free;

	dev->fd = open(real_path, O_RDWR | O_CLOEXEC);
	if (dev->fd < 0)
		goto err_free;

	snprintf(dev->name, sizeof(dev->name), "%s", path);

	if (check_switchtec_device(dev))
		goto err_close_free;

	if (get_partition(dev))
		goto err_close_free;

	return dev;

err_close_free:
	close(dev->fd);
err_free:
	free(dev);
	return NULL;
}

void switchtec_close(struct switchtec_dev *dev)
{
	if (dev == NULL)
		return;

	close(dev->fd);
	free(dev);
}

__attribute__ ((pure))
const char *switchtec_name(struct switchtec_dev *dev)
{
	return dev->name;
}

__attribute__ ((pure))
int switchtec_fd(struct switchtec_dev *dev)
{
	return dev->fd;
}

__attribute__ ((pure))
int switchtec_partition(struct switchtec_dev *dev)
{
	return dev->partition;
}

static int scan_dev_filter(const struct dirent *d)
{
	if (d->d_name[0] == '.')
		return 0;

	return 1;
}

static void get_device_str(const char *path, const char *file,
			   char *buf, size_t buflen)
{
	char sysfs_path[PATH_MAX];
	int ret;

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",
		 path, file);

	ret = sysfs_read_str(sysfs_path, buf, buflen);
	if (ret < 0)
		snprintf(buf, buflen, "unknown");

	buf[strcspn(buf, "\n")] = 0;
}

static void get_fw_version(const char *path, char *buf, size_t buflen)
{
	char sysfs_path[PATH_MAX];
	int fw_ver;

	snprintf(sysfs_path, sizeof(sysfs_path), "%s/fw_version",
		 path);

	fw_ver = sysfs_read_int(sysfs_path, 16);

	if (fw_ver < 0)
		snprintf(buf, buflen, "unknown");
	else
		version_to_string(fw_ver, buf, buflen);
}

int switchtec_list(struct switchtec_device_info **devlist)
{
	struct dirent **devices;
	int i, n;
	char link_path[PATH_MAX];
	char pci_path[PATH_MAX];
	struct switchtec_device_info *dl;

	n = scandir(sys_path, &devices, scan_dev_filter, alphasort);
	if (n <= 0)
		return n;

	dl = *devlist = calloc(n, sizeof(struct switchtec_device_info));

	if (dl == NULL) {
		for (i = 0; i < n; i++)
			free(devices[n]);
		free(devices);
		errno=ENOMEM;
		return -errno;
	}

	for (i = 0; i < n; i++) {
		snprintf(dl[i].name, sizeof(dl[i].name),
			 "%s", devices[i]->d_name);
		snprintf(dl[i].path, sizeof(dl[i].path),
			 "/dev/%s", devices[i]->d_name);

		snprintf(link_path, sizeof(link_path), "%s/%s/device",
			 sys_path, devices[i]->d_name);

		if (readlink(link_path, pci_path, sizeof(pci_path)) > 0)
			snprintf(dl[i].pci_dev, sizeof(dl[i].pci_dev),
				 "%s", basename(pci_path));
		else
			snprintf(dl[i].pci_dev, sizeof(dl[i].pci_dev),
				 "unknown pci device");


		snprintf(link_path, sizeof(link_path), "%s/%s",
			 sys_path, devices[i]->d_name);

		get_device_str(link_path, "product_id", dl[i].product_id,
			       sizeof(dl[i].product_id));
		get_device_str(link_path, "product_revision",
			       dl[i].product_rev, sizeof(dl[i].product_rev));
		get_fw_version(link_path, dl[i].fw_version,
			       sizeof(dl[i].fw_version));

		free(devices[n]);
	}

	free(devices);
	return n;
}

int switchtec_get_fw_version(struct switchtec_dev *dev, char *buf,
			     size_t buflen)
{
	int ret;
	uint32_t version;
	uint32_t offset;

	offset = SWITCHTEC_GAS_SYS_INFO_OFFSET +
		offsetof(struct sys_info_regs, firmware_version);

	ret = switchtec_gas_read(dev,
			(uint8_t *)&(version), offset, 4);

	if (ret)
		return ret;

	version_to_string(version, buf, buflen);

	return 0;
}

int switchtec_submit_cmd_inband(struct switchtec_dev *dev, uint32_t cmd,
			 const void *payload, size_t payload_len)
{
	int ret;
	char buf[payload_len + sizeof(cmd)];

	cmd = htole32(cmd);
	memcpy(buf, &cmd, sizeof(cmd));
	memcpy(&buf[sizeof(cmd)], payload, payload_len);

	ret = write(dev->fd, buf, sizeof(buf));

	if (ret < 0)
		return ret;

	if (ret != sizeof(buf)) {
		errno = EIO;
		return -errno;
	}

	return 0;
}

int switchtec_read_resp_inband(struct switchtec_dev *dev, void *resp,
			size_t resp_len)
{
	int32_t ret;
	char buf[sizeof(uint32_t) + resp_len];

	ret = read(dev->fd, buf, sizeof(buf));

	if (ret < 0)
		return ret;

	if (ret != sizeof(buf)) {
		errno = EIO;
		return -errno;
	}

	memcpy(&ret, buf, sizeof(ret));
	if (ret)
		errno = ret;

	if (!resp)
		return ret;

	memcpy(resp, &buf[sizeof(ret)], resp_len);

	return ret;
}

int switchtec_submit_cmd_twi(struct switchtec_dev *dev, uint32_t cmd,
		const void *payload, size_t payload_len)
{
	uint32_t offset;
	int ret;

	offset = SWITCHTC_GAS_MRPC_INPUT_DATA_OFFSET;
	ret = switchtec_twi_gas_write(dev, offset, payload, payload_len);
	if (ret) {
		return ret;
	}

	offset = SWITCHTC_GAS_MRPC_COMMAND_OFFSET;
	ret = switchtec_twi_gas_write(dev, offset, &cmd, sizeof(cmd));
	if (ret) {
		return ret;
	}

	return 0;
}

int switchtec_read_resp_twi(struct switchtec_dev *dev, void *resp,
		size_t resp_len)
{
	int max_retry = 1000;
	int i = 0;
	int ret;
	uint32_t offset;
	uint32_t cmd_status;
	uint32_t cmd_ret;

	while (i++ < max_retry) {
		offset = SWITCHTC_GAS_MRPC_STATUS_OFFSET;
		ret = switchtec_twi_gas_read(dev, offset,
				&cmd_status, sizeof(cmd_status));
		if (ret)
			return ret;

		if (cmd_status == MRPC_BG_STAT_DONE)
			break;

		if (cmd_status == MRPC_BG_STAT_ERROR)
			return cmd_status;

		usleep(10000);
	}

	offset = SWITCHTC_GAS_MRPC_COMMAND_RETURN_VALUE_OFFSET;
	ret = switchtec_twi_gas_read(dev, offset,
			&cmd_ret, sizeof(cmd_ret));
	if (ret)
		return ret;

	if (cmd_ret)
		return cmd_ret;

	offset = SWITCHTC_GAS_MRPC_OUTPUT_DATA_OFFSET;
	ret = switchtec_twi_gas_read(dev, offset, resp, resp_len);
	if (ret)
		return ret;

	return 0;
}

int switchtec_submit_cmd(struct switchtec_dev *dev, uint32_t cmd,
		const void *payload, size_t payload_len)
{
	if (SWITCHTEC_GAS_CHAN_INBAND == dev->gas_chan)
		return switchtec_submit_cmd_inband(dev, cmd,
				payload, payload_len);
	else if (SWITCHTEC_GAS_CHAN_TWI == dev->gas_chan)
		return switchtec_submit_cmd_twi(dev, cmd,
				payload, payload_len);

	return -1;
}


int switchtec_read_resp(struct switchtec_dev *dev, void *resp,
		size_t resp_len)
{
	if (SWITCHTEC_GAS_CHAN_INBAND == dev->gas_chan)
		return switchtec_read_resp_inband(dev, resp, resp_len);
	if (SWITCHTEC_GAS_CHAN_TWI == dev->gas_chan)
		return switchtec_read_resp_twi(dev, resp, resp_len);

	return -1;
}

int switchtec_cmd(struct switchtec_dev *dev,  uint32_t cmd,
		  const void *payload, size_t payload_len, void *resp,
		  size_t resp_len)
{
	int ret;

retry:
	ret = switchtec_submit_cmd(dev, cmd, payload, payload_len);
	if (errno == EBADE) {
		switchtec_read_resp(dev, NULL, 0);
		errno = 0;
		goto retry;
	}

	if (ret < 0)
		return ret;

	return switchtec_read_resp(dev, resp, resp_len);
}

int switchtec_echo(struct switchtec_dev *dev, uint32_t input,
		   uint32_t *output)
{
	return switchtec_cmd(dev, MRPC_ECHO, &input, sizeof(input),
			     output, sizeof(output));
}

int switchtec_hard_reset(struct switchtec_dev *dev)
{
	uint32_t subcmd = 0;

	return switchtec_cmd(dev, MRPC_RESET, &subcmd, sizeof(subcmd),
			     NULL, 0);
}

static const char *ltssm_str(int ltssm, int show_minor)
{
	if (!show_minor)
		ltssm |= 0xFF00;

	switch(ltssm) {
	case 0x0000: return "Detect (INACTIVE)";
	case 0x0100: return "Detect (QUIET)";
	case 0x0200: return "Detect (SPD_CHD0)";
	case 0x0300: return "Detect (SPD_CHD1)";
	case 0x0400: return "Detect (ACTIVE0)";
	case 0x0500: return "Detect (ACTIVE1)";
	case 0x0600: return "Detect (P1_TO_P0)";
	case 0x0700: return "Detect (P0_TO_P1_0)";
	case 0x0800: return "Detect (P0_TO_P1_1)";
	case 0x0900: return "Detect (P0_TO_P1_2)";
	case 0xFF00: return "Detect";
	case 0x0001: return "Polling (INACTIVE)";
	case 0x0101: return "Polling (ACTIVE_ENTRY)";
	case 0x0201: return "Polling (ACTIVE)";
	case 0x0301: return "Polling (CFG)";
	case 0x0401: return "Polling (COMP)";
	case 0x0501: return "Polling (COMP_ENTRY)";
	case 0x0601: return "Polling (COMP_EIOS)";
	case 0x0701: return "Polling (COMP_EIOS_ACK)";
	case 0x0801: return "Polling (COMP_IDLE)";
	case 0xFF01: return "Polling";
	case 0x0002: return "Config (INACTIVE)";
	case 0x0102: return "Config (US_LW_START)";
	case 0x0202: return "Config (US_LW_ACCEPT)";
	case 0x0302: return "Config (US_LN_WAIT)";
	case 0x0402: return "Config (US_LN_ACCEPT)";
	case 0x0502: return "Config (DS_LW_START)";
	case 0x0602: return "Config (DS_LW_ACCEPT)";
	case 0x0702: return "Config (DS_LN_WAIT)";
	case 0x0802: return "Config (DS_LN_ACCEPT)";
	case 0x0902: return "Config (COMPLETE)";
	case 0x0A02: return "Config (IDLE)";
	case 0xFF02: return "Config";
	case 0x0003: return "L0 (INACTIVE)";
	case 0x0103: return "L0 (L0)";
	case 0x0203: return "L0 (TX_EL_IDLE)";
	case 0x0303: return "L0 (TX_IDLE_MIN)";
	case 0xFF03: return "L0";
	case 0x0004: return "Recovery (INACTIVE)";
	case 0x0104: return "Recovery (RCVR_LOCK)";
	case 0x0204: return "Recovery (RCVR_CFG)";
	case 0x0304: return "Recovery (IDLE)";
	case 0x0404: return "Recovery (SPEED0)";
	case 0x0504: return "Recovery (SPEED1)";
	case 0x0604: return "Recovery (SPEED2)";
	case 0x0704: return "Recovery (SPEED3)";
	case 0x0804: return "Recovery (EQ_PH0)";
	case 0x0904: return "Recovery (EQ_PH1)";
	case 0x0A04: return "Recovery (EQ_PH2)";
	case 0x0B04: return "Recovery (EQ_PH3)";
	case 0xFF04: return "Recovery";
	case 0x0005: return "Disable (INACTIVE)";
	case 0x0105: return "Disable (DISABLE0)";
	case 0x0205: return "Disable (DISABLE1)";
	case 0x0305: return "Disable (DISABLE2)";
	case 0x0405: return "Disable (DISABLE3)";
	case 0xFF05: return "Disable";
	case 0x0006: return "Loop Back (INACTIVE)";
	case 0x0106: return "Loop Back (ENTRY)";
	case 0x0206: return "Loop Back (ENTRY_EXIT)";
	case 0x0306: return "Loop Back (EIOS)";
	case 0x0406: return "Loop Back (EIOS_ACK)";
	case 0x0506: return "Loop Back (IDLE)";
	case 0x0606: return "Loop Back (ACTIVE)";
	case 0x0706: return "Loop Back (EXIT0)";
	case 0x0806: return "Loop Back (EXIT1)";
	case 0xFF06: return "Loop Back";
	case 0x0007: return "Hot Reset (INACTIVE)";
	case 0x0107: return "Hot Reset (HOT_RESET)";
	case 0x0207: return "Hot Reset (MASTER_UP)";
	case 0x0307: return "Hot Reset (MASTER_DOWN)";
	case 0xFF07: return "Hot Reset";
	case 0x0008: return "TxL0s (INACTIVE)";
	case 0x0108: return "TxL0s (IDLE)";
	case 0x0208: return "TxL0s (T0_L0)";
	case 0x0308: return "TxL0s (FTS0)";
	case 0x0408: return "TxL0s (FTS1)";
	case 0xFF08: return "TxL0s";
	case 0x0009: return "L1 (INACTIVE)";
	case 0x0109: return "L1 (IDLE)";
	case 0x0209: return "L1 (SUBSTATE)";
	case 0x0309: return "L1 (SPD_CHG1)";
	case 0x0409: return "L1 (T0_L0)";
	case 0xFF09: return "L1";
	case 0x000A: return "L2 (INACTIVE)";
	case 0x010A: return "L2 (IDLE)";
	case 0x020A: return "L2 (TX_WAKE0)";
	case 0x030A: return "L2 (TX_WAKE1)";
	case 0x040A: return "L2 (EXIT)";
	case 0x050A: return "L2 (SPEED)";
	case 0xFF0A: return "L2";
	default: return "UNKNOWN";
	}

}

static int compare_port_id(const void *aa, const void *bb)
{
	const struct switchtec_port_id *a = aa, *b = bb;

	if (a->partition != b->partition)
		return a->partition - b->partition;
	if (a->upstream != b->upstream)
		return b->upstream - a->upstream;
	return a->log_id - b->log_id;
}

static int compare_status(const void *aa, const void *bb)
{
	const struct switchtec_status *a = aa, *b = bb;

	return compare_port_id(&a->port, &b->port);
}

int switchtec_status(struct switchtec_dev *dev,
		     struct switchtec_status **status)
{
	uint64_t port_bitmap = 0;
	int ret;
	int i, p;
	int nr_ports = 0;
	struct switchtec_status *s;

	if (!status) {
		errno = EINVAL;
		return -errno;
	}

	struct {
		uint8_t phys_port_id;
		uint8_t par_id;
		uint8_t log_port_id;
		uint8_t stk_id;
		uint8_t cfg_lnk_width;
		uint8_t neg_lnk_width;
		uint8_t usp_flag;
		uint8_t linkup_linkrate;
		uint16_t LTSSM;
		uint16_t reserved;
	} ports[SWITCHTEC_MAX_PORTS];

	ret = switchtec_cmd(dev, MRPC_LNKSTAT, &port_bitmap, sizeof(port_bitmap),
			    ports, sizeof(ports));
	if (ret)
		return ret;


	for (i = 0; i < SWITCHTEC_MAX_PORTS; i++) {
		if ((ports[i].stk_id >> 4) > SWITCHTEC_MAX_STACKS)
			continue;
		nr_ports++;
	}

	s = *status = calloc(nr_ports, sizeof(*s));
	if (!s)
		return -ENOMEM;

	for (i = 0, p = 0; i < SWITCHTEC_MAX_PORTS && p < nr_ports; i++) {
		if ((ports[i].stk_id >> 4) > SWITCHTEC_MAX_STACKS)
			continue;

		s[p].port.partition = ports[i].par_id;
		s[p].port.stack = ports[i].stk_id >> 4;
		s[p].port.upstream = ports[i].usp_flag;
		s[p].port.stk_id = ports[i].stk_id & 0xF;
		s[p].port.phys_id = ports[i].phys_port_id;
		s[p].port.log_id = ports[i].log_port_id;

		s[p].cfg_lnk_width = ports[i].cfg_lnk_width;
		s[p].neg_lnk_width = ports[i].neg_lnk_width;
		s[p].link_up = ports[i].linkup_linkrate >> 7;
		s[p].link_rate = ports[i].linkup_linkrate & 0x7F;
		s[p].ltssm = le16toh(ports[i].LTSSM);
		s[p].ltssm_str = ltssm_str(s[i].ltssm, 0);
		p++;
	}

	qsort(s, nr_ports, sizeof(*s), compare_status);

	return nr_ports;
}

static int get_class_devices(const char *searchpath,
			     struct switchtec_status *status)
{
	int i;
	ssize_t len;
	char syspath[PATH_MAX];
	glob_t paths;
	int found = 0;
	const size_t MAX_LEN = 256;

	snprintf(syspath, sizeof(syspath), "%s*/*/device", searchpath);
	glob(syspath, 0, NULL, &paths);

	for (i = 0; i < paths.gl_pathc; i++) {
		char *p = paths.gl_pathv[i];

		len = readlink(p, syspath, sizeof(syspath));
		if (len <= 0)
			continue;

		p = dirname(p);

		if (!status->class_devices) {
			status->class_devices = calloc(MAX_LEN, 1);
			strcpy(status->class_devices, basename(p));
		} else {
			len = strlen(status->class_devices);
			snprintf(&status->class_devices[len], MAX_LEN - len,
				 ", %s", basename(p));
		}

		found = 1;
	}

	globfree(&paths);
	return found;
}

static void get_port_info(const char *searchpath, int port,
			  struct switchtec_status *status)
{
	int i;
	char syspath[PATH_MAX];
	glob_t paths;

	snprintf(syspath, sizeof(syspath), "%s/*:*:%02d.*/*:*:*/",
		 searchpath, port);

	glob(syspath, 0, NULL, &paths);

	for (i = 0; i < paths.gl_pathc; i++) {
		char *p = paths.gl_pathv[i];

		snprintf(syspath, sizeof(syspath), "%s/vendor", p);
		status->vendor_id = sysfs_read_int(syspath, 16);
		if (status->vendor_id < 0)
			continue;

		snprintf(syspath, sizeof(syspath), "%s/device", p);
		status->device_id = sysfs_read_int(syspath, 16);
		if (status->device_id < 0)
			continue;

		if (get_class_devices(p, status)) {
			if (status->pci_dev)
				free(status->pci_dev);
			status->pci_dev = strdup(basename(p));
		}

		if (!status->pci_dev)
			status->pci_dev = strdup(basename(p));
	}

	globfree(&paths);
}

int switchtec_get_devices(struct switchtec_dev *dev,
			  struct switchtec_status *status,
			  int ports)
{
	int ret;
	int i;
	int local_part;
	int port;
	char syspath[PATH_MAX];
	char searchpath[PATH_MAX];

	ret = dev_to_sysfs_path(dev, "device", syspath,
				sizeof(syspath));
	if (ret)
		return ret;

	if (!realpath(syspath, searchpath)) {
		errno = ENXIO;
		return -errno;
	}

	//Replace eg "0000:03:00.1" into "0000:03:00.0"
	searchpath[strlen(searchpath) - 1] = '0';

	local_part = switchtec_partition(dev);
	port = 0;

	for (i = 0; i < ports; i++) {
		if (status[i].port.upstream ||
		    status[i].port.partition != local_part)
			continue;

		get_port_info(searchpath, port, &status[i]);
		port++;
	}

	return 0;
}

void switchtec_status_free(struct switchtec_status *status, int ports)
{
	int i;

	for (i = 0; i < ports; i++) {
		if (status[i].pci_dev)
			free(status[i].pci_dev);

		if (status[i].class_devices)
			free(status[i].class_devices);
	}

	free(status);
}

void switchtec_perror(const char *str)
{
	const char *msg;

	switch (errno) {

	case ERR_NO_AVAIL_MRPC_THREAD:
		msg = "No available MRPC handler thread"; break;
	case ERR_HANDLER_THREAD_NOT_IDLE:
		msg = "The handler thread is not idle"; break;
	case ERR_NO_BG_THREAD:
		msg = "No background thread run for the command"; break;

	case ERR_SUBCMD_INVALID: 	msg = "Invalid subcommand"; break;
	case ERR_CMD_INVALID: 		msg = "Invalid command"; break;
	case ERR_PARAM_INVALID:		msg = "Invalid parameter"; break;
	case ERR_BAD_FW_STATE:		msg = "Bad firmware state"; break;
	case ERR_STACK_INVALID: 	msg = "Invalid Stack"; break;
	case ERR_PORT_INVALID: 		msg = "Invalid Port"; break;
	case ERR_EVENT_INVALID: 	msg = "Invalid Event"; break;
	case ERR_RST_RULE_FAILED: 	msg = "Reset rule search failed"; break;
	case ERR_ACCESS_REFUSED: 	msg = "Access Refused"; break;

	default:
		perror(str);
		return;
	}

	fprintf(stderr, "%s: %s\n", str, msg);
}

int switchtec_pff_to_port(struct switchtec_dev *dev, int pff,
			  int *partition, int *port)
{
	int ret;
	struct switchtec_ioctl_pff_port p;

	p.pff = pff;
	ret = ioctl(dev->fd, SWITCHTEC_IOCTL_PFF_TO_PORT, &p);
	if (ret)
		return ret;

	if (partition)
		*partition = p.partition;
	if (port)
		*port = p.port;

	return 0;
}

int switchtec_port_to_pff(struct switchtec_dev *dev, int partition,
			  int port, int *pff)
{
	int ret;
	struct switchtec_ioctl_pff_port p;

	p.port = port;
	p.partition = partition;

	ret = ioctl(dev->fd, SWITCHTEC_IOCTL_PORT_TO_PFF, &p);
	if (ret)
		return ret;

	if (pff)
		*pff = p.pff;

	return 0;
}

static int log_a_to_file(struct switchtec_dev *dev, int sub_cmd_id, int fd)
{
	int ret;
	int read = 0;
	struct log_a_retr_result res;
	struct log_a_retr cmd = {
		.sub_cmd_id = sub_cmd_id,
		.start = -1,
	};

	res.hdr.remain = 1;

	while (res.hdr.remain) {
		ret = switchtec_cmd(dev, MRPC_FWLOGRD, &cmd, sizeof(cmd),
				    &res, sizeof(res));
		if (ret)
			return -1;

		ret = write(fd, res.data, sizeof(*res.data) * res.hdr.count);
		if (ret < 0)
			return ret;

		read += le32toh(res.hdr.count);
		cmd.start = res.hdr.next_start;
	}

	return read;
}

static int log_b_to_file(struct switchtec_dev *dev, int sub_cmd_id, int fd)
{
	int ret;
	int read = 0;
	struct log_b_retr_result res;
	struct log_b_retr cmd = {
		.sub_cmd_id = sub_cmd_id,
		.offset = 0,
		.length = htole32(sizeof(res.data)),
	};

	res.hdr.remain = sizeof(res.data);

	while (res.hdr.remain) {
		ret = switchtec_cmd(dev, MRPC_FWLOGRD, &cmd, sizeof(cmd),
				    &res, sizeof(res));
		if (ret)
			return -1;

		ret = write(fd, res.data, res.hdr.length);
		if (ret < 0)
			return ret;

		read += le32toh(res.hdr.length);
		cmd.offset = htole32(read);
	}

	return read;
}

int switchtec_log_to_file(struct switchtec_dev *dev,
			  enum switchtec_log_type type,
			  int fd)
{
	switch (type) {
	case SWITCHTEC_LOG_RAM:
		return log_a_to_file(dev, MRPC_FWLOGRD_RAM, fd);
	case SWITCHTEC_LOG_FLASH:
		return log_a_to_file(dev, MRPC_FWLOGRD_FLASH, fd);
	case SWITCHTEC_LOG_MEMLOG:
		return log_b_to_file(dev, MRPC_FWLOGRD_MEMLOG, fd);
	case SWITCHTEC_LOG_REGS:
		return log_b_to_file(dev, MRPC_FWLOGRD_REGS, fd);
	case SWITCHTEC_LOG_THRD_STACK:
		return log_b_to_file(dev, MRPC_FWLOGRD_THRD_STACK, fd);
	case SWITCHTEC_LOG_SYS_STACK:
		return log_b_to_file(dev, MRPC_FWLOGRD_SYS_STACK, fd);
	case SWITCHTEC_LOG_THRD:
		return log_b_to_file(dev, MRPC_FWLOGRD_THRD, fd);
	};

	errno = EINVAL;
	return -errno;
}

float switchtec_die_temp(struct switchtec_dev *dev)
{
	int ret;
	uint32_t sub_cmd_id = MRPC_DIETEMP_SET_MEAS;
	uint32_t temp;

	ret = switchtec_cmd(dev, MRPC_DIETEMP, &sub_cmd_id,
			    sizeof(sub_cmd_id), NULL, 0);
	if (ret)
		return -1.0;

	sub_cmd_id = MRPC_DIETEMP_GET;
	ret = switchtec_cmd(dev, MRPC_DIETEMP, &sub_cmd_id,
			    sizeof(sub_cmd_id), &temp, sizeof(temp));
	if (ret)
		return -1.0;

	return temp / 100.;
}

/*
 * GAS map maps the hardware registers into user memory space.
 * Needless to say, this can be very dangerous and should only
 * be done if you know what you are doing. Any register accesses
 * that use this will remain unsupported by Microsemi unless it's
 * done within the switchtec user project or otherwise specified.
 */
void *switchtec_gas_map(struct switchtec_dev *dev, int writeable,
			size_t *map_size)
{
	int ret;
	int fd;
	void *map;
	char respath[PATH_MAX];
	struct stat stat;

	ret = dev_to_sysfs_path(dev, "device/resource0_wc", respath,
				sizeof(respath));

	if (ret) {
		errno = ret;
		return MAP_FAILED;
	}

	fd = open(respath, writeable ? O_RDWR : O_RDONLY);
	if (fd < 0)
		return MAP_FAILED;

	ret = fstat(fd, &stat);
	if (ret < 0)
		return MAP_FAILED;

	map = mmap(NULL, stat.st_size,
		   (writeable ? PROT_WRITE : 0) | PROT_READ,
		   MAP_SHARED, fd, 0);
	close(fd);

	if (map_size)
		*map_size = stat.st_size;

	dev->gas_map = map;
	dev->gas_map_size = stat.st_size;

	return map;
}

void switchtec_gas_unmap(struct switchtec_dev *dev, void *map)
{
	munmap(map, dev->gas_map_size);
}

int switchtec_gas_read(struct switchtec_dev *dev, uint8_t *data,
		uint32_t offset, uint32_t size)
{
	if (SWITCHTEC_GAS_CHAN_TWI == dev->gas_chan)
		return switchtec_twi_gas_read(dev, offset, data, size);
	else if (SWITCHTEC_GAS_CHAN_INBAND == dev->gas_chan)
		return switchtec_inband_gas_read(dev, offset, data, size);

	return -1;
}

int switchtec_gas_write(struct switchtec_dev *dev, uint8_t *data,
		uint32_t offset, uint32_t size)
{
	if (SWITCHTEC_GAS_CHAN_TWI == dev->gas_chan)
		return switchtec_twi_gas_write(dev, offset, data, size);
	else if (SWITCHTEC_GAS_CHAN_INBAND == dev->gas_chan)
		return switchtec_inband_gas_write(dev, offset, data, size);

	return -1;
}
