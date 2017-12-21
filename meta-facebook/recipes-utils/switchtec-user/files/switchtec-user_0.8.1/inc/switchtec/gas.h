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
#ifndef LIBSWITCHTEC_GAS_H
#define LIBSWITCHTEC_GAS_H

/* MRPC Region  */
enum {
    SWITCHTC_GAS_MRPC_INPUT_DATA_OFFSET = 0x0,
    SWITCHTC_GAS_MRPC_OUTPUT_DATA_OFFSET = 0x0400,
    SWITCHTC_GAS_MRPC_COMMAND_OFFSET = 0x0800,
    SWITCHTC_GAS_MRPC_STATUS_OFFSET = 0x0804,
    SWITCHTC_GAS_MRPC_COMMAND_RETURN_VALUE_OFFSET = 0x0808,
};

int switchtec_twi_gas_get_result(struct switchtec_dev *dev, uint32_t *result);
int switchtec_twi_gas_write_28(struct switchtec_dev *dev, uint32_t offset,
			 const void *data, size_t data_len); 
int switchtec_twi_gas_read_1024(struct switchtec_dev *dev, uint32_t offset,
			 void *data, size_t data_len); 
int switchtec_inband_gas_read(struct switchtec_dev *dev, uint32_t offset,
			 void *data, size_t data_len); 
int switchtec_inband_gas_write(struct switchtec_dev *dev, uint32_t offset,
			 const void *data, size_t data_len); 
int switchtec_twi_gas_read(struct switchtec_dev *dev, uint32_t offset,
			 void *data, size_t data_len); 
int switchtec_twi_gas_write(struct switchtec_dev *dev, uint32_t offset,
			 const void *data, size_t data_len); 
#endif
