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

#ifndef LIBSWITCHTEC_LOG_H
#define LIBSWITCHTEC_LOG_H

#include "mrpc.h"
#include <stdint.h>
#include <stddef.h>

struct log_a_retr {
	uint8_t sub_cmd_id;
	uint8_t from_end;
	uint8_t reserved1[6];
	uint32_t count;
	uint32_t reserved2;
	uint32_t start;
};

struct log_a_data {
	uint32_t data[8];
};

struct log_a_retr_result {
	struct log_a_retr_hdr {
		uint8_t sub_cmd_id;
		uint8_t from_end;
		uint8_t reserved1[2];
		uint32_t total;
		uint32_t count;
		uint32_t remain;
		uint32_t next_start;
		uint32_t reserved2[3];
	} hdr;

	struct log_a_data data[(MRPC_MAX_DATA_LEN -
				sizeof(struct log_a_retr_hdr)) /
			       sizeof(struct log_a_data)];
};

struct log_b_retr {
	uint8_t sub_cmd_id;
	uint8_t reserved[3];
	uint32_t offset;
	uint32_t length;
};

struct log_b_retr_result {
	struct log_b_retr_hdr {
		uint8_t sub_cmd_id;
		uint8_t reserved[3];
		uint32_t length;
		uint32_t remain;
	} hdr;
	uint8_t data[MRPC_MAX_DATA_LEN - sizeof(struct log_b_retr_hdr)];
};

#endif
