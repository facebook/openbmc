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

#ifndef LIBSWITCHTEC_SWITCHTEC_H
#define LIBSWITCHTEC_SWITCHTEC_H

#include "mrpc.h"

#include <linux/limits.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct switchtec_dev;

#define SWITCHTEC_MAX_PARTS  48
#define SWITCHTEC_MAX_PORTS  48
#define SWITCHTEC_MAX_STACKS 8
#define SWITCHTEC_MAX_EVENT_COUNTERS 64
#define SWITCHTEC_UNBOUND_PORT 255
#define SWITCHTEC_PFF_PORT_VEP 100

struct switchtec_device_info {
	char name[256];
	char pci_dev[256];
	char product_id[32];
	char product_rev[8];
	char fw_version[32];
	char path[PATH_MAX];
};

struct switchtec_port_id {
	unsigned char partition;
	unsigned char stack;
	unsigned char upstream;
	unsigned char stk_id;
	unsigned char phys_id;
	unsigned char log_id;
};

struct switchtec_status {
	struct switchtec_port_id port;
	unsigned char cfg_lnk_width;
	unsigned char neg_lnk_width;
	unsigned char link_up;
	unsigned char link_rate;
	unsigned char ltssm;
	const char *ltssm_str;
	char *pci_dev;
	int vendor_id;
	int device_id;
	char *class_devices;
};

enum switchtec_log_type {
	SWITCHTEC_LOG_RAM,
	SWITCHTEC_LOG_FLASH,
	SWITCHTEC_LOG_MEMLOG,
	SWITCHTEC_LOG_REGS,
	SWITCHTEC_LOG_THRD_STACK,
	SWITCHTEC_LOG_SYS_STACK,
	SWITCHTEC_LOG_THRD,
};

struct switchtec_dev *switchtec_open(const char *path);
void switchtec_close(struct switchtec_dev *dev);

__attribute__ ((pure))
const char *switchtec_name(struct switchtec_dev *dev);
__attribute__ ((pure)) int switchtec_fd(struct switchtec_dev *dev);
__attribute__ ((pure)) int switchtec_partition(struct switchtec_dev *dev);

int switchtec_list(struct switchtec_device_info **devlist);
int switchtec_get_fw_version(struct switchtec_dev *dev, char *buf,
			     size_t buflen);

int switchtec_submit_cmd(struct switchtec_dev *dev, uint32_t cmd,
			 const void *payload, size_t payload_len);

int switchtec_read_resp(struct switchtec_dev *dev, void *resp,
			size_t resp_len);

int switchtec_cmd(struct switchtec_dev *dev, uint32_t cmd,
		  const void *payload, size_t payload_len, void *resp,
		  size_t resp_len);

int switchtec_echo(struct switchtec_dev *dev, uint32_t input, uint32_t *output);
int switchtec_hard_reset(struct switchtec_dev *dev);
int switchtec_status(struct switchtec_dev *dev,
		     struct switchtec_status **status);
int switchtec_get_devices(struct switchtec_dev *dev,
			  struct switchtec_status *status,
			  int ports);
void switchtec_status_free(struct switchtec_status *status, int ports);

void switchtec_perror(const char *str);
int switchtec_pff_to_port(struct switchtec_dev *dev, int pff,
			  int *partition, int *port);
int switchtec_port_to_pff(struct switchtec_dev *dev, int partition,
			  int port, int *pff);
int switchtec_log_to_file(struct switchtec_dev *dev,
			  enum switchtec_log_type type,
			  int fd);
float switchtec_die_temp(struct switchtec_dev *dev);

static const float switchtec_gen_transfers[] = {0, 2.5, 5, 8, 16};
static const float switchtec_gen_datarate[] = {0, 250, 500, 985, 1969};

/*********** EVENT Handling ***********/

enum switchtec_event_id {
	SWITCHTEC_GLOBAL_EVT_STACK_ERROR,
	SWITCHTEC_GLOBAL_EVT_PPU_ERROR,
	SWITCHTEC_GLOBAL_EVT_ISP_ERROR,
	SWITCHTEC_GLOBAL_EVT_SYS_RESET,
	SWITCHTEC_GLOBAL_EVT_FW_EXC,
	SWITCHTEC_GLOBAL_EVT_FW_NMI,
	SWITCHTEC_GLOBAL_EVT_FW_NON_FATAL,
	SWITCHTEC_GLOBAL_EVT_FW_FATAL,
	SWITCHTEC_GLOBAL_EVT_TWI_MRPC_COMP,
	SWITCHTEC_GLOBAL_EVT_TWI_MRPC_COMP_ASYNC,
	SWITCHTEC_GLOBAL_EVT_CLI_MRPC_COMP,
	SWITCHTEC_GLOBAL_EVT_CLI_MRPC_COMP_ASYNC,
	SWITCHTEC_GLOBAL_EVT_GPIO_INT,
	SWITCHTEC_GLOBAL_EVT_GFMS,
	SWITCHTEC_PART_EVT_PART_RESET,
	SWITCHTEC_PART_EVT_MRPC_COMP,
	SWITCHTEC_PART_EVT_MRPC_COMP_ASYNC,
	SWITCHTEC_PART_EVT_DYN_PART_BIND_COMP,
	SWITCHTEC_PFF_EVT_AER_IN_P2P,
	SWITCHTEC_PFF_EVT_AER_IN_VEP,
	SWITCHTEC_PFF_EVT_DPC,
	SWITCHTEC_PFF_EVT_CTS,
	SWITCHTEC_PFF_EVT_HOTPLUG,
	SWITCHTEC_PFF_EVT_IER,
	SWITCHTEC_PFF_EVT_THRESH,
	SWITCHTEC_PFF_EVT_POWER_MGMT,
	SWITCHTEC_PFF_EVT_TLP_THROTTLING,
	SWITCHTEC_PFF_EVT_FORCE_SPEED,
	SWITCHTEC_PFF_EVT_CREDIT_TIMEOUT,
	SWITCHTEC_PFF_EVT_LINK_STATE,
	SWITCHTEC_MAX_EVENTS,
};

enum switchtec_event_flags {
	SWITCHTEC_EVT_FLAG_CLEAR = 1 << 0,
	SWITCHTEC_EVT_FLAG_EN_POLL = 1 << 1,
	SWITCHTEC_EVT_FLAG_EN_LOG = 1 << 2,
	SWITCHTEC_EVT_FLAG_EN_CLI = 1 << 3,
	SWITCHTEC_EVT_FLAG_EN_FATAL = 1 << 4,
	SWITCHTEC_EVT_FLAG_DIS_POLL = 1 << 5,
	SWITCHTEC_EVT_FLAG_DIS_LOG = 1 << 6,
	SWITCHTEC_EVT_FLAG_DIS_CLI = 1 << 7,
	SWITCHTEC_EVT_FLAG_DIS_FATAL = 1 << 8,
};

enum {
	SWITCHTEC_EVT_IDX_LOCAL = -1,
	SWITCHTEC_EVT_IDX_ALL = -2,
};

enum switchtec_event_type {
	SWITCHTEC_EVT_GLOBAL,
	SWITCHTEC_EVT_PART,
	SWITCHTEC_EVT_PFF,
};

struct switchtec_event_summary {
	uint64_t global;
	uint64_t part_bitmap;
	unsigned local_part;
	unsigned part[SWITCHTEC_MAX_PARTS];
	unsigned pff[SWITCHTEC_MAX_PORTS];
};

int switchtec_event_wait(struct switchtec_dev *dev, int timeout_ms);
int switchtec_event_summary_set(struct switchtec_event_summary *sum,
				enum switchtec_event_id e,
				int index);
int switchtec_event_summary_test(struct switchtec_event_summary *sum,
				 enum switchtec_event_id e,
				 int index);
int switchtec_event_summary_iter(struct switchtec_event_summary *sum,
				 enum switchtec_event_id *e,
				 int *idx);
enum switchtec_event_type switchtec_event_info(enum switchtec_event_id e,
					       const char **name,
					       const char **desc);
int switchtec_event_summary(struct switchtec_dev *dev,
			    struct switchtec_event_summary *sum);
int switchtec_event_check(struct switchtec_dev *dev,
			  struct switchtec_event_summary *check,
			  struct switchtec_event_summary *res);
int switchtec_event_wait_for(struct switchtec_dev *dev,
			     enum switchtec_event_id e, int index,
			     struct switchtec_event_summary *res,
			     int timeout_ms);
int switchtec_event_ctl(struct switchtec_dev *dev,
			enum switchtec_event_id e,
			int index, int flags,
			uint32_t data[5]);

/******** FIRMWARE Management ********/

enum switchtec_fw_dlstatus {
	SWITCHTEC_DLSTAT_READY = 0,
	SWITCHTEC_DLSTAT_INPROGRESS = 1,
	SWITCHTEC_DLSTAT_HEADER_INCORRECT = 2,
	SWITCHTEC_DLSTAT_OFFSET_INCORRECT = 3,
	SWITCHTEC_DLSTAT_CRC_INCORRECT = 4,
	SWITCHTEC_DLSTAT_LENGTH_INCORRECT = 5,
	SWITCHTEC_DLSTAT_HARDWARE_ERR = 6,
	SWITCHTEC_DLSTAT_COMPLETES = 7,
	SWITCHTEC_DLSTAT_SUCCESS_FIRM_ACT = 8,
	SWITCHTEC_DLSTAT_SUCCESS_DATA_ACT = 9,
};

enum switchtec_fw_image_type {
	SWITCHTEC_FW_TYPE_BOOT = 0x0,
	SWITCHTEC_FW_TYPE_MAP0 = 0x1,
	SWITCHTEC_FW_TYPE_MAP1 = 0x2,
	SWITCHTEC_FW_TYPE_IMG0 = 0x3,
	SWITCHTEC_FW_TYPE_DAT0 = 0x4,
	SWITCHTEC_FW_TYPE_DAT1 = 0x5,
	SWITCHTEC_FW_TYPE_NVLOG = 0x6,
	SWITCHTEC_FW_TYPE_IMG1 = 0x7,
};

enum switchtec_fw_ro {
	SWITCHTEC_FW_RW = 0,
	SWITCHTEC_FW_RO = 1,
};

enum switchtec_fw_active {
	SWITCHTEC_FW_PART_ACTIVE = 1,
	SWITCHTEC_FW_PART_RUNNING = 2,
};

struct switchtec_fw_image_info {
	enum switchtec_fw_image_type type;
	char version[32];
	size_t image_addr;
	size_t image_len;
	unsigned long crc;
	int active;
	int running;
};

static inline int switchtec_fw_active(struct switchtec_fw_image_info *inf)
{
	return inf->active & SWITCHTEC_FW_PART_ACTIVE;
}

static inline int switchtec_fw_running(struct switchtec_fw_image_info *inf)
{
	return inf->active & SWITCHTEC_FW_PART_RUNNING;
}

struct switchtec_fw_footer {
	char magic[4];
	uint32_t image_len;
	uint32_t load_addr;
	uint32_t version;
	uint32_t rsvd;
	uint32_t header_crc;
	uint32_t image_crc;
};

int switchtec_fw_dlstatus(struct switchtec_dev *dev,
			  enum switchtec_fw_dlstatus *status,
			  enum mrpc_bg_status *bgstatus);
int switchtec_fw_wait(struct switchtec_dev *dev,
		      enum switchtec_fw_dlstatus *status);
int switchtec_fw_toggle_active_partition(struct switchtec_dev *dev,
					 int toggle_fw, int toggle_cfg);
int switchtec_fw_write_fd(struct switchtec_dev *dev, int img_fd,
			  int dont_activate,
			  void (*progress_callback)(int cur, int tot));
int switchtec_fw_write_file(struct switchtec_dev *dev, FILE *fimg,
			    int dont_activate,
			    void (*progress_callback)(int cur, int tot));
int switchtec_fw_read_fd(struct switchtec_dev *dev, int fd,
			 unsigned long addr, size_t len,
			 void (*progress_callback)(int cur, int tot));
int switchtec_fw_read(struct switchtec_dev *dev, unsigned long addr,
		      size_t len, void *buf);
int switchtec_fw_read_footer(struct switchtec_dev *dev,
			     unsigned long partition_start,
			     size_t partition_len,
			     struct switchtec_fw_footer *ftr,
			     char *version, size_t version_len);
void switchtec_fw_perror(const char *s, int ret);
int switchtec_fw_file_info(int fd, struct switchtec_fw_image_info *info);
const char *switchtec_fw_image_type(const struct switchtec_fw_image_info *info);
int switchtec_fw_part_info(struct switchtec_dev *dev, int nr_info,
			   struct switchtec_fw_image_info *info);
int switchtec_fw_img_info(struct switchtec_dev *dev,
			  struct switchtec_fw_image_info *act_img,
			  struct switchtec_fw_image_info *inact_img);
int switchtec_fw_cfg_info(struct switchtec_dev *dev,
			  struct switchtec_fw_image_info *act_cfg,
			  struct switchtec_fw_image_info *inact_cfg,
			  struct switchtec_fw_image_info *mult_cfg,
			  int *nr_mult);
int switchtec_fw_img_write_hdr(int fd, struct switchtec_fw_footer *ftr,
			       enum switchtec_fw_image_type type);
int switchtec_fw_is_boot_ro(struct switchtec_dev *dev);
int switchtec_fw_set_boot_ro(struct switchtec_dev *dev,
			     enum switchtec_fw_ro ro);

/********** EVENT COUNTER *********/

enum switchtec_evcntr_type_mask {
	UNSUP_REQ_ERR = 1 << 0,
	ECRC_ERR = 1 << 1,
	MALFORM_TLP_ERR = 1 << 2,
	RCVR_OFLOW_ERR = 1 << 3,
	CMPLTR_ABORT_ERR = 1 << 4,
	POISONED_TLP_ERR = 1 << 5,
	SURPRISE_DOWN_ERR = 1 << 6,
	DATA_LINK_PROTO_ERR = 1 << 7,
	HDR_LOG_OFLOW_ERR = 1 << 8,
	UNCOR_INT_ERR = 1 << 9,
	REPLAY_TMR_TIMEOUT = 1 << 10,
	REPLAY_NUM_ROLLOVER = 1 << 11,
	BAD_DLPP = 1 << 12,
	BAD_TLP = 1 << 13,
	RCVR_ERR = 1 << 14,
	RCV_FATAL_MSG = 1 << 15,
	RCV_NON_FATAL_MSG = 1 << 16,
	RCV_CORR_MSG = 1 << 17,
	NAK_RCVD = 1 << 18,
	RULE_TABLE_HIT = 1 << 19,
	POSTED_TLP = 1 << 20,
	COMP_TLP = 1 << 21,
	NON_POSTED_TLP = 1 << 22,
	ALL_ERRORS = (UNSUP_REQ_ERR | ECRC_ERR | MALFORM_TLP_ERR |
		      RCVR_OFLOW_ERR | CMPLTR_ABORT_ERR | POISONED_TLP_ERR |
		      SURPRISE_DOWN_ERR | DATA_LINK_PROTO_ERR |
		      HDR_LOG_OFLOW_ERR | UNCOR_INT_ERR |
		      REPLAY_TMR_TIMEOUT | REPLAY_NUM_ROLLOVER |
		      BAD_DLPP | BAD_TLP | RCVR_ERR | RCV_FATAL_MSG |
		      RCV_NON_FATAL_MSG | RCV_CORR_MSG | NAK_RCVD),
	ALL_TLPS = (POSTED_TLP | COMP_TLP | NON_POSTED_TLP),
	ALL = (1 << 23) - 1,
};

extern const struct switchtec_evcntr_type_list {
	enum switchtec_evcntr_type_mask mask;
	const char *name;
	const char *help;
} switchtec_evcntr_type_list[];

struct switchtec_evcntr_setup {
	unsigned port_mask;
	enum switchtec_evcntr_type_mask type_mask;
	int egress;
	unsigned threshold;
};

int switchtec_evcntr_type_count(void);
const char *switchtec_evcntr_type_str(int *type_mask);
int switchtec_evcntr_setup(struct switchtec_dev *dev, unsigned stack_id,
			   unsigned cntr_id,
			   struct switchtec_evcntr_setup *setup);
int switchtec_evcntr_get_setup(struct switchtec_dev *dev, unsigned stack_id,
			       unsigned cntr_id, unsigned nr_cntrs,
			       struct switchtec_evcntr_setup *res);
int switchtec_evcntr_get(struct switchtec_dev *dev, unsigned stack_id,
			 unsigned cntr_id, unsigned nr_cntrs, unsigned *res,
			 int clear);
int switchtec_evcntr_get_both(struct switchtec_dev *dev, unsigned stack_id,
			      unsigned cntr_id, unsigned nr_cntrs,
			      struct switchtec_evcntr_setup *setup,
			      unsigned *counts, int clear);
int switchtec_evcntr_wait(struct switchtec_dev *dev, int timeout_ms);

/********** BANDWIDTH COUNTER *********/

struct switchtec_bwcntr_res {
	uint64_t time_us;
	struct switchtec_bwcntr_dir {
		uint64_t posted;
		uint64_t comp;
		uint64_t nonposted;
	} egress, ingress;
};

void switchtec_bwcntr_sub(struct switchtec_bwcntr_res *new,
			  struct switchtec_bwcntr_res *old);

int switchtec_bwcntr_many(struct switchtec_dev *dev, int nr_ports,
			  int *phys_port_ids, int clear,
			  struct switchtec_bwcntr_res *res);
int switchtec_bwcntr_all(struct switchtec_dev *dev, int clear,
			 struct switchtec_port_id **ports,
			 struct switchtec_bwcntr_res **res);
uint64_t switchtec_bwcntr_tot(struct switchtec_bwcntr_dir *d);

/********** LATENCY COUNTER *********/

#define SWITCHTEC_LAT_ALL_INGRESS 63

int switchtec_lat_setup_many(struct switchtec_dev *dev, int nr_ports,
			     int *egress_port_ids, int *ingress_port_ids);
int switchtec_lat_setup(struct switchtec_dev *dev, int egress_port_id,
			int ingress_port_id, int clear);
int switchtec_lat_get_many(struct switchtec_dev *dev, int nr_ports,
			   int clear, int *egress_port_ids,
			   int *cur_ns, int *max_ns);
int switchtec_lat_get(struct switchtec_dev *dev, int clear,
		      int egress_port_ids, int *cur_ns,
		      int *max_ns);

/********** GLOBAL ADDRESS SPACE ACCESS *********/

/*
 * GAS map maps the hardware registers into user memory space.
 * Needless to say, this can be very dangerous and should only
 * be done if you know what you are doing. Any register accesses
 * that use this will remain unsupported by Microsemi unless it's
 * done within the switchtec user project or otherwise specified.
 */

void *switchtec_gas_map(struct switchtec_dev *dev, int writeable,
			size_t *map_size);
void switchtec_gas_unmap(struct switchtec_dev *dev, void *map);
int switchtec_gas_read(struct switchtec_dev *dev, uint8_t *data,
			uint32_t offset, uint32_t size);
int switchtec_gas_write(struct switchtec_dev *dev, uint8_t *data,
			uint32_t offset, uint32_t size);

enum {
	SWITCHTEC_GAS_MRPC_OFFSET       = 0x0000,
	SWITCHTEC_GAS_TOP_CFG_OFFSET    = 0x1000,
	SWITCHTEC_GAS_SW_EVENT_OFFSET   = 0x1800,
	SWITCHTEC_GAS_SYS_INFO_OFFSET   = 0x2000,
	SWITCHTEC_GAS_FLASH_INFO_OFFSET = 0x2200,
	SWITCHTEC_GAS_PART_CFG_OFFSET   = 0x4000,
	SWITCHTEC_GAS_NTB_OFFSET        = 0x10000,
	SWITCHTEC_GAS_PFF_CSR_OFFSET    = 0x134000,
};

enum {
	SWITCHTEC_CFG0_RUNNING = 0x04,
	SWITCHTEC_CFG1_RUNNING = 0x05,
	SWITCHTEC_IMG0_RUNNING = 0x03,
	SWITCHTEC_IMG1_RUNNING = 0x07,
};

struct sys_info_regs {
	uint32_t device_id;
	uint32_t device_version;
	uint32_t firmware_version;
	uint32_t reserved1;
	uint32_t vendor_table_revision;
	uint32_t table_format_version;
	uint32_t partition_id;
	uint32_t cfg_file_fmt_version;
	uint16_t cfg_running;
	uint16_t img_running;
	uint32_t reserved2[57];
	char vendor_id[8];
	char product_id[16];
	char product_revision[4];
	char component_vendor[8];
	uint16_t component_id;
	uint8_t component_revision;
} __attribute__(( packed ));

struct flash_info_regs {
	uint32_t flash_part_map_upd_idx;

	struct active_partition_info {
		uint32_t address;
		uint32_t build_version;
		uint32_t build_string;
	} __attribute__(( packed )) active_img;

	struct active_partition_info active_cfg;
	struct active_partition_info inactive_img;
	struct active_partition_info inactive_cfg;

	uint32_t flash_length;

	struct partition_info {
		uint32_t address;
		uint32_t length;
	} __attribute__(( packed )) cfg0;

	struct partition_info cfg1;
	struct partition_info img0;
	struct partition_info img1;
	struct partition_info nvlog;
	struct partition_info vendor[8];
} __attribute__(( packed ));

#ifdef __cplusplus
}
#endif

#endif
