/*
Copyright (c) 2017, Intel Corporation
Copyright (c) 2017, Facebook Inc.
 
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

/* Stub functions for Pin handling
   Each platform must override this file depending on the access to 
   the CPU Pins */

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "SoftwareJTAGHandler.h"
#include "pin_interface.h"

int pin_initialize(const int fru)
{
  return ST_ERR;
}
int pin_deinitialize(const int fru)
{
  return ST_ERR;
}

int power_debug_assert(const int fru, const bool assert)
{
  return ST_ERR;
}
int power_debug_is_asserted(const int fru, bool* asserted)
{
  return ST_ERR;
}

int preq_assert(const int fru, const bool assert)
{
  return ST_ERR;
}
int preq_is_asserted(const int fru, bool* asserted)
{
  return ST_ERR;
}

int prdy_is_event_triggered(const int fru, bool* triggered)
{
  return ST_ERR;
}
int prdy_is_asserted(const int fru, bool* asserted)
{
  return ST_ERR;
}

int platform_reset_is_event_triggered(const int fru, bool* triggered)
{
  return ST_ERR;
}
int platform_reset_is_asserted(const int fru, bool* asserted)
{
  return ST_ERR;
}

int xdp_present_is_event_triggered(const int fru, bool* triggered)
{
  return ST_ERR;
}

int xdp_present_is_asserted(const int fru, bool* asserted)
{
  return ST_ERR;
}

int tck_mux_select_assert(const int fru, bool assert)
{
  return ST_OK;
}
