/**
 *  @brief      Declares global definitions for the Linux platform
 *  @details
 *  @file       Globals_Linux.h
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

// Common include files
#include "Globals.h"

/// @cond SHOW_SAL_DEFINITIONS
// SAL annotations don't exist on Linux
#define _Check_return_
#define _Success_(x)

#define _In_
#define _In_opt_
#define _In_bytecount_(x)
#define _In_opt_bytecount_(x)
#define _In_z_
#define _In_opt_z_
#define _In_z_count_(x)
#define _In_opt_z_count_(x)
#define _In_reads_z_(x)
#define _In_reads_or_z_(x)
#define _In_reads_bytes_(x)
#define _In_reads_bytes_opt_(x)

#define _Inout_
#define _Inout_bytecap_(x)
#define _Inout_opt_
#define _Inout_updates_z_(x)
#define _Inout_z_
#define _Inout_z_cap_(x)
#define _Inout_opt_z_cap_(x)

#define _Out_
#define _Out_bytecap_(x)
#define _Out_bytecapcount_(x)
#define _Out_opt_bytecap_(x)
#define _Out_opt_bytecapcount_(x)
#define _Out_writes_bytes_all_(x)
#define _Out_writes_bytes_to_opt_(x, y)
#define _Outptr_result_buffer_(x)
#define _Outptr_result_maybenull_
#define _Outptr_result_maybenull_z_
#define _Out_z_cap_(x)
#define _Out_z_bytecap_(x)
#define _Out_writes_z_(x)
/// @endcond

/// Definition of type uint32_t
typedef unsigned int uint32_t;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

#ifndef MAXBYTE
/// Maximum numerical value of a byte
#define MAXBYTE 0xFF
#endif

/// Calling convention
#define IFXAPI
