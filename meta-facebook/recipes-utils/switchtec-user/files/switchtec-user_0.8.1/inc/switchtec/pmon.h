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

#ifndef LIBSWITCHTEC_PMON_H
#define LIBSWITCHTEC_PMON_H

#include <stdint.h>
#include <switchtec/switchtec.h>

struct pmon_event_counter_setup {
	uint8_t sub_cmd_id;
	uint8_t stack_id;
	uint8_t counter_id;
	uint8_t num_counters;

	struct {
		uint8_t  port_mask;
		uint32_t type_mask:24;
		uint8_t  ieg;
		uint32_t thresh;
	} __attribute__(( packed )) counters[63];
};

struct pmon_event_counter_get_setup_result {
	uint8_t  port_mask;
	uint32_t type_mask:24;
	uint8_t  ieg;
	uint32_t thresh;
} __attribute__(( packed ));

struct pmon_event_counter_get {
	uint8_t sub_cmd_id;
	uint8_t stack_id;
	uint8_t counter_id;
	uint8_t num_counters;
	uint8_t read_clear;
};

struct pmon_event_counter_result {
	uint32_t value;
	uint32_t threshold;
};

struct pmon_bw_get {
	uint8_t sub_cmd_id;
	uint8_t count;
	struct {
		uint8_t id;
		uint8_t clear;
	} ports[SWITCHTEC_MAX_PORTS];
};

struct pmon_lat_setup {
	uint8_t sub_cmd_id;
	uint8_t count;
	struct {
		uint8_t egress;
		uint8_t ingress;
	} ports[SWITCHTEC_MAX_PORTS];
};

struct pmon_lat_get {
	uint8_t sub_cmd_id;
	uint8_t count;
	uint8_t clear;
	uint8_t port_ids[SWITCHTEC_MAX_PORTS];
};

struct pmon_lat_data {
	uint16_t cur_ns;
	uint16_t max_ns;
};

#endif
