/*
 * University of Illinois/NCSA Open Source License
 *
 * Copyright (c) 2022, Advanced Micro Devices, Inc.
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
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/apml_recovery.h>
#include <esmi_oob/esmi_rmi.h>
#include <esmi_oob/esmi_tsi.h>

#define MAX_RETRY	20
#define CONFIG_MASK	0x1
#define CTRL_MASK	0x2
/* Recovery wait time of 1000 micro seconds */
#define REC_WAIT	1000

static oob_status_t apml_recover_sbrmi(uint8_t soc_die_num)
{
	int retry = MAX_RETRY;
	oob_status_t ret = 0;
	uint8_t config = 0, rev = 0;

	/* Verify that the SBTSI is working */
	ret = read_sbtsi_revision(soc_die_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Read the curent configuration before writing to bit 0 */
	ret = read_sbtsi_config(soc_die_num, &config);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Write 1 to bit 0 of sbtsi config register to notify recovery of sbrmi */
	config |= CONFIG_MASK;
	ret = esmi_oob_tsi_write_byte(soc_die_num, SBTSI_CONFIGWR, config);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Wait for the sbtsi config reg bit 0 to set to 0 */
	do {
		ret = read_sbtsi_config(soc_die_num, &config);
		if (ret != OOB_SUCCESS)
			return ret;

		if (!(config & CONFIG_MASK))
			break;

		/* Sleep for 1 millsecond */
		usleep(REC_WAIT);

	} while (retry--);

	/* Verify if sbrmi is recoverd */
	ret = read_sbrmi_revision(soc_die_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;
	return ret;
}

static oob_status_t apml_recover_sbtsi(uint8_t soc_die_num)
{
	int retry = MAX_RETRY;
	oob_status_t ret = 0;
	uint8_t control= 0, rev = 0;

	/* Verify that the SBRMI is working */
	ret = read_sbrmi_revision(soc_die_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Read the curent configuration before writing to bit 1 */
	ret = read_sbrmi_control(soc_die_num, &control);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Write 1 to bit 1 of sbrmi control register to notify recovery of sbtsi */
	control |= CTRL_MASK;
	ret = esmi_oob_rmi_write_byte(soc_die_num, SBRMI_CONTROL, control);
	if (ret != OOB_SUCCESS)
		return ret;

	/* Wait for the control reg bit 1 to set to 0 */
	do {
		ret = read_sbrmi_control(soc_die_num, &control);
		if (ret != OOB_SUCCESS)
			return ret;

		if (!(control & CTRL_MASK))
			break;
		/* sleep for 1 millisecond */
		usleep(REC_WAIT);

	} while (retry--);

	/* Verify if sbtsi is recoverd */
	ret = read_sbtsi_revision(soc_die_num, &rev);
	if (ret != OOB_SUCCESS)
		return ret;
	return ret;
}

oob_status_t apml_recover_dev(uint8_t soc_die_num, uint8_t client)
{
	oob_status_t ret = 0;

	/* Verify recovery require in sbrmi or sbtsi */
	if (client == DEV_SBRMI) {
		ret = apml_recover_sbrmi(soc_die_num);
		if (ret != OOB_SUCCESS)
			return ret;
	} else if (client == DEV_SBTSI) {
		ret = apml_recover_sbtsi(soc_die_num);
		if (ret != OOB_SUCCESS)
			return ret;
	} else {
		ret = OOB_INVALID_INPUT;
	}

	return ret;
}
