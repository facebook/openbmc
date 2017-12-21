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
#include "switchtec/utils.h"

#include <linux/switchtec_ioctl.h>

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <errno.h>
#include <string.h>
#include <strings.h>

#define EV(t, n, s, d)[SWITCHTEC_ ## t ## _EVT_ ## n] = {\
	.type = t, \
	.summary_bit = (1 << (s)), \
	.ioctl_id = SWITCHTEC_IOCTL_EVENT_ ## n, \
	.short_name = #n, \
	.desc = d, \
}

static const struct {
	enum switchtec_event_id id;
	enum {
		GLOBAL = SWITCHTEC_EVT_GLOBAL,
		PART = SWITCHTEC_EVT_PART,
		PFF = SWITCHTEC_EVT_PFF,
	} type;
	uint64_t summary_bit;
	int ioctl_id;
	const char *short_name;
	const char *desc;
} events[] = {
	EV(GLOBAL, STACK_ERROR, 0, "Stack Error"),
	EV(GLOBAL, PPU_ERROR, 1, "PPU Error"),
	EV(GLOBAL, ISP_ERROR, 2, "ISP Error"),
	EV(GLOBAL, SYS_RESET, 3, "System Reset"),
	EV(GLOBAL, FW_EXC, 4, "Firmware Exception"),
	EV(GLOBAL, FW_NMI, 5, "Firmware Non-Maskable Interrupt"),
	EV(GLOBAL, FW_NON_FATAL, 6, "Firmware Non-Fatal Error"),
	EV(GLOBAL, FW_FATAL, 7, "Firmware Fatal Error"),
	EV(GLOBAL, TWI_MRPC_COMP, 8, "TWI MRPC Completion"),
	EV(GLOBAL, TWI_MRPC_COMP_ASYNC, 9, "TWI MRPC Async Completion"),
	EV(GLOBAL, CLI_MRPC_COMP, 10, "CLI MRPC Completion"),
	EV(GLOBAL, CLI_MRPC_COMP_ASYNC, 11, "CLI MRPC Async Completion"),
	EV(GLOBAL, GPIO_INT, 12, "GPIO Interrupt"),
	EV(GLOBAL, GFMS, 13, "Global Fabric Management Server Event"),
	EV(PART, PART_RESET, 0, "Partition Reset"),
	EV(PART, MRPC_COMP, 1, "MRPC Completion"),
	EV(PART, MRPC_COMP_ASYNC, 2, "MRPC Async Completion"),
	EV(PART, DYN_PART_BIND_COMP, 3,
	   "Dynamic Partition Binding Completion"),
	EV(PFF, AER_IN_P2P, 0, "Advanced Error Reporting in P2P Port"),
	EV(PFF, AER_IN_VEP, 1, "Advancde Error Reporting in vEP"),
	EV(PFF, DPC, 2, "Downstream Port Containment Event"),
	EV(PFF, CTS, 3, "Completion Timeout Synthesis Event"),
	EV(PFF, HOTPLUG, 5, "Hotplug Event"),
	EV(PFF, IER, 6, "Internal Error Reporting Event"),
	EV(PFF, THRESH, 7, "Event Counter Threshold Reached"),
	EV(PFF, POWER_MGMT, 8, "Power Management Event"),
	EV(PFF, TLP_THROTTLING, 9, "TLP Throttling Event"),
	EV(PFF, FORCE_SPEED, 10, "Force Speed Error"),
	EV(PFF, CREDIT_TIMEOUT, 11, "Credit Timeout"),
	EV(PFF, LINK_STATE, 12, "Link State Change Event"),
};

#define EVBIT(t, n, b)[b] = SWITCHTEC_ ## t ## _EVT_ ## n
static const enum switchtec_event_id global_event_bits[64] = {
	[0 ... 63] = -1,
	EVBIT(GLOBAL, STACK_ERROR, 0),
	EVBIT(GLOBAL, PPU_ERROR, 1),
	EVBIT(GLOBAL, ISP_ERROR, 2),
	EVBIT(GLOBAL, SYS_RESET, 3),
	EVBIT(GLOBAL, FW_EXC, 4),
	EVBIT(GLOBAL, FW_NMI, 5),
	EVBIT(GLOBAL, FW_NON_FATAL, 6),
	EVBIT(GLOBAL, FW_FATAL, 7),
	EVBIT(GLOBAL, TWI_MRPC_COMP, 8),
	EVBIT(GLOBAL, TWI_MRPC_COMP_ASYNC, 9),
	EVBIT(GLOBAL, CLI_MRPC_COMP, 10),
	EVBIT(GLOBAL, CLI_MRPC_COMP_ASYNC, 11),
	EVBIT(GLOBAL, GPIO_INT, 12),
};

static const enum switchtec_event_id part_event_bits[64] = {
	[0 ... 63] = -1,
	EVBIT(PART, PART_RESET, 0),
	EVBIT(PART, MRPC_COMP, 1),
	EVBIT(PART, MRPC_COMP_ASYNC, 2),
	EVBIT(PART, DYN_PART_BIND_COMP, 3),
};

static const enum switchtec_event_id pff_event_bits[64] = {
	[0 ... 63] = -1,
	EVBIT(PFF, AER_IN_P2P, 0),
	EVBIT(PFF, AER_IN_VEP, 1),
	EVBIT(PFF, DPC, 2),
	EVBIT(PFF, CTS, 3),
	EVBIT(PFF, HOTPLUG, 5),
	EVBIT(PFF, IER, 6),
	EVBIT(PFF, THRESH, 7),
	EVBIT(PFF, POWER_MGMT, 8),
	EVBIT(PFF, TLP_THROTTLING, 9),
	EVBIT(PFF, FORCE_SPEED, 10),
	EVBIT(PFF, CREDIT_TIMEOUT, 11),
	EVBIT(PFF, LINK_STATE, 12),
};

int switchtec_event_wait(struct switchtec_dev *dev, int timeout_ms)
{
	int ret;
	struct pollfd fds = {
		.fd = dev->fd,
		.events = POLLPRI,
	};

	ret = poll(&fds, 1, timeout_ms);
	if (ret <= 0)
		return ret;

	if (fds.revents & POLLERR) {
		errno = ENODEV;
		return -1;
	}

	if (fds.revents & POLLPRI)
		return 1;

	return 0;
}

static void set_all_parts(struct switchtec_event_summary *sum, uint64_t bit)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sum->part); i++)
		sum->part[i] |= bit;
}

static void set_all_pffs(struct switchtec_event_summary *sum, uint64_t bit)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sum->pff); i++)
		sum->pff[i] |= bit;
}

int switchtec_event_summary_set(struct switchtec_event_summary *sum,
				enum switchtec_event_id e,
				int index)
{
	uint64_t bit = events[e].summary_bit;

	switch (events[e].type) {
	case GLOBAL:
		sum->global &= bit;
		break;
	case PART:
		if (index == SWITCHTEC_EVT_IDX_LOCAL) {
			sum->local_part |= bit;
		} else if (index == SWITCHTEC_EVT_IDX_ALL) {
			set_all_parts(sum, bit);
		} else if (index < 0 || index >= ARRAY_SIZE(sum->part)) {
			errno = EINVAL;
			return -EINVAL;
		} else {
			sum->part[index] |= bit;
		}
		break;
	case PFF:
		if (index == SWITCHTEC_EVT_IDX_ALL) {
			set_all_pffs(sum, bit);
		} else if (index < 0 || index >= ARRAY_SIZE(sum->pff)) {
			errno = EINVAL;
			return -EINVAL;
		} else {
			sum->pff[index] |= bit;
		}
		break;
	}

	return 0;
}

int switchtec_event_summary_test(struct switchtec_event_summary *sum,
				 enum switchtec_event_id e,
				 int index)
{
	uint64_t bit = events[e].summary_bit;

	switch (events[e].type) {
	case GLOBAL:
		return sum->global & bit;
	case PART:
		return sum->part[index] & bit;
	case PFF:
		return sum->pff[index] & bit;
	}

	return 0;
}

int switchtec_event_summary_iter(struct switchtec_event_summary *sum,
				 enum switchtec_event_id *e,
				 int *idx)
{
	int bit;

	if (!idx || !e)
		return -EINVAL;

	*idx = 0;

	bit = ffs(sum->global) - 1;
	if (bit >= 0) {
		*e = global_event_bits[bit];
		sum->global &= ~(1 << bit);
		return 1;
	}

	for (*idx = 0; *idx < ARRAY_SIZE(sum->part); (*idx)++) {
		bit = ffs(sum->part[*idx]) - 1;
		if (bit < 0)
			continue;

		*e = part_event_bits[bit];
		sum->part[*idx] &= ~(1 << bit);
		return 1;
	}

	for (*idx = 0; *idx < ARRAY_SIZE(sum->pff); (*idx)++) {
		bit = ffs(sum->pff[*idx]) - 1;
		if (bit < 0)
			continue;

		*e = pff_event_bits[bit];
		sum->pff[*idx] &= ~(1 << bit);
		return 1;
	}

	return 0;
}

enum switchtec_event_type switchtec_event_info(enum switchtec_event_id e,
					       const char **name,
					       const char **desc)
{
	if (name)
		*name = events[e].short_name;

	if (desc)
		*desc = events[e].desc;

	return events[e].type;
}

static void event_summary_copy(struct switchtec_event_summary *dst,
			       struct switchtec_ioctl_event_summary *src)
{
	int i;

	dst->global = src->global;
	dst->part_bitmap = src->part_bitmap;
	dst->local_part = src->local_part;

	for (i = 0; i < SWITCHTEC_MAX_PARTS; i++)
		dst->part[i] = src->part[i];

	for (i = 0; i < SWITCHTEC_MAX_PORTS; i++)
		dst->pff[i] = src->pff[i];
}

int switchtec_event_summary(struct switchtec_dev *dev,
			    struct switchtec_event_summary *sum)
{
	int ret;
	struct switchtec_ioctl_event_summary isum;

	if (!sum)
		return -EINVAL;

	ret = ioctl(dev->fd, SWITCHTEC_IOCTL_EVENT_SUMMARY, &isum);
	if (ret < 0)
		return ret;

	event_summary_copy(sum, &isum);

	return 0;
}

int switchtec_event_check(struct switchtec_dev *dev,
			  struct switchtec_event_summary *check,
			  struct switchtec_event_summary *res)
{
	int ret, i;
	struct switchtec_ioctl_event_summary isum;

	if (!check)
		return -EINVAL;

	ret = ioctl(dev->fd, SWITCHTEC_IOCTL_EVENT_SUMMARY, &isum);
	if (ret < 0)
		return ret;

	ret = 0;

	if (isum.global & check->global)
		ret = 1;

	if (isum.part_bitmap & check->part_bitmap)
		ret = 1;

	if (isum.local_part & check->local_part)
		ret = 1;

	for (i = 0; i < SWITCHTEC_MAX_PARTS; i++)
		if (isum.part[i] & check->part[i])
			ret = 1;

	for (i = 0; i < SWITCHTEC_MAX_PORTS; i++)
		if (isum.pff[i] & check->pff[i])
			ret = 1;

	if (res)
		event_summary_copy(res, &isum);

	return ret;
}

int switchtec_event_wait_for(struct switchtec_dev *dev,
			     enum switchtec_event_id e, int index,
			     struct switchtec_event_summary *res,
			     int timeout_ms)
{
	struct timeval tv;
	long long start, now;
	struct switchtec_event_summary wait_for = {0};
	int ret;

	ret = switchtec_event_summary_set(&wait_for, e, index);
	if (ret)
		return ret;

	ret = switchtec_event_ctl(dev, e, index,
				  SWITCHTEC_EVT_FLAG_CLEAR |
				  SWITCHTEC_EVT_FLAG_EN_POLL,
				  NULL);
	if (ret)
		return ret;

	ret = gettimeofday(&tv, NULL);
	if (ret)
		return ret;

	now = start = ((tv.tv_sec) * 1000 + tv.tv_usec / 1000);

	while (1) {
		ret = switchtec_event_wait(dev, timeout_ms > 0 ?
					   now - start + timeout_ms : -1);
		if (ret < 0)
			return ret;

		if (ret == 0)
			goto next;

		ret = switchtec_event_check(dev, &wait_for, res);
		if (ret < 0)
			return ret;

		if (ret)
			return 1;

next:
		ret = gettimeofday(&tv, NULL);
		if (ret)
			return ret;

		now = ((tv.tv_sec) * 1000 + tv.tv_usec / 1000);

		if (timeout_ms > 0 && now - start >= timeout_ms)
			return 0;
	}
}

int switchtec_event_ctl(struct switchtec_dev *dev,
			enum switchtec_event_id e,
			int index, int flags,
			uint32_t data[5])
{
	int ret;
	struct switchtec_ioctl_event_ctl ctl;

	if (e > SWITCHTEC_MAX_EVENTS)
		return -EINVAL;

	ctl.event_id = events[e].ioctl_id;
	ctl.flags = 0;

	if (flags & SWITCHTEC_EVT_FLAG_CLEAR)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_CLEAR;
	if (flags & SWITCHTEC_EVT_FLAG_EN_POLL)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_EN_POLL;
	if (flags & SWITCHTEC_EVT_FLAG_EN_LOG)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_EN_LOG;
	if (flags & SWITCHTEC_EVT_FLAG_EN_CLI)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_EN_CLI;
	if (flags & SWITCHTEC_EVT_FLAG_EN_FATAL)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_EN_FATAL;
	if (flags & SWITCHTEC_EVT_FLAG_DIS_POLL)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_DIS_POLL;
	if (flags & SWITCHTEC_EVT_FLAG_DIS_LOG)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_DIS_LOG;
	if (flags & SWITCHTEC_EVT_FLAG_DIS_CLI)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_DIS_CLI;
	if (flags & SWITCHTEC_EVT_FLAG_DIS_FATAL)
		ctl.flags |= SWITCHTEC_IOCTL_EVENT_FLAG_DIS_FATAL;

	ctl.index = index;
	ret = ioctl(dev->fd, SWITCHTEC_IOCTL_EVENT_CTL, &ctl);

	if (ret)
		return ret;

	if (data)
		memcpy(data, ctl.data, sizeof(ctl.data));

	return ctl.count;
}
