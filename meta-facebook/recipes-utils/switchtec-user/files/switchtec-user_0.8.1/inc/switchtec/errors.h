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

#ifndef LIBSWITCHTEC_ERRORS_H
#define LIBSWITCHTEC_ERRORS_H


enum {
	ERR_NO_AVAIL_MRPC_THREAD = 0x64001,
	ERR_HANDLER_THREAD_NOT_IDLE = 0x64002,
	ERR_NO_BG_THREAD = 0x64003,
	ERR_SUBCMD_INVALID = 0x64004,
	ERR_CMD_INVALID = 0x64005,
	ERR_PARAM_INVALID = 0x64006,
	ERR_BAD_FW_STATE = 0x64007,

	ERR_STACK_INVALID = 0x100001,
	ERR_PORT_INVALID = 0x100002,
	ERR_EVENT_INVALID = 0x100003,
	ERR_RST_RULE_FAILED = 0x100005,
	ERR_ACCESS_REFUSED = 0xFFFF0001,
};

#endif
