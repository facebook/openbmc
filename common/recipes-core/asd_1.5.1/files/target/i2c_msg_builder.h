/*
Copyright (c) 2018, Intel Corporation

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

#ifndef ASD_I2C_MSG_BUILDER_H
#define ASD_I2C_MSG_BUILDER_H

#include <linux/i2c.h>

#include "asd_common.h"

typedef struct I2C_Msg_Builder
{
    void* msg_set;
} I2C_Msg_Builder;

I2C_Msg_Builder* I2CMsgBuilder();
STATUS i2c_msg_initialize(I2C_Msg_Builder* state);
STATUS i2c_msg_deinitialize(I2C_Msg_Builder* state);
STATUS i2c_msg_add(I2C_Msg_Builder* state, asd_i2c_msg* msg);
STATUS i2c_msg_get_count(I2C_Msg_Builder* state, uint32_t* count);
STATUS i2c_msg_get_asd_i2c_msg(I2C_Msg_Builder* state, uint32_t index,
                               asd_i2c_msg* msg);
STATUS i2c_msg_reset(I2C_Msg_Builder* state);

#endif // ASD_I2C_MSG_BUILDER_H
