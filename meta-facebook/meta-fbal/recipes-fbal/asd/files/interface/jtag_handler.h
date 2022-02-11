/*
Copyright (c) 2019, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _JTAG_HANDLER_H_
#define _JTAG_HANDLER_H_

#include <stdbool.h>

#include "SoftwareJTAGHandler.h"

typedef uint8_t __u8;
typedef uint32_t __u32;

// contains common ioctl command numbers for accessing jtag driver and structs
// jtag_states enum is defined here
//#define JTAG_LEGACY_DRIVER
#ifdef JTAG_LEGACY_DRIVER
#include "jtag_drv.h"
#else
typedef unsigned long long __u64;
#include "jtag.h"
#endif

#ifndef JTAG_LEGACY_DRIVER
#define jtag_states jtag_tapstate
#define jtag_tlr JTAG_STATE_TLRESET
#define jtag_rti JTAG_STATE_IDLE
#define jtag_sel_dr JTAG_STATE_SELECTDR
#define jtag_cap_dr JTAG_STATE_CAPTUREDR
#define jtag_shf_dr JTAG_STATE_SHIFTDR
#define jtag_ex1_dr JTAG_STATE_EXIT1DR
#define jtag_pau_dr JTAG_STATE_PAUSEDR
#define jtag_ex2_dr JTAG_STATE_EXIT2DR
#define jtag_upd_dr JTAG_STATE_UPDATEDR
#define jtag_sel_ir JTAG_STATE_SELECTIR
#define jtag_cap_ir JTAG_STATE_CAPTUREIR
#define jtag_shf_ir JTAG_STATE_SHIFTIR
#define jtag_ex1_ir JTAG_STATE_EXIT1IR
#define jtag_pau_ir JTAG_STATE_PAUSEIR
#define jtag_ex2_ir JTAG_STATE_EXIT2IR
#define jtag_upd_ir JTAG_STATE_UPDATEIR
#define SW_MODE JTAG_XFER_SW_MODE
#define HW_MODE JTAG_XFER_HW_MODE
#define AST_JTAG_BITBANG JTAG_IOCBITBANG
#define AST_JTAG_SET_TAPSTATE JTAG_SIOCSTATE
#define AST_JTAG_READWRITESCAN JTAG_IOCXFER
#define AST_JTAG_SET_TCK JTAG_SIOCFREQ
#define controller_mode_param jtag_mode
#define tap_state_param jtag_tap_state
#endif

#define DRMAXPADSIZE 250
#define IRMAXPADSIZE 2000
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define BITS_PER_BYTE 8
#ifndef APB_FREQ
#define APB_FREQ 24740000
#endif


#endif // _JTAG_HANDLER_H_
