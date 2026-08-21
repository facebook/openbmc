/*
 * University of Illinois/NCSA Open Source License
 *
 * Copyright (c) 2020, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Developed by:
 *
 *		AMD Research and AMD Software Development
 *
 *		Advanced Micro Devices, Inc.
 *
 *		www.amd.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in
 *    the documentation and/or other materials provided with the distribution.
 *  - Neither the names of <Name of Development Group, Name of Institution>,
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this Software without specific prior written
 *    permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS WITH THE SOFTWARE.
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <esmi_oob/apml_common.h>
#include <esmi_oob/esmi_cpuid_msr.h>
#include <esmi_oob/esmi_rmi.h>
#include <esmi_oob/apml.h>

/* REVISION 0x10 */
/* Thread enable status registers */
const uint8_t thread_en_reg_v10[] = {0x4, 0x5, 0x8, 0x9,
				     0xA, 0xB, 0xC, 0xD,
				     0x43, 0x44, 0x45, 0x46,
				     0x47, 0x48, 0x49, 0x4A};

/* REVISION 0x20 */
/* Thread enable status registers */
const uint8_t thread_en_reg_v20[] = {0x4, 0x5, 0x8, 0x9,
				     0xA, 0xB, 0xC, 0xD,
				     0x43, 0x44, 0x45, 0x46,
				     0x47, 0x48, 0x49, 0x4A,
				     0x91, 0x92, 0x93, 0x94,
				     0x95, 0x96, 0x97, 0x98,
				     0xD8, 0xD9, 0xDA, 0xDB,
				     0xDC, 0xDD, 0xDE, 0xDF};

/* REVISION 0x21 Dense platform */
/* Thread enable status registers for Dense platform */
/* Alert status registers for revision 0x21 Dense platform */
const uint16_t thread_en_reg_v21_dense[] = {0x4, 0x5, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD,
					   0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
					   0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
					   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
					   0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107,
					   0x108, 0x109, 0x10A, 0x10B, 0x10C, 0x10D, 0x10E, 0x10F};

const uint8_t alert_status[] = {0x10, 0x11, 0x12, 0x13,
				0x14, 0x15, 0x16, 0x17,
				0x18, 0x19, 0x1A, 0x1B,
				0x1C, 0x1D, 0x1E, 0x1F,
				0x50, 0x51, 0x52, 0x53,
				0x54, 0x55, 0x56, 0x57,
				0x58, 0x59, 0x5A, 0x5B,
				0x5C, 0x5D, 0x5E, 0x5F};

/* Alert status Registers for the V21 Revision Dense Platform */
const uint16_t alert_status_v21_dense[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
					   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
					   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
					   0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
					   0x220, 0x221, 0x222, 0x223, 0x224, 0x225, 0x226, 0x227,
					   0x228, 0x229, 0x22A, 0x22B, 0x22C, 0x22D, 0x22E, 0x22F};


/* Alert Mask registers */
const uint8_t alert_mask[] = {0x20, 0x21, 0x22, 0x23,
			      0x24, 0x25, 0x26, 0x27,
			      0x28, 0x29, 0x2A, 0x2B,
			      0x2C, 0x2D, 0x2E, 0x2F,
			      0xC0, 0xC1, 0xC2, 0xC3,
			      0xC4, 0xC5, 0xC6, 0xC7,
			      0xC8, 0xC9, 0xCA, 0xCB,
			      0xCC, 0xCD, 0xCE, 0xCF};

/* Alert Mask registers  for the V21 Revision Dense Platform */
const uint16_t alert_mask_v21_dense[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
					0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
					0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
					0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
					0x1C0, 0x1C1, 0x1C2, 0x1C3, 0x1C4, 0x1C5, 0x1C6, 0x1C7,
					0x1C8, 0x1C9, 0x1CA, 0x1CB, 0x1CC, 0x1CD, 0x1CE, 0x1CF};

/* sb-rmi register access */
oob_status_t read_sbrmi_revision(uint8_t soc_die_num,
				 uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_REVISION, buffer);
}

oob_status_t read_sbrmi_control(uint8_t soc_die_num,
				uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_CONTROL, buffer);
}

oob_status_t read_sbrmi_status(uint8_t soc_die_num,
			       uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_STATUS, buffer);
}

oob_status_t read_sbrmi_readsize(uint8_t soc_die_num,
				 uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_READSIZE, buffer);
}

oob_status_t read_sbrmi_threadenablestatus(uint8_t soc_die_num,
					   uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_THREADENABLESTATUS0,
				      buffer);
}

oob_status_t read_sbrmi_multithreadenablestatus(uint8_t soc_die_num,
						uint8_t *buffer)
{
	struct processor_info plat_info = {0};
	oob_status_t ret;
	int i;
	uint8_t rev;

	if (!buffer)
		return OOB_ARG_PTR_NULL;

	ret = read_sbrmi_revision(soc_die_num, &rev);
	if (ret)
		return ret;
	if (rev == 0x10) {
		for (i = 0; i < sizeof(thread_en_reg_v10); i++) {
			ret = esmi_oob_rmi_read_byte(soc_die_num, thread_en_reg_v10[i],
						     &buffer[i]);
			if (ret)
				return ret;
		}
	} else if (rev == 0x20) {
		for (i = 0; i < sizeof(thread_en_reg_v20); i++) {
			ret = esmi_oob_rmi_read_byte(soc_die_num, thread_en_reg_v20[i],
						     &buffer[i]);
			if (ret)
				return ret;
		}
	} else if (rev == 0x21) {
		ret = esmi_get_processor_info(soc_die_num, &plat_info);
		if (ret)
			return ret;
		if (plat_info.family == 0x1A
		    && (plat_info.model >= 0x10 && plat_info.model <=0x1F)) {
			for (i = 0; i < ARRAY_SIZE(thread_en_reg_v21_dense); i++) {
				ret = esmi_oob_rmi_read_byte(soc_die_num, thread_en_reg_v21_dense[i],
							     &buffer[i]);

				if (ret)
					return ret;
			}
		} else {
			for (i = 0; i < sizeof(thread_en_reg_v20); i++) {
				ret = esmi_oob_rmi_read_byte(soc_die_num, thread_en_reg_v20[i],
							     &buffer[i]);
				if (ret)
					return ret;
			}
		}

	} else {
		for (i = 0; i < sizeof(thread_en_reg_v20); i++) {
			ret = esmi_oob_rmi_read_byte(soc_die_num, thread_en_reg_v20[i],
						     &buffer[i]);
			if (ret)
				return ret;
		}
	}

	return OOB_SUCCESS;
}

oob_status_t read_sbrmi_swinterrupt(uint8_t soc_die_num,
				    uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_SOFTWAREINTERRUPT, buffer);
}

oob_status_t read_sbrmi_threadnumber(uint8_t soc_die_num,
				     uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_THREADNUMBER, buffer);
}

oob_status_t read_sbrmi_threadnumberlow(uint8_t soc_die_num,
					uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_THREADNUMBERLOW, buffer);
}

oob_status_t read_sbrmi_threadnumberhi(uint8_t soc_die_num,
				       uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_THREADNUMBERHIGH, buffer);
}

oob_status_t read_sbrmi_mp0_msg(uint8_t soc_die_num,
				uint8_t *buffer)
{
	int i, range;
	oob_status_t ret;

	range = SBRMI_MP0OUTBNDMSG7 - SBRMI_MP0OUTBNDMSG0 + 1;
	for (i = 0; i < range; i++) {
		ret = esmi_oob_rmi_read_byte(soc_die_num, SBRMI_MP0OUTBNDMSG0 + i,
					     &buffer[i]);
		if (ret)
			return ret;
	}

	return OOB_SUCCESS;
}

oob_status_t read_sbrmi_alert_status(uint8_t soc_die_num,
				     uint8_t num_of_alert_mask_reg,
				     uint8_t **buffer)
{
	struct processor_info plat_info = {0};
	oob_status_t ret;
	int i;
	uint8_t rev;

	if (!buffer || !(*buffer))
		return OOB_ARG_PTR_NULL;

	ret = esmi_get_processor_info(soc_die_num, &plat_info);
	if (ret)
		return ret;

	if (plat_info.family == 0x1A
	    && (plat_info.model >= 0x10 && plat_info.model <=0x1F)) {
		/*  Number of alert mask regsiters should be */
		/* equal to size of alert status v21 dense array */
		if (num_of_alert_mask_reg != ARRAY_SIZE(alert_status_v21_dense))
			return OOB_UNEXPECTED_SIZE;

		for (i = 0; i < ARRAY_SIZE(alert_status_v21_dense); i++) {
			ret = esmi_oob_rmi_read_byte(soc_die_num, alert_status_v21_dense[i],
						     (*buffer) + i);
			if (ret)
				return ret;
		}
	} else {
		/* Number of alert mask regsiters should be */
		/* equal to size of alert status array */
		if (num_of_alert_mask_reg != sizeof(alert_status))
			return OOB_UNEXPECTED_SIZE;

		for (i = 0; i < sizeof(alert_status); i++) {
			ret = esmi_oob_rmi_read_byte(soc_die_num, alert_status[i],
						     (*buffer) + i);
			if (ret)
				return ret;
		}
	}

	return OOB_SUCCESS;
}

oob_status_t read_sbrmi_alert_mask(uint8_t soc_die_num,
				   uint8_t num_of_alert_mask_reg,
				   uint8_t **buffer)
{
	struct processor_info plat_info = {0};
	oob_status_t ret;
	int i;
	uint8_t rev;

	if (!buffer || !(*buffer))
		return OOB_ARG_PTR_NULL;

	ret = esmi_get_processor_info(soc_die_num, &plat_info);
	if (ret)
		return ret;

	if (plat_info.family == 0x1A
	    && (plat_info.model >= 0x10 && plat_info.model <=0x1F)) {
		/*  Number of alert mask regsiters should be */
		/* equal to size of alert mask v21 dense array */
		if (num_of_alert_mask_reg != ARRAY_SIZE(alert_mask_v21_dense))
			return OOB_UNEXPECTED_SIZE;
		for (i = 0; i < ARRAY_SIZE(alert_mask_v21_dense); i++) {
			ret = esmi_oob_rmi_read_byte(soc_die_num, alert_mask_v21_dense[i],
						     (*buffer) + i);
			if (ret)
				return ret;
		}
	} else {
		/* Number of alert status registers should be */
		/* equal to size of alert mask arrary */
		if (num_of_alert_mask_reg != sizeof(alert_mask))
			return OOB_UNEXPECTED_SIZE;

		for (i = 0; i < sizeof(alert_mask); i++) {
			ret = esmi_oob_rmi_read_byte(soc_die_num, alert_mask[i],
						     (*buffer) + i);
		if (ret)
			return ret;
		}
	}

	return OOB_SUCCESS;
}

oob_status_t read_sbrmi_inbound_msg(uint8_t soc_die_num,
				    uint8_t *buffer)
{
	int i, range;
	oob_status_t ret;

	range = SBRMI_INBNDMSG7 - SBRMI_INBNDMSG0 + 1;
	for (i = 0; i < range; i++) {
		ret = esmi_oob_rmi_read_byte(soc_die_num, SBRMI_INBNDMSG0 + i,
					     &buffer[i]);
		if (ret)
			return ret;
	}

	return OOB_SUCCESS;
}

oob_status_t read_sbrmi_outbound_msg(uint8_t soc_die_num,
				     uint8_t *buffer)
{
	int i, range;
	oob_status_t ret;

	range = SBRMI_OUTBNDMSG7 - SBRMI_OUTBNDMSG0 + 1;
	for (i = 0; i < range; i++) {
		ret = esmi_oob_rmi_read_byte(soc_die_num, SBRMI_OUTBNDMSG0 + i,
					     &buffer[i]);
		if (ret)
			return ret;
	}

	return OOB_SUCCESS;
}

oob_status_t read_sbrmi_thread_cs(uint8_t soc_die_num,
				  uint8_t *buffer)
{
	oob_status_t ret;

	ret = esmi_oob_rmi_read_byte(soc_die_num, SBRMI_THREAD128CS, buffer);
	*buffer &= 1;

	return OOB_SUCCESS;
}

oob_status_t read_sbrmi_ras_status(uint8_t soc_die_num,
				   uint8_t *buffer)
{
	return esmi_oob_rmi_read_byte(soc_die_num, SBRMI_RASSTATUS, buffer);
}

oob_status_t clear_sbrmi_ras_status(uint8_t soc_die_num, uint8_t buffer)
{
	return esmi_oob_rmi_write_byte(soc_die_num, SBRMI_RASSTATUS, buffer);
}

oob_status_t esmi_get_threads_per_socket(uint8_t soc_die_num,
					 uint32_t *threads_per_socket)
{
	oob_status_t ret = 0;
	uint8_t thread_num_low = 0;
	uint8_t thread_num_hi = 0;

	if (!threads_per_socket)
		return OOB_ARG_PTR_NULL;

	/* Verify if requested thread number is for Milan */
	ret = esmi_oob_read_byte(soc_die_num, SBRMI_THREADNUMBER, SBRMI, &thread_num_low);
	if (ret != OOB_SUCCESS)
		return ret;

	if (!thread_num_low) {
		ret = esmi_oob_read_byte(soc_die_num, SBRMI_THREADNUMBERLOW, SBRMI, &thread_num_low);
		if (ret != OOB_SUCCESS)
			return ret;

		ret = esmi_oob_read_byte(soc_die_num, SBRMI_THREADNUMBERHIGH, SBRMI, &thread_num_hi);
		if (ret != OOB_SUCCESS)
			return ret;
	}

	*threads_per_socket = ((uint32_t)thread_num_hi << 8) | thread_num_low;
	return ret;
}

oob_status_t clear_sw_async_alert_status(uint8_t soc_die_num)
{
	uint8_t clear_alert_status = BIT(3);

	return esmi_oob_rmi_write_byte(soc_die_num, SBRMI_STATUS, clear_alert_status);
}

oob_status_t clear_MP0_alert_status(uint8_t soc_die_num)
{
	uint8_t clear_mp0_alert_status = BIT(6);

	return esmi_oob_rmi_write_byte(soc_die_num, SBRMI_STATUS, clear_mp0_alert_status);
}

oob_status_t set_sw_async_alert_mask(uint8_t soc_die_num, bool alert_mask_bit)
{
	uint8_t control_data = 0;
	oob_status_t ret;

	ret = read_sbrmi_control(soc_die_num, &control_data);
	if (ret)
		return ret;

	if (alert_mask_bit)
		control_data |= BIT(6);
	else
		control_data &= ~BIT(6);

	return esmi_oob_rmi_write_byte(soc_die_num, SBRMI_CONTROL, control_data);
}

oob_status_t read_sbrmi_ready_status(uint8_t soc_die_num, bool *buffer)
{
	uint8_t status_data = 0;
	oob_status_t ret;

	ret = esmi_oob_rmi_read_byte(soc_die_num, SBRMI_CONTROL2,  &status_data);
	if (!ret)
		*buffer = (status_data & BIT(0)) ? true : false;

	return ret;
}

oob_status_t clear_shut_down_err(uint8_t soc_die_num)
{
	uint8_t clear_shut_down_err = BIT(6);
	return esmi_oob_rmi_write_byte(soc_die_num, SBRMI_RASSTATUS,
				       clear_shut_down_err);
}

oob_status_t gpio_assertion_on_mailbox(uint8_t soc_die_num, uint8_t dis_alert)
{
	uint8_t control_data = 0;
	oob_status_t ret;

	if (dis_alert > MAX_ONE_BIT_DATA)
		return OOB_INVALID_INPUT;

	ret = read_sbrmi_control(soc_die_num, &control_data);
	if (ret)
		return ret;

	if (dis_alert)
		control_data |= BIT(4);  /* Set bit-4 to 1 to disable Alert-L signaling */
	else
		control_data &= ~BIT(4); /* Set bit-4 to 0 to enable Alert-L signaling */

	return esmi_oob_rmi_write_byte(soc_die_num, SBRMI_CONTROL, control_data);
}

oob_status_t gpio_assertion_for_async_alerts(uint8_t soc_die_num, uint8_t dis_alert)
{
	uint8_t control_data = 0;
	oob_status_t ret;

	if (dis_alert > MAX_ONE_BIT_DATA)
		return OOB_INVALID_INPUT;

	ret = read_sbrmi_control(soc_die_num, &control_data);
	if (ret)
		return ret;

	if (dis_alert)
		control_data |= BIT(6);  /* Set bit-6 to 1 to disable Alert-L signaling for asynchronous alerts */
	else
		control_data &= ~BIT(6); /* Set bit-6 to 0 to enable Alert-L signaling for asynchronous alerts */

	return esmi_oob_rmi_write_byte(soc_die_num, SBRMI_CONTROL, control_data);
}
