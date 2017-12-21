/*
 * Microsemi Switchtec(tm) PCIe Management Command Line Interface
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

#include "commands.h"
#include "argconfig.h"
#include "suffix.h"
#include "progress.h"
#include "gui.h"
#include "common.h"

#include <switchtec/switchtec.h>
#include <switchtec/utils.h>

#include <locale.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>

static struct {} empty_cfg;
const struct argconfig_options empty_opts[] = {{NULL}};

struct switchtec_dev *global_dev = NULL;

int switchtec_handler(const char *optarg, void *value_addr,
		      const struct argconfig_options *opt)
{
	struct switchtec_dev *dev;

	global_dev = dev = switchtec_open(optarg);

	if (dev == NULL) {
		switchtec_perror(optarg);
		return 1;
	}

	*((struct switchtec_dev  **) value_addr) = dev;
	return 0;
}

static int list(int argc, char **argv)
{
	struct switchtec_device_info *devices;
	int i, n;
	const char *desc = "List all the switchtec devices on this machine";

	argconfig_parse(argc, argv, desc, empty_opts, &empty_cfg,
			sizeof(empty_cfg));

	n = switchtec_list(&devices);
	if (n < 0)
		return n;

	for (i = 0; i < n; i++)
		printf("%-20s\t%-15s\t%-5s\t%-10s\t%s\n", devices[i].path,
		       devices[i].product_id, devices[i].product_rev,
		       devices[i].fw_version, devices[i].pci_dev);

	free(devices);
	return 0;
}

static void print_port_title(struct switchtec_dev *dev,
			     struct switchtec_port_id *p)
{
	static int last_partition = -1;
	const char *local = "";

	if (p->partition != last_partition) {
		if (p->partition == SWITCHTEC_UNBOUND_PORT) {
			printf("Unbound Ports:\n");
		} else {
			if (p->partition == switchtec_partition(dev))
				local = "    (LOCAL)";
			printf("Partition %d:%s\n", p->partition, local);
		}
	}
	last_partition = p->partition;

	if (p->partition == SWITCHTEC_UNBOUND_PORT) {
		printf("    Phys Port ID %d  (Stack %d, Port %d)\n",
		       p->phys_id, p->stack, p->stk_id);
	} else {
		printf("    Logical Port ID %d (%s):\n", p->log_id,
		       p->upstream ? "USP" : "DSP");
	}
}

static int gui(int argc, char **argv)
{

	const char *desc = "Display a simple ncurses GUI for the switch";
	int ret;

	static struct {
		struct switchtec_dev *dev;
		unsigned all_ports;
		unsigned reset_bytes;
		unsigned refresh;
		int duration;
	} cfg = {
	    .refresh  = 1,
	    .duration = -1,
	};

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"all_ports", 'a', "", CFG_NONE, &cfg.all_ports, no_argument,
		 "show all ports (including downed links)"},
		{"reset", 'r', "", CFG_NONE, &cfg.reset_bytes, no_argument,
		 "reset byte counters"},
		{"refresh", 'f', "", CFG_POSITIVE, &cfg.refresh, required_argument,
		 "gui refresh period in seconds (default: 1 second)"},
		{"duration", 'd', "", CFG_INT, &cfg.duration, required_argument,
		 "gui duration in seconds (-1 forever)"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = gui_main(cfg.dev, cfg.all_ports, cfg.reset_bytes, cfg.refresh,
		       cfg. duration);

	return ret;
}

static int status(int argc, char **argv)
{
	const char *desc = "Display status of the ports on the switch";
	int ret;
	int ports;
	struct switchtec_status *status;
	int port_ids[SWITCHTEC_MAX_PORTS];
	struct switchtec_bwcntr_res bw_data[SWITCHTEC_MAX_PORTS];
	int p;
	double bw_val;
	const char *bw_suf;

	static struct {
		struct switchtec_dev *dev;
		int reset_bytes;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"reset", 'r', "", CFG_NONE, &cfg.reset_bytes, no_argument,
		 "reset byte counters"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ports = switchtec_status(cfg.dev, &status);
	if (ports < 0) {
		switchtec_perror("status");
		return ports;
	}

	ret = switchtec_get_devices(cfg.dev, status, ports);
	if (ret < 0) {
		switchtec_perror("get_devices");
		goto free_and_return;
	}

	for (p = 0; p < ports; p++)
		port_ids[p] = status[p].port.phys_id;

	ret = switchtec_bwcntr_many(cfg.dev, ports, port_ids, cfg.reset_bytes,
				    bw_data);
	if (ret < 0) {
		switchtec_perror("bwcntr");
		goto free_and_return;
	}

	if (cfg.reset_bytes)
		memset(bw_data, 0, sizeof(bw_data));

	for (p = 0; p < ports; p++) {
		struct switchtec_status *s = &status[p];
		print_port_title(cfg.dev, &s->port);

		if (s->port.partition == SWITCHTEC_UNBOUND_PORT)
			continue;

		printf("\tPhys Port ID:    \t%d (Stack %d, Port %d)\n",
		       s->port.phys_id, s->port.stack, s->port.stk_id);

		printf("\tStatus:          \t%s\n",
		       s->link_up ? "UP" : "DOWN");
		printf("\tLTSSM:           \t%s\n", s->ltssm_str);
		printf("\tMax-Width:       \tx%d\n", s->cfg_lnk_width);

		if (!s->link_up) continue;

		printf("\tNeg Width:       \tx%d\n", s->neg_lnk_width);
		printf("\tRate:            \tGen%d - %g GT/s  %g GB/s\n",
		       s->link_rate, switchtec_gen_transfers[s->link_rate],
		       switchtec_gen_datarate[s->link_rate]*s->neg_lnk_width/1000.);

		bw_val = switchtec_bwcntr_tot(&bw_data[p].egress);
		bw_suf = suffix_si_get(&bw_val);
		printf("\tOut Bytes:       \t%-.3g %sB\n", bw_val, bw_suf);

		bw_val = switchtec_bwcntr_tot(&bw_data[p].ingress);
		bw_suf = suffix_si_get(&bw_val);
		printf("\tIn Bytes:        \t%-.3g %sB\n", bw_val, bw_suf);

		if (!s->vendor_id || !s->device_id || !s->pci_dev)
			continue;

		printf("\tDevice:          \t%04x:%04x (%s)\n",
		       s->vendor_id, s->device_id, s->pci_dev);
		if (s->class_devices)
			printf("\t                 \t%s\n", s->class_devices);
	}

	ret = 0;

free_and_return:
	switchtec_status_free(status, ports);

	return ret;
}

static void print_bw(const char *msg, uint64_t time_us, uint64_t bytes)
{
	double rate = bytes / (time_us * 1e-6);
	const char *suf = suffix_si_get(&rate);

	printf("\t%-8s\t%5.3g %sB/s\n", msg, rate, suf);
}

static int bw(int argc, char **argv)
{
	const char *desc = "Measure switch bandwidth";
	struct switchtec_bwcntr_res *before, *after;
	struct switchtec_port_id *port_ids;
	int ret;
	int i;
	uint64_t ingress_tot, egress_tot;

	static struct {
		struct switchtec_dev *dev;
		unsigned meas_time;
		int verbose;
	} cfg = {
		.meas_time = 5,
	};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"time", 't', "NUM", CFG_POSITIVE, &cfg.meas_time,
		  required_argument,
		 "measurement time, in seconds"},
		{"verbose", 'v', "", CFG_NONE, &cfg.verbose, no_argument,
		 "print posted, non-posted and completion results"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_bwcntr_all(cfg.dev, 0, &port_ids, &before);
	if (ret < 0) {
		switchtec_perror("bw");
		return ret;
	}

	sleep(cfg.meas_time);

	ret = switchtec_bwcntr_all(cfg.dev, 0, NULL, &after);
	if (ret < 0) {
		free(before);
		free(port_ids);
		switchtec_perror("bw");
		return ret;
	}

	for (i = 0; i < ret; i++) {
		print_port_title(cfg.dev, &port_ids[i]);

		switchtec_bwcntr_sub(&after[i], &before[i]);

		egress_tot = switchtec_bwcntr_tot(&after[i].egress);
		ingress_tot = switchtec_bwcntr_tot(&after[i].ingress);

		if (!cfg.verbose) {
			print_bw("Out:", after[i].time_us, egress_tot);
			print_bw("In:", after[i].time_us, ingress_tot);
		} else {
			printf("\tOut:\n");
			print_bw("  Posted:", after[i].time_us,
				 after[i].egress.posted);
			print_bw("  Non-Posted:", after[i].time_us,
				 after[i].egress.nonposted);
			print_bw("  Completion:", after[i].time_us,
				 after[i].egress.comp);
			print_bw("  Total:", after[i].time_us, egress_tot);

			printf("\tIn:\n");
			print_bw("  Posted:", after[i].time_us,
				 after[i].ingress.posted);
			print_bw("  Non-Posted:", after[i].time_us,
				 after[i].ingress.nonposted);
			print_bw("  Completion:", after[i].time_us,
				 after[i].ingress.comp);
			print_bw("  Total:", after[i].time_us, ingress_tot);
		}
	}

	free(before);
	free(after);
	free(port_ids);
	return 0;
}

static int latency(int argc, char **argv)
{
	const char *desc = "Measure latency of a port";
	int ret;
	int cur_ns, max_ns;

	static struct {
		struct switchtec_dev *dev;
		unsigned meas_time;
		int egress;
		int ingress;
	} cfg = {
		.meas_time = 5,
		.egress = -1,
		.ingress = SWITCHTEC_LAT_ALL_INGRESS,
	};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"time", 't', "NUM", CFG_POSITIVE, &cfg.meas_time,
		  required_argument,
		 "measurement time, in seconds"},
		{"egress", 'e', "NUM", CFG_POSITIVE, &cfg.egress,
		  required_argument,
		 "physical port id for the egress side",
		 .require_in_usage=1},
		{"ingress", 'i', "NUM", CFG_POSITIVE, &cfg.ingress,
		  required_argument,
		 "physical port id for the ingress side, by default use all ports"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (cfg.egress < 0) {
		argconfig_print_usage(opts);
		fprintf(stderr, "The --egress argument is required!\n");
		return 1;
	}

	ret = switchtec_lat_setup(cfg.dev, cfg.egress, cfg.ingress, 1);
	if (ret < 0) {
		switchtec_perror("latency");
		return -1;
	}

	sleep(cfg.meas_time);

	ret = switchtec_lat_get(cfg.dev, 0, cfg.egress, &cur_ns, &max_ns);
	if (ret < 0) {
		switchtec_perror("latency");
		return -1;
	}

	printf("Current: %d ns\n", cur_ns);
	printf("Maximum: %d ns\n", max_ns);

	return 0;
}

struct event_list {
	enum switchtec_event_id eid;
	int partition;
	int port;
	unsigned count;
};

static int compare_event_list(const void *aa, const void *bb)
{
	const struct event_list *a = aa, *b = bb;

	if (a->partition != b->partition)
		return a->partition - b->partition;
	if (a->port != b->port)
		return a->port - b->port;

	return a->eid - b->eid;
}

static void print_event_list(struct event_list *e, size_t cnt)
{
	const char *name, *desc;
	int last_part = -2, last_port = -2;

	while (cnt--) {
		if (e->partition != last_part) {
			if (e->partition == -1)
				printf("Global Events:\n");
			else
				printf("Partition %d Events:\n", e->partition);
		}

		if (e->port != last_port && e->port != -1) {
			if (e->port == SWITCHTEC_PFF_PORT_VEP)
				printf("    Port VEP:\n");
			else
				printf("    Port %d:\n", e->port);
		}

		last_part = e->partition;
		last_port = e->port;

		switchtec_event_info(e->eid, &name, &desc);
		printf("\t%-22s\t%-4u\t%s\n", name, e->count, desc);

		e++;
	}
}

static void populate_event_choices(struct argconfig_choice *c, int mask)
{
	int i;

	for (i = 0; i < SWITCHTEC_MAX_EVENTS; i++) {
		if (mask)
			c->value = 1 << i;
		else
			c->value = i;
		switchtec_event_info(i, &c->name, &c->help);
		c++;
	}
}

static int get_events(struct switchtec_dev *dev,
		      struct switchtec_event_summary *sum,
		      struct event_list *elist, size_t elist_len,
		      int event_id, int show_all, int clear_all,
		      int index)
{
	struct event_list *e = elist;
	enum switchtec_event_type type;
	int flags;
	int idx;
	int ret;
	int local_part;

	local_part = switchtec_partition(dev);

	while (switchtec_event_summary_iter(sum, &e->eid, &idx)) {
		if (e->eid == -1)
			continue;

		type = switchtec_event_info(e->eid, NULL, NULL);

		if (index >= 0 && index != idx)
			continue;

		switch (type)  {
		case SWITCHTEC_EVT_GLOBAL:
			e->partition = -1;
			e->port = -1;
			break;
		case SWITCHTEC_EVT_PART:
			e->partition = idx;
			e->port = -1;
			break;
		case SWITCHTEC_EVT_PFF:
			ret = switchtec_pff_to_port(dev, idx, &e->partition,
						    &e->port);
			if (ret < 0) {
				perror("pff_to_port");
				return -1;
			}
			break;
		}

		if (!show_all && e->partition != local_part)
			continue;

		if (clear_all || event_id & (1 << e->eid))
			flags = SWITCHTEC_EVT_FLAG_CLEAR;
		else
			flags = 0;

		ret = switchtec_event_ctl(dev, e->eid, idx, flags, NULL);
		if (ret < 0) {
			perror("event_ctl");
			return -1;
		}

		e->count = ret;
		e++;
		if (e - elist > elist_len)
			break;
	}

	return e - elist;
}

static int events(int argc, char **argv)
{
	const char *desc = "Display information on events that have occurred";
	struct event_list elist[256];
	struct switchtec_event_summary sum;
	struct argconfig_choice event_choices[SWITCHTEC_MAX_EVENTS + 1] = {};
	int ret;

	static struct {
		struct switchtec_dev *dev;
		int show_all;
		int clear_all;
		unsigned event_id;
	} cfg = {};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"all", 'a', "", CFG_NONE, &cfg.show_all, no_argument,
		 "show events in all partitions"},
		{"reset", 'r', "", CFG_NONE, &cfg.clear_all, no_argument,
		 "clear all events"},
		{"event", 'e', "EVENT", CFG_MULT_CHOICES, &cfg.event_id,
		  required_argument, .choices=event_choices,
		  .help="clear all events of a specified type"},
		{NULL}};

	populate_event_choices(event_choices, 1);
	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_event_summary(cfg.dev, &sum);
	if (ret < 0) {
		perror("event_summary");
		return ret;
	}

	ret = get_events(cfg.dev, &sum, elist, ARRAY_SIZE(elist), cfg.event_id,
			 cfg.show_all, cfg.clear_all, -1);
	if (ret < 0)
		return ret;

	qsort(elist, ret, sizeof(*elist), compare_event_list);
	print_event_list(elist, ret);

	return 0;
}

static int event_wait(int argc, char **argv)
{
	const char *desc = "Wait for an event to occur";
	struct event_list elist[256];
	struct switchtec_event_summary sum;
	struct argconfig_choice event_choices[SWITCHTEC_MAX_EVENTS + 1] = {};
	int index;
	int ret;

	static struct {
		struct switchtec_dev *dev;
		int show_all;
		int partition;
		int port;
		int timeout;
		unsigned event_id;
	} cfg = {
		.partition = -1,
		.port = -1,
		.timeout = -1,
	};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"event", 'e', "EVENT", CFG_CHOICES, &cfg.event_id,
		  required_argument, .choices=event_choices,
		  .help="event to wait on"},
		{"partition", 'p', "NUM", CFG_POSITIVE, &cfg.partition,
		  required_argument,
		  .help="partition number for the event"},
		{"port", 'q', "NUM", CFG_POSITIVE, &cfg.port,
		  required_argument,
		  .help="port number for the event"},
		{"timeout", 't', "MS", CFG_INT, &cfg.timeout,
		  required_argument,
		  "timeout in milliseconds"},
		{NULL}};

	populate_event_choices(event_choices, 0);
	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	switch (switchtec_event_info(cfg.event_id, NULL, NULL)) {
	case SWITCHTEC_EVT_GLOBAL:
		break;
	case SWITCHTEC_EVT_PART:
		if (cfg.port < 0) {
			fprintf(stderr, "Port cannot be specified for this event type.\n");
			return -1;
		}

		if (cfg.partition < 0)
			index = SWITCHTEC_EVT_IDX_ALL;
		else
			index = cfg.partition;

		break;
	case SWITCHTEC_EVT_PFF:
		if (cfg.partition < 0 && cfg.port < 0) {
			index = SWITCHTEC_EVT_IDX_ALL;
		} else if (cfg.partition < 0 || cfg.port < 0) {
			fprintf(stderr, "Must specify partition and port for this event type.\n");
			return -1;
		} else {
			ret = switchtec_port_to_pff(cfg.dev, cfg.partition,
						    cfg.port, &index);
			if (ret) {
				perror("port");
				return ret;
			}
		}
	}

	ret = switchtec_event_wait_for(cfg.dev, cfg.event_id, index, &sum,
				       cfg.timeout);
	if (ret < 0) {
		switchtec_perror("event-wait");
		return ret;
	}

	ret = get_events(cfg.dev, &sum, elist, ARRAY_SIZE(elist),
			 1 << cfg.event_id, 0, 0, index);
	if (ret < 0)
		return ret;

	qsort(elist, ret, sizeof(*elist), compare_event_list);
	print_event_list(elist, ret);

	return 0;
}

static int log_dump(int argc, char **argv)
{
	const char *desc = "Dump the raw APP log to a file";
	int ret;

	const struct argconfig_choice types[] = {
		{"RAM", SWITCHTEC_LOG_RAM, "Dump the APP log from RAM"},
		{"FLASH", SWITCHTEC_LOG_FLASH, "Dump the APP log from FLASH"},
		{"MEMLOG", SWITCHTEC_LOG_MEMLOG,
		 "Dump the Memlog info from Flash in last fatal error handing dump"},
		{"REGS", SWITCHTEC_LOG_REGS,
		 "Dump the Generic Registers context from Flash in last fatal error handing dump"},
		{"THRD_STACK", SWITCHTEC_LOG_THRD_STACK,
		 "Dump the thread stack info from Flash in last fatal error handing dump"},
		{"SYS_STACK", SWITCHTEC_LOG_SYS_STACK,
		 "Dump the system stack info from Flash in last fatal error handing dump"},
		{"THRDS", SWITCHTEC_LOG_THRD,
		 "Dump all thread info from Flash in last fatal error handing dump"},
		{0}
	};

	static struct {
		struct switchtec_dev *dev;
		int out_fd;
		const char *out_filename;
		unsigned type;
	} cfg = {
		.type = SWITCHTEC_LOG_RAM
	};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"filename", .cfg_type=CFG_FD_WR, .value_addr=&cfg.out_fd,
		  .argument_type=optional_positional,
		  .force_default="switchtec.log",
		  .help="image file to display information for"},
		{"type", 't', "TYPE", CFG_CHOICES, &cfg.type,
		  required_argument,
		 "log type to dump", .choices=types},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_log_to_file(cfg.dev, cfg.type, cfg.out_fd);
	if (ret < 0) {
		switchtec_perror("log");
		return ret;
	}

	fprintf(stderr, "\nLog saved to %s.\n", cfg.out_filename);

	return ret;
}

static int test(int argc, char **argv)
{
	const char *desc = "Test if switchtec interface is working";
	int ret;
	uint32_t in, out;

	static struct {
		struct switchtec_dev *dev;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	in = time(NULL);

	ret = switchtec_echo(cfg.dev, in, &out);

	if (ret) {
		switchtec_perror(argv[optind]);
		return ret;
	}

	if (in != ~out) {
		fprintf(stderr, "argv[optind]: echo command returned the "
			"wrong result; got %x, expected %x\n",
			out, ~in);
		return 1;
	}

	fprintf(stderr, "%s: success\n", argv[optind-1]);

	return 0;
}

static int temp(int argc, char **argv)
{
	const char *desc = "Display die temperature of the switchtec device";
	float ret;

	static struct {
		struct switchtec_dev *dev;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_die_temp(cfg.dev);
	if (ret < 0) {
		switchtec_perror("die_temp");
		return 1;
	}

	printf("%.3g °C\n", ret);

	return 0;
}

int ask_if_sure(int always_yes)
{
	char buf[10];
	char *ret;

	if (always_yes)
		return 0;

	fprintf(stderr, "Do you want to continue? [y/N] ");
	ret = fgets(buf, sizeof(buf), stdin);

	if (!ret)
		goto abort;

	if (strcmp(buf, "y\n") == 0 || strcmp(buf, "Y\n") == 0)
		return 0;

abort:
	fprintf(stderr, "Abort.\n");
	errno = EINTR;
	return -errno;
}

static int hard_reset(int argc, char **argv)
{
	const char *desc = "Perform a hard reset on the switch";
	int ret;

	static struct {
		struct switchtec_dev *dev;
		int assume_yes;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"yes", 'y', "", CFG_NONE, &cfg.assume_yes, no_argument,
		 "assume yes when prompted"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (!cfg.assume_yes)
		fprintf(stderr,
			"WARNING: if your system does not support hotplug,\n"
			"a hard reset can leave the system in a broken state.\n"
			"Make sure you reboot after issuing this command.\n\n");

	ret = ask_if_sure(cfg.assume_yes);
	if (ret)
		return ret;

	ret = switchtec_hard_reset(cfg.dev);
	if (ret) {
		switchtec_perror(argv[optind]);
		return ret;
	}

	fprintf(stderr, "%s: hard reset\n", switchtec_name(cfg.dev));
	return 0;
}

static const char *get_basename(const char *buf)
{
	const char *slash = strrchr(buf, '/');

	if (slash)
		return slash+1;

	return buf;
}

static enum switchtec_fw_image_type check_and_print_fw_image(
	int img_fd, const char *img_filename)
{
	int ret;
	struct switchtec_fw_image_info info;
	ret = switchtec_fw_file_info(img_fd, &info);

	if (ret < 0) {
		fprintf(stderr, "%s: Invalid image file format\n",
			img_filename);
		return ret;
	}

	printf("File:     %s\n", get_basename(img_filename));
	printf("Type:     %s\n", switchtec_fw_image_type(&info));
	printf("Version:  %s\n", info.version);
	printf("Img Len:  0x%zx\n", info.image_len);
	printf("CRC:      0x%08lx\n", info.crc);

	return info.type;
}

static int fw_img_info(int argc, char **argv)
{
	const char *desc = "Display information for a firmware image";
	int ret;

	static struct {
		int img_fd;
		const char *img_filename;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		{"img_file", .cfg_type=CFG_FD_RD, .value_addr=&cfg.img_fd,
		  .argument_type=required_positional,
		  .help="image file to display information for"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = check_and_print_fw_image(cfg.img_fd, cfg.img_filename);
	if (ret < 0)
		return ret;

	close(cfg.img_fd);
	return 0;
}

static const char *fw_active_string(struct switchtec_fw_image_info *inf)
{
	return switchtec_fw_active(inf) ? " - Active" : "";
}

static const char *fw_running_string(struct switchtec_fw_image_info *inf)
{
	return switchtec_fw_running(inf) ? "\t(Running)" : "";
}

static int print_fw_part_info(struct switchtec_dev *dev)
{
	int nr_mult = 16;
	struct switchtec_fw_image_info act_img, inact_img, act_cfg, inact_cfg,
		mult_cfg[nr_mult];
	struct switchtec_fw_footer bootloader;
	char bootloader_ver[16];
	int bootloader_ro;
	int ret, i;

	ret = switchtec_fw_img_info(dev, &act_img, &inact_img);
	if (ret < 0)
		return ret;

	ret = switchtec_fw_cfg_info(dev, &act_cfg, &inact_cfg, mult_cfg,
				    &nr_mult);

	if (ret < 0)
		return ret;

	ret = switchtec_fw_read_footer(dev, 0xa8000000, 0x10000,
				       &bootloader, bootloader_ver,
				       sizeof(bootloader_ver));
	if (ret < 0) {
		switchtec_perror("BOOT");
		return ret;
	}

	bootloader_ro = switchtec_fw_is_boot_ro(dev);
	if (bootloader_ro != SWITCHTEC_FW_RO)
		bootloader_ro = 0;

	printf("Active Partition:\n");
	printf("  BOOT \tVersion: %-8s\tCRC: %08lx   %s\n",
	       bootloader_ver, (long)bootloader.image_crc,
	       bootloader_ro ? "(RO)" : "");
	printf("  IMG  \tVersion: %-8s\tCRC: %08lx%s\n",
	       act_img.version, act_img.crc,
	       fw_running_string(&act_img));
	printf("  CFG  \tVersion: %-8s\tCRC: %08lx%s\n",
	       act_cfg.version, act_cfg.crc,
	       fw_running_string(&act_cfg));

	for (i = 0; i < nr_mult; i++) {
		printf("   \tMulti Config %d%s\n", i,
		       fw_active_string(&mult_cfg[i]));
	}

	printf("Inactive Partition:\n");
	printf("  IMG  \tVersion: %-8s\tCRC: %08lx%s\n",
	       inact_img.version, inact_img.crc,
	       fw_running_string(&inact_img));
	printf("  CFG  \tVersion: %-8s\tCRC: %08lx%s\n",
	       inact_cfg.version, inact_cfg.crc,
	       fw_running_string(&inact_cfg));

	return 0;
}

static int fw_info(int argc, char **argv)
{
	const char *desc = "Test if switchtec interface is working";
	int ret;
	char version[64];

	static struct {
		struct switchtec_dev *dev;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_get_fw_version(cfg.dev, version, sizeof(version));
	if (ret < 0) {
		switchtec_perror("fw info");
		return ret;
	}

	printf("Currently Running:\n");
	printf("  IMG Version: %s\n", version);

	print_fw_part_info(cfg.dev);

	return 0;
}

static int fw_update(int argc, char **argv)
{
	int ret;
	const char *desc = "Flash the firmware with a new image";

	static struct {
		struct switchtec_dev *dev;
		FILE *fimg;
		const char *img_filename;
		int assume_yes;
		int dont_activate;
		int set_boot_rw;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"img_file", .cfg_type=CFG_FILE_R, .value_addr=&cfg.fimg,
		  .argument_type=required_positional,
		  .help="image file to use as the new firmware"},
		{"yes", 'y', "", CFG_NONE, &cfg.assume_yes, no_argument,
		 "assume yes when prompted"},
		{"dont-activate", 'A', "", CFG_NONE, &cfg.dont_activate, no_argument,
		 "don't activate the new image, use fw-toggle to do so "
		 "when it is safe"},
		{"set-boot-rw", 'W', "", CFG_NONE, &cfg.set_boot_rw, no_argument,
		 "set the bootloader partition as RW (only valid for BOOT images)"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	printf("Writing the following firmware image to %s.\n",
	       switchtec_name(cfg.dev));

	ret = check_and_print_fw_image(fileno(cfg.fimg), cfg.img_filename);
	if (ret < 0)
		return ret;

	ret = ask_if_sure(cfg.assume_yes);
	if (ret) {
		fclose(cfg.fimg);
		return ret;
	}

	if (cfg.set_boot_rw && ret != SWITCHTEC_FW_TYPE_BOOT) {
		fprintf(stderr, "The --set-boot-rw option only applies for BOOT images\n");
		return -1;
	} else if (ret == SWITCHTEC_FW_TYPE_BOOT) {
		if (cfg.set_boot_rw)
			switchtec_fw_set_boot_ro(cfg.dev, SWITCHTEC_FW_RW);

		if (switchtec_fw_is_boot_ro(cfg.dev) == SWITCHTEC_FW_RO) {
			fprintf(stderr, "\nfirmware update: the BOOT partition is read-only. "
				"use --set-boot-rw to override\n");
			return -1;
		}
	}

	progress_start();
	ret = switchtec_fw_write_file(cfg.dev, cfg.fimg, cfg.dont_activate,
				      progress_update);
	fclose(cfg.fimg);

	if (ret) {
		printf("\n");
		switchtec_fw_perror("firmware update", ret);
		goto set_boot_ro;
	}

	progress_finish();
	printf("\n");

	print_fw_part_info(cfg.dev);
	printf("\n");

set_boot_ro:
	if (cfg.set_boot_rw)
		switchtec_fw_set_boot_ro(cfg.dev, SWITCHTEC_FW_RO);

	return ret;
}

static int fw_toggle(int argc, char **argv)
{
	const char *desc = "Toggle active and inactive firmware partitions";
	int ret = 0;

	static struct {
		struct switchtec_dev *dev;
		int firmware;
		int config;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"firmware", 'f', "", CFG_NONE, &cfg.firmware, no_argument,
		 "toggle IMG firmware"},
		{"config", 'c', "", CFG_NONE, &cfg.config, no_argument,
		 "toggle CFG data"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (!cfg.firmware && !cfg.config) {
		fprintf(stderr, "NOTE: Not toggling images seeing neither "
			"--firmware nor --config were specified\n\n");
	} else {
		ret = switchtec_fw_toggle_active_partition(cfg.dev,
							   cfg.firmware,
							   cfg.config);
	}

	print_fw_part_info(cfg.dev);
	printf("\n");

	switchtec_perror("firmware toggle");

	return ret;
}

static int fw_read(int argc, char **argv)
{
	const char *desc = "Flash the firmware with a new image";
	struct switchtec_fw_footer ftr;
	struct switchtec_fw_image_info act_img, inact_img, act_cfg, inact_cfg;
	int ret = 0;
	char version[16];
	unsigned long img_addr;
	size_t img_size;
	enum switchtec_fw_image_type type;

	static struct {
		struct switchtec_dev *dev;
		int out_fd;
		const char *out_filename;
		int inactive;
		int data;
	} cfg = {0};
	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"filename", .cfg_type=CFG_FD_WR, .value_addr=&cfg.out_fd,
		  .argument_type=optional_positional,
		  .force_default="image.pmc",
		  .help="image file to display information for"},
		{"inactive", 'i', "", CFG_NONE, &cfg.inactive, no_argument,
		 "read the inactive partition"},
		{"data", 'd', "", CFG_NONE, &cfg.data, no_argument,
		 "read the data/config partiton instead of the main firmware"},
		{"config", 'c', "", CFG_NONE, &cfg.data, no_argument,
		 "read the data/config partiton instead of the main firmware"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_fw_img_info(cfg.dev, &act_img, &inact_img);
	if (ret < 0) {
		switchtec_perror("fw_img_info");
		goto close_and_exit;
	}

	ret = switchtec_fw_cfg_info(cfg.dev, &act_cfg, &inact_cfg, NULL, NULL);
	if (ret < 0) {
		switchtec_perror("fw_cfg_info");
		goto close_and_exit;
	}

	if (cfg.data) {
		img_addr = cfg.inactive ? inact_cfg.image_addr :
			act_cfg.image_addr;
		img_size = cfg.inactive ? inact_cfg.image_len :
			act_cfg.image_len;;
		type = SWITCHTEC_FW_TYPE_DAT0;
	} else {
		img_addr = cfg.inactive ? inact_img.image_addr :
			act_img.image_addr;
		img_size = cfg.inactive ? inact_img.image_len :
			act_img.image_len;
		type = SWITCHTEC_FW_TYPE_IMG0;
	}

	ret = switchtec_fw_read_footer(cfg.dev, img_addr, img_size, &ftr,
				       version, sizeof(version));
	if (ret < 0) {
		switchtec_perror("fw_read_footer");
		goto close_and_exit;
	}

	fprintf(stderr, "Version:  %s\n", version);
	fprintf(stderr, "Type:     %s\n", cfg.data ? "DAT" : "IMG");
	fprintf(stderr, "Img Len:  0x%x\n", (int) ftr.image_len);
	fprintf(stderr, "CRC:      0x%x\n", (int) ftr.image_crc);

	ret = switchtec_fw_img_write_hdr(cfg.out_fd, &ftr, type);
	if (ret < 0) {
		switchtec_perror(cfg.out_filename);
		goto close_and_exit;
	}

	progress_start();
	ret = switchtec_fw_read_fd(cfg.dev, cfg.out_fd, img_addr,
				   ftr.image_len, progress_update);
	progress_finish();

	if (ret < 0)
		switchtec_perror("fw_read");

	fprintf(stderr, "\nFirmware read to %s.\n", cfg.out_filename);

close_and_exit:
	close(cfg.out_fd);

	return ret;
}

static void create_type_choices(struct argconfig_choice *c)
{
	const struct switchtec_evcntr_type_list *t;

	for (t = switchtec_evcntr_type_list; t->name; t++, c++) {
		c->name = t->name;
		c->value = t->mask;
		c->help = t->help;
	}

	c->name = NULL;
	c->value = 0;
	c->help = 0;
}

static char *type_mask_to_string(int type_mask, char *buf, size_t buflen)
{
	int w;
	char *ret = buf;

	while (type_mask) {
		const char *str = switchtec_evcntr_type_str(&type_mask);
		if (str == NULL)
			break;

		w = snprintf(buf, buflen, "%s,", str);
		buf += w;
		buflen -= w;
		if (buflen < 0)
			return ret;
	}

	buf[-1] = 0;

	return ret;
}

static char *port_mask_to_string(unsigned port_mask, char *buf, size_t buflen)
{
	int i, range=-1;
	int w;
	char *ret = buf;

	port_mask &= 0xFF;

	if (port_mask == 0xFF) {
		snprintf(buf, buflen, "ALL");
		return buf;
	}

	for (i = 0; port_mask; port_mask = port_mask >> 1, i++) {
		if (port_mask & 1 && range < 0) {
			w = snprintf(buf, buflen, "%d,", i);
			buf += w;
			buflen -=w;
			range = i;
		} else if (!(port_mask & 1)) {
			if (range >= 0 && range < i-1) {
				buf--;
				buflen++;
				w = snprintf(buf, buflen, "-%d,", i-1);
				buf += w;
				buflen -=w;
			}
			range = -1;
		}
	}

	if (range >= 0 && range < i-1) {
		buf--;
		buflen++;
		w = snprintf(buf, buflen, "-%d,", i-1);
		buf += w;
		buflen -=w;
	}


	buf[-1] = 0;

	return ret;
}


static int display_event_counters(struct switchtec_dev *dev, int stack,
				  int reset)
{
	int ret, i;
	struct switchtec_evcntr_setup setups[SWITCHTEC_MAX_EVENT_COUNTERS];
	unsigned counts[SWITCHTEC_MAX_EVENT_COUNTERS];
	char buf[1024];
	int count = 0;

	ret = switchtec_evcntr_get_both(dev, stack, 0, ARRAY_SIZE(setups),
					setups, counts, reset);

	if (ret < 0)
		return ret;

	printf("Stack %d:\n", stack);

	for (i = 0; i < ret; i ++) {
		if (!setups[i].port_mask || !setups[i].type_mask)
			continue;

		port_mask_to_string(setups[i].port_mask, buf, sizeof(buf));
		printf("   %2d - %-11s", i, buf);

		type_mask_to_string(setups[i].type_mask, buf, sizeof(buf));
		if (strlen(buf) > 39)
			strcpy(buf, "MANY");

		printf("%-40s   %10u\n", buf, counts[i]);
		count++;
	}

	if (!count)
		printf("  No event counters enabled.\n");

	return 0;
}

static int get_free_counter(struct switchtec_dev *dev, int stack)
{
	struct switchtec_evcntr_setup setups[SWITCHTEC_MAX_EVENT_COUNTERS];
	int ret;
	int i;

	ret = switchtec_evcntr_get_setup(dev, stack, 0, ARRAY_SIZE(setups),
					 setups);
	if (ret < 0) {
		switchtec_perror("evcntr_get_setup");
		return ret;
	}

	for (i = 0; i < ret; i ++) {
		if (!setups[i].port_mask || !setups[i].type_mask)
			return i;
	}

	errno = EUSERS;
	return -errno;
}

static void show_event_counter(int stack, int counter,
			       struct switchtec_evcntr_setup *setup)
{
	char buf[200];

	printf("Stack:     %d\n", stack);
	printf("Counter:   %d\n", counter);

	if (!setup->port_mask || !setup->type_mask) {
		printf("Not Configured.\n");
		return;
	}

	if (setup->threshold)
		printf("Threshold: %d\n", setup->threshold);
	printf("Ports:     %s\n", port_mask_to_string(setup->port_mask,
						      buf, sizeof(buf)));
	printf("Events:    %s\n", type_mask_to_string(setup->type_mask,
						      buf, sizeof(buf)));
	if (setup->type_mask & ALL_TLPS)
		printf("Direction: %s\n", setup->egress ?
		       "EGRESS" : "INGRESS");
}

static int evcntr_setup(int argc, char **argv)
{
	const char *desc = "Setup a new event counter";
	int nr_type_choices = switchtec_evcntr_type_count();
	struct argconfig_choice type_choices[nr_type_choices+1];
	int ret;

	static struct {
		struct switchtec_dev *dev;
		int stack;
		int counter;
		struct switchtec_evcntr_setup setup;
	} cfg = {
		.stack = -1,
		.counter = -1,
	};

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"stack", 's', "NUM", CFG_POSITIVE, &cfg.stack, required_argument,
		 "stack to create the counter in",
		 .require_in_usage=1},
		{"event", 'e', "EVENT", CFG_MULT_CHOICES, &cfg.setup.type_mask,
		  required_argument,
		 "event to count on, may specify this argument multiple times "
		 "to count on multiple events",
		 .choices=type_choices, .require_in_usage=1},

		{"counter", 'c', "NUM", CFG_POSITIVE, &cfg.counter, required_argument,
		 "counter index, default is to use the next unused index"},
		{"egress", 'g', "", CFG_NONE, &cfg.setup.egress, no_argument,
		 "measure egress TLPs instead of ingress -- only meaningful for "
		 "POSTED_TLP, COMP_TLP and NON_POSTED_TLP counts"},
		{"port_mask", 'p', "0xXX|#,#,#-#,#", CFG_MASK_8, &cfg.setup.port_mask,
		  required_argument,
		 "ports to capture events on, default is all ports"},
		{"thresh", 't', "NUM", CFG_POSITIVE, &cfg.setup.threshold,
		 required_argument,
		 "threshold to trigger an event notification"},
		{NULL}};

	create_type_choices(type_choices);
	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (cfg.stack < 0) {
		argconfig_print_usage(opts);
		fprintf(stderr, "The --stack argument is required!\n");
		return 1;
	}

	if (!cfg.setup.type_mask) {
		argconfig_print_usage(opts);
		fprintf(stderr, "Must specify at least one event!\n");
		return 1;
	}

	if (!cfg.setup.port_mask)
		cfg.setup.port_mask = -1;

	if (cfg.counter < 0) {
		cfg.counter = get_free_counter(cfg.dev, cfg.stack);
		if (cfg.counter < 0)
			return cfg.counter;
	}

	if (cfg.setup.threshold &&
	    __builtin_popcount(cfg.setup.port_mask) > 1 &&
	    __builtin_popcount(cfg.setup.type_mask) > 1)
	{
		fprintf(stderr, "A threshold can only be used with a counter "
			"that has a single port and single event\n");
		return 1;
	}

	show_event_counter(cfg.stack, cfg.counter, &cfg.setup);

	ret = switchtec_evcntr_setup(cfg.dev, cfg.stack, cfg.counter,
				     &cfg.setup);

	switchtec_perror("evcntr-setup");

	return ret;
}

static int evcntr(int argc, char **argv)
{
	const char *desc = "Display event counters";
	int ret, i;

	static struct {
		struct switchtec_dev *dev;
		int stack;
		int reset;
	} cfg = {
		.stack = -1,
	};

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"reset", 'r', "", CFG_NONE, &cfg.reset, no_argument,
		 "reset counters back to zero"},
		{"stack", 's', "NUM", CFG_POSITIVE, &cfg.stack, required_argument,
		 "stack to create the counter in"},
		{0}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (cfg.stack < 0) {
		for (i = 0; i < SWITCHTEC_MAX_STACKS; i++)
			display_event_counters(cfg.dev, i, cfg.reset);
		return 0;
	}

	ret = display_event_counters(cfg.dev, cfg.stack, cfg.reset);
	if (ret)
		switchtec_perror("display events");

	return ret;
}

static int evcntr_show(int argc, char **argv)
{
	const char *desc = "Display setup information for an event counter";
	struct switchtec_evcntr_setup setup;
	int ret;

	static struct {
		struct switchtec_dev *dev;
		int stack;
		int counter;
	} cfg = {
		.stack = -1,
		.counter = -1,
	};

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"stack", 's', "NUM", CFG_POSITIVE, &cfg.stack, required_argument,
		 "stack to create the counter in",
		 .require_in_usage=1},
		{"counter", 'c', "NUM", CFG_POSITIVE, &cfg.counter, required_argument,
		 "counter index, default is to use the next unused index",
		 .require_in_usage=1},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (cfg.stack < 0) {
		argconfig_print_usage(opts);
		fprintf(stderr, "The --stack argument is required!\n");
		return 1;
	}

	if (cfg.counter < 0) {
		argconfig_print_usage(opts);
		fprintf(stderr, "The --counter argument is required!\n");
		return 1;
	}

	ret = switchtec_evcntr_get_setup(cfg.dev, cfg.stack, cfg.counter, 1,
					 &setup);
	if (ret < 0) {
		switchtec_perror("evcntr_show");
		return ret;
	}

	show_event_counter(cfg.stack, cfg.counter, &setup);

	return 0;
}

static int evcntr_del(int argc, char **argv)
{
	const char *desc = "Deconfigure an event counter counter";
	struct switchtec_evcntr_setup setup = {};
	int ret;

	static struct {
		struct switchtec_dev *dev;
		int stack;
		int counter;
	} cfg = {
		.stack = -1,
		.counter = -1,
	};

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"stack", 's', "NUM", CFG_POSITIVE, &cfg.stack, required_argument,
		 "stack to create the counter in",
		 .require_in_usage=1},
		{"counter", 'c', "NUM", CFG_POSITIVE, &cfg.counter, required_argument,
		 "counter index, default is to use the next unused index",
		 .require_in_usage=1},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	if (cfg.stack < 0) {
		argconfig_print_usage(opts);
		fprintf(stderr, "The --stack argument is required!\n");
		return 1;
	}

	if (cfg.counter < 0) {
		argconfig_print_usage(opts);
		fprintf(stderr, "The --counter argument is required!\n");
		return 1;
	}

	ret = switchtec_evcntr_setup(cfg.dev, cfg.stack, cfg.counter, &setup);
	if (ret < 0) {
		switchtec_perror("evcntr_del");
		return ret;
	}


	return 0;
}

static int evcntr_wait(int argc, char **argv)
{
	const char *desc = "Wait for an event counter to reach its threshold";
	int ret, i;

	static struct {
		struct switchtec_dev *dev;
		int timeout;
	} cfg = {
		.timeout = -1,
	};

	const struct argconfig_options opts[] = {
		DEVICE_OPTION,
		{"timeout", 't', "MS", CFG_INT, &cfg.timeout, required_argument,
		 "timeout in milliseconds"},
		{NULL}};

	argconfig_parse(argc, argv, desc, opts, &cfg, sizeof(cfg));

	ret = switchtec_evcntr_wait(cfg.dev, cfg.timeout);
	if (ret < 0) {
		perror("evcntr_wait");
		return -1;
	}

	if (!ret) {
		fprintf(stderr, "timeout\n");
		return 1;
	}

	for (i = 0; i < SWITCHTEC_MAX_STACKS; i++)
		display_event_counters(cfg.dev, i, 0);

	return 0;
}

static const struct cmd commands[] = {
	CMD(list, "List all switchtec devices on this machine"),
	CMD(gui, "Display a simple ncurses GUI for the switch"),
	CMD(status, "Display status information"),
	CMD(bw, "Measure the bandwidth for each port"),
	CMD(latency, "Measure the latency of a port"),
	CMD(events, "Display events that have occurred"),
	CMD(event_wait, "Wait for an event to occur"),
	CMD(log_dump, "Dump firmware log to a file"),
	CMD(test, "Test if switchtec interface is working"),
	CMD(temp, "Return the switchtec die temperature"),
	CMD(hard_reset, "Perform a hard reset of the switch"),
	CMD(fw_update, "Upload a new firmware image"),
	CMD(fw_info, "Return information on currently flashed firmware"),
	CMD(fw_toggle, "Toggle the active and inactive firmware partition"),
	CMD(fw_read, "Read back firmware image from hardware"),
	CMD(fw_img_info, "Display information for a firmware image"),
	CMD(evcntr, "Display event counters"),
	CMD(evcntr_setup, "Setup an event counter"),
	CMD(evcntr_show, "Show an event counters setup info"),
	CMD(evcntr_del, "Deconfigure an event counter"),
	CMD(evcntr_wait, "Wait for an event counter to exceed its threshold"),
	{},
};

static struct subcommand subcmd = {
	.cmds = commands,
};

REGISTER_SUBCMD(subcmd);

static struct prog_info prog_info = {
	.usage = "<command> [<device>] [OPTIONS]",
	.desc = "The <device> must be a switchtec device "
		"(ex: /dev/switchtec0)",
};

int main(int argc, char **argv)
{
	int ret;

	ret = commands_handle(argc, argv, &prog_info);

	switchtec_close(global_dev);

	return ret;
}
