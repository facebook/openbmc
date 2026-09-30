/*
 * University of Illinois/NCSA Open Source License
 *
 * Copyright (c) 2021, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Developed by:
 *
 *                 AMD Research and AMD Software Development
 *
 *                 Advanced Micro Devices, Inc.
 *
 *                 www.amd.com
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

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <esmi_oob/apml.h>
#include <esmi_oob/apml64Config.h>
#include <esmi_oob/apml_recovery.h>
#include <esmi_oob/esmi_cpuid_msr.h>
#include <esmi_oob/esmi_mailbox.h>
#include <esmi_oob/esmi_rmi.h>
#include <esmi_oob/esmi_tsi.h>
#include <esmi_oob/rmi_mailbox_mi300.h>
#include <esmi_oob/tsi_mi300.h>
#include "mi300_tool.h"

#define RED "\x1b[31m"
#define RESET "\x1b[0m"
#define ARGS_MAX 64
#define APML_SLEEP 10000
#define SCALING_FACTOR	0.25
/* Maximum post code offset */
#define MAX_POST_CODE_OFFSET	8
/* Zero OFFSET */
#define ZERO_OFFSET		0
/* DRAM ECC error category */
#define DRAM_ECC_ERR_CAT	1
/* SVI3 Rail mode bits */
#define SVI3_RAIL_MODE_BITS	1
/* SVI3 Rail index bits */
#define SVI3_RAIL_INDEX_BITS	3
/* HSMP commands mask bits */
#define HSMP_CMDS_MASK_BITS	1
/* HSMP Commands Offset bits */
#define HSMP_CMDS_OFFSET_BITS	4
/* BIT Mask Segment bits */
#define BIT_MASK_SEGMENT_BITS	24
/* Rank Multiplier Mask */
#define RANK_MUL_MASK		0x7
/* Nibble mask */
#define NIBBLE_MASK		0xF
/* Channel number position */
#define CH_NUM_POS		16
/* MAX PCIE PORT */
#define MAX_PCIE_PORT		16
/* Sub channel number position */
#define SUB_CH_POS		20
/* Chip select number position */
#define CHIP_SEL_NUM_POS	21
/* Rank multiplier number */
#define RANK_MUL_NUM_POS	23
/* BIT mask */
#define BIT_MASK		0x1
/* DRAM CECC leak rate mask */
#define DRAM_CECC_LEAK_RATE_MASK	0x1F
#define MAX_PERF_MODE		5

static uint32_t clamp_val(int val, uint32_t lo, uint32_t hi) {
    if (val < (int)lo)
	return lo;
    else if (val > (int)hi)
	return hi;
    else
	return (uint32_t)val;
}

/**
 * @brief Validate that an integer value fits within the specified bit width
 *
 * @param[in] value The integer value to validate
 * @param[in] max_bits The maximum number of bits for the field (e.g., 1 for bool, 4 for 4-bit field)
 * @param[in] param_name The name of the parameter for error reporting
 *
 * @retval 0 on success (value is within valid range)
 * @retval UNEXPECTED_SIZE on failure (value exceeds maximum allowed for the bit width)
 *
 * @details This function validates that the input value does not exceed the maximum
 * value representable by the specified number of bits. For example:
 * - 1-bit field: valid range is 0-1
 * - 4-bit field: valid range is 0-15
 * - 8-bit field: valid range is 0-255
 * If validation fails, an error message is printed indicating the invalid input.
 */
static oob_status_t validate_bitfield_range(long value, uint8_t max_bits, const char *param_name)
{
	uint32_t max_value;
	const char *parameter_name = (param_name != NULL) ? param_name : "parameter";

	if (max_bits >= D_WORD_BITS)
		max_value = UINT32_MAX;
	else
		max_value = ((uint32_t)1 << max_bits) - 1U;  /* Calculate max value: 2^max_bits - 1 */

	/* Check if value is negative or exceeds maximum */
	if (value < 0 || value > max_value) {
		printf(RED "Invalid Input for %s. Value %ld is out of range [0-%u] for %u-bit field." RESET "\n",
			parameter_name, value, max_value, max_bits);
		return OOB_UNEXPECTED_SIZE;
	}

	return OOB_SUCCESS;
}

static int flag;

static oob_status_t validate_apml_sbtsi_module(uint8_t soc_die_num)
{
	bool is_sbtsi = false;
	oob_status_t ret = OOB_SUCCESS;

	ret = validate_sbtsi_module(soc_die_num, &is_sbtsi);
	if (ret || !is_sbtsi)
		printf(RED" SBTSI module not present.Please install "
		       "the module" RESET"\n");
	return ret;
}

static oob_status_t validate_apml_sbrmi_module(uint8_t soc_die_num)
{
	bool is_sbrmi =false;
	oob_status_t ret = OOB_SUCCESS;

	ret = validate_sbrmi_module(soc_die_num, &is_sbrmi);
	if (ret || !is_sbrmi)
		printf(RED" SBRMI module not present.Please install "
		       "the module" RESET"\n");
	return ret;
}

static oob_status_t get_platform_info(uint8_t soc_die_num,
				      struct processor_info *plat_info,
				      bool *rev_status)
{
	uint8_t rev = 0;
	oob_status_t ret = OOB_SUCCESS;

	ret = read_sbrmi_revision(soc_die_num, &rev);
	if (!ret) {
		*rev_status = true;
		if (rev != 0x10)
			ret = esmi_get_processor_info(soc_die_num, plat_info);
		return ret;
	}
	*rev_status = false;
	return ret;
}

static oob_status_t get_proc_type(uint8_t soc_die_num,  uint8_t *p_type)
{
	uint8_t rev = 0;
	oob_status_t ret = OOB_SUCCESS;
	bool rev_status = false;

	ret = get_platform_info(soc_die_num, plat_info, &rev_status);
	if (ret) {
		if (!rev_status) {
			*p_type = NOT_SUPPORTED;
			return ret;
		}
	}
	/* Family 1A and Model in 00 - 0Fh */
	if (plat_info->family == 0x1A) {
		switch (plat_info->model) {
		case 0x00 ... 0x0F:
			*p_type = FAM_1A_MOD_00;
			break;
		case 0x10 ... 0x1F:
			*p_type = FAM_1A_MOD_10;
			break;
                case 0x50 ... 0x5F:
                        *p_type = FAM_1A_MOD_50;
                        break;
		default:
			*p_type = LEGACY_PLATFORMS;
		}
	} else if (plat_info->family == 0x19) {
		switch (plat_info->model) {
		case 0x10 ... 0x1F:
			*p_type = FAM_19_MOD_10;
			break;
		case 0x90 ... 0x9F:
			*p_type = FAM_19_MOD_90;
			break;
		case 0xA0 ... 0xAF:
			*p_type = FAM_19_MOD_A0;
			break;
		default:
			*p_type = LEGACY_PLATFORMS;
			break;
		}
	} else {
		*p_type = LEGACY_PLATFORMS;
	}
	return ret;
}

static oob_status_t apml_get_sockpower(uint8_t soc_die_num)
{
	uint32_t power = 0;
	oob_status_t ret;

	ret = read_socket_power(soc_die_num, &power);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get power, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("---------------------------------------------");
	printf("\n| Power (Watts)\t\t |");
	printf(" %-17.3f|", (double)power/1000);

	/* Get the PowerLimit for a given soc_die_num index */
	ret = read_socket_power_limit(soc_die_num, &power);
	if (ret != OOB_SUCCESS) {
		printf("\nFailed to get powerlimit, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("\n| PowerLimit (Watts)\t |");
	printf(" %-17.3f|", (double)power/1000);

	/* Get the maxpower for a given soc_die_num index */
	ret = read_max_socket_power_limit(soc_die_num, &power);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get maxpower, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("\n| PowerLimitMax (Watts)\t |");
	printf(" %-17.3f|", (double)power/1000);
	printf("\n---------------------------------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_get_socktdp(uint8_t soc_die_num)
{
	uint32_t buffer = 0;
	oob_status_t ret;

	ret = read_tdp(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get tdp, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("---------------------------------------------\n");
	printf("| TDP (Watts)\t\t| %-17.03f |\n", (double)buffer/1000);

	/* Get min tdp value for a given soc_die_num */
	ret = read_min_tdp(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get min tdp, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("| Min_TDP (Watts)\t| %-17.03f |\n", (double)buffer/1000);

	/* Get max tdp value for a given soc_die_num */
	ret = read_max_tdp(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get max_tdp, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("| Max_TDP (Watts)\t| %-17.03f |\n", (double)buffer/1000);
	printf("---------------------------------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_setpower_limit(uint8_t soc_die_num,
					uint32_t power)
{
	uint32_t max_power = 0;
	oob_status_t ret;

	ret = read_max_socket_power_limit(soc_die_num, &max_power);
	if ((ret == OOB_SUCCESS) && (power > max_power)) {
		printf("Input power is not within accepted limit,\n"
			"So value set to default max %.3f Watts\n",
			(double)max_power/1000);
		power = max_power;
	}
	ret = write_socket_power_limit(soc_die_num, power);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set power_limit, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("\nSet power_limit : %16.03f Watts "
		"successfully\n", (double)power/1000);
	return OOB_SUCCESS;
}

static oob_status_t apml_get_soc_dimm_power_limit(uint8_t soc_die_num)
{
	uint32_t limit = 0;
	oob_status_t ret;

	ret = soc_dimm_power_limit(soc_die_num, 1, &limit);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get SOC+DIMM power limit, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}

	printf("---------------------------------------------");
	printf("\n| SOC+DIMM Power Limit (Watts) |");
	if (limit == 0) {
		printf(" %-17s|", "Disabled");
	} else {
		printf(" %-17.3f|", (double)limit/1000);
	}
	printf("\n---------------------------------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_set_soc_dimm_power_limit(uint8_t soc_die_num, uint32_t power)
{
	oob_status_t ret;

	ret = soc_dimm_power_limit(soc_die_num, 0, &power);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set SOC+DIMM power limit, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}

	if (power == 0) {
		printf("\nSOC+DIMM power limit disabled successfully\n");
	} else {
		printf("\nSet SOC+DIMM power limit: %16.03f Watts successfully\n",
			(double)power/1000);
	}

	return OOB_SUCCESS;
}

static void apml_get_ddr_bandwidth(uint8_t soc_die_num)
{
	struct max_ddr_bw max_ddr;
	oob_status_t ret;

	ret = read_ddr_bandwidth(soc_die_num, &max_ddr);
	if (ret != OOB_SUCCESS) {
		printf("Failed:to get DDR Bandwidth, "
		       "Err[%d]:%s\n", ret,
		       esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------------");
	printf("\n| DDR Max BW (GB/s)\t |");
	printf(" %-17d|", max_ddr.max_bw);
	printf("\n| DDR Utilized BW (GB/s) |");
	printf(" %-17d|", max_ddr.utilized_bw);
	printf("\n| DDR Utilized Percent(%%)|");
	printf(" %-17d|", max_ddr.utilized_pct);
	printf("\n---------------------------------------------\n");
}

static oob_status_t get_boostlimit(uint8_t soc_die_num,
				   uint32_t core_id)
{
	uint32_t buffer = 0;
	oob_status_t ret;

	ret = read_esb_boost_limit(soc_die_num, core_id, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to get core[%d] apml_boostlimit,"
		       " Err[%d]: %s\n", core_id, ret,
		       esmi_get_err_msg(ret));
		return ret;
	}

	printf("------------------------------------------------------"
		"-------\n");
	printf("| core[%03d] apml_boostlimit (MHz)\t | %-17u|\n",
	       core_id, buffer);

	usleep(APML_SLEEP);
	/* Get the Bios boostlimit for a given soc_die_num index */
	ret = read_bios_boost_fmax(soc_die_num, core_id, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get core[%d] bios_boostlimit, "
		       "Err[%d]: %s\n", core_id, ret,
			esmi_get_err_msg(ret));
		return ret;
	}
	printf("| core[%03d] bios_boostlimit (MHz)\t | %-17u|\n",
		core_id, buffer);
	printf("------------------------------------------------------"
		"-------\n");

	return OOB_SUCCESS;
}


static oob_status_t validate_bootlimit_input(uint8_t soc_die_num, uint32_t *boostlimit)
{
	uint16_t fmax, fmin;
	oob_status_t ret = 0;

	/*if (*boostlimit > UINT16_MAX)
                return OOB_INVALID_INPUT; */

	ret = read_socket_freq_range(soc_die_num, &fmax, &fmin);
        if (ret != OOB_SUCCESS) {
		printf("Failed to get Fmax and Fmin, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
         }

	if (*boostlimit > fmax) {
		printf("Input > max boostlimit, Configuring max boostlimit \n");
		*boostlimit = fmax;
        }
        if(*boostlimit < fmin) {
		printf("Input < min boostlimit, Configuring min boostlimit\n");
		*boostlimit = fmin;
        }

	return OOB_SUCCESS;

}

static oob_status_t set_apml_boostlimit(uint8_t soc_die_num, uint32_t core_id,
					uint32_t boostlimit)
{
	oob_status_t ret;

	ret = validate_bootlimit_input(soc_die_num, &boostlimit);
	if(ret != OOB_SUCCESS) {
		printf("Input validation failed \n");
		return ret;
	}

	ret = write_esb_boost_limit(soc_die_num, core_id, boostlimit);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set core[%d] apml_boostlimit "
		       "Err[%d]: %s\n", core_id, ret,
			esmi_get_err_msg(ret));
		return ret;
	}

	printf("core[%d] apml_boostlimit %u MHz set successfully\n", core_id, boostlimit);
	return OOB_SUCCESS;
}

static oob_status_t set_apml_socket_boostlimit(uint8_t soc_die_num,
					       uint32_t boostlimit)
{
	oob_status_t ret;

	ret = validate_bootlimit_input(soc_die_num, &boostlimit);
	if(ret != OOB_SUCCESS) {
                printf("Input validation failed, try again with valid input range \n");
                return ret;
        }

	ret = write_esb_boost_limit_allcores(soc_die_num, boostlimit);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set apml_boostlimit for all cores Err[%d]: "
		       "%s\n", ret, esmi_get_err_msg(ret));
		return ret;
	}

	printf("apml_boostlimit for all cores set successfully\n");
	return OOB_SUCCESS;
}

static oob_status_t set_and_verify_dram_throttle(uint8_t soc_die_num,
						 uint32_t dram_thr)
{
	uint32_t limit = 0;
	oob_status_t ret;

	ret = write_dram_throttle(soc_die_num, dram_thr);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set DRAM throttle, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	usleep(APML_SLEEP);
	ret = read_dram_throttle(soc_die_num, &limit);
	if (ret == OOB_SUCCESS) {
		if (limit < dram_thr)
			printf("Set to max dram throttle: %u %%\n", limit);
		else if (limit > dram_thr)
			printf("Set to min dram throttle: %u %%\n", limit);
	}
	printf("Set and Verify Success %u %%\n", limit);
	return OOB_SUCCESS;
}

static oob_status_t set_and_verify_apml_socket_uprate(uint8_t soc_die_num,
						      float uprate)
{
	float rduprate;
	oob_status_t ret;

	ret = write_sbtsi_updaterate(soc_die_num, uprate);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set Update rate for addr, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return ret;
	}
	usleep(APML_SLEEP);

	if (read_sbtsi_updaterate(soc_die_num, &rduprate) == 0) {
		if (uprate != rduprate)
			return OOB_TRY_AGAIN;
		printf("Set and verify Success %f\n", rduprate);
	}

	return OOB_SUCCESS;
}

static oob_status_t set_high_temp_threshold(uint8_t soc_die_num, float temp)
{
	float temp_dec;
	int temp_int;
	oob_status_t ret;

	ret = sbtsi_set_hitemp_threshold(soc_die_num, temp);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set Higher Temp threshold limit, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("Set Success\n");
	return OOB_SUCCESS;
}

static oob_status_t set_low_temp_threshold(uint8_t soc_die_num, float temp)
{
	float temp_dec;
	int temp_int;
	oob_status_t ret;

	if (temp < 0 || temp > 70) {
		printf("Invalid temp, please mention temp between 0 and 70\n");
		return OOB_INVALID_INPUT;
	}

	ret = sbtsi_set_lotemp_threshold(soc_die_num, temp);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set Lower Temp threshold limit, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("Set Success\n");
	return OOB_SUCCESS;
}

static oob_status_t set_temp_offset(uint8_t soc_die_num, float temp)
{
	oob_status_t ret;

	ret = write_sbtsi_cputempoffset(soc_die_num, temp);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set Temp offset, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("Set CPU temp offset success\n");
	return OOB_SUCCESS;
}

static oob_status_t set_timeout_config(uint8_t soc_die_num, int value)
{
	oob_status_t ret;

	ret = sbtsi_set_timeout_config(soc_die_num, value);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set timeout config, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("Set timeout config success\n");
	return OOB_SUCCESS;
}

static oob_status_t set_alert_threshold(uint8_t soc_die_num, int value)
{
	oob_status_t ret;

	ret = sbtsi_set_alert_threshold(soc_die_num, value);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set alert threshold sample, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("Set alert threshold success\n");
	return OOB_SUCCESS;
}

static oob_status_t set_alert_config(uint8_t soc_die_num, int value)
{
	oob_status_t ret;

	ret = sbtsi_set_alert_config(soc_die_num, value);
	if (ret != OOB_SUCCESS) {
		printf("Failed: to set alert config, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("Set alert config success\n");
	return OOB_SUCCESS;
}

static oob_status_t set_tsi_config(uint8_t soc_die_num, int value,
				   uint16_t check)
{
	oob_status_t ret;

	switch (check) {
	case 1208:
		ret = sbtsi_set_configwr(soc_die_num, value,
					 ALERTMASK_MASK);
		if (ret != OOB_SUCCESS) {
			printf("Failed: to set tsi config alert_mask, "
				"Err[%d]: %s\n", ret,
				esmi_get_err_msg(ret));
			return ret;
		}
		printf("ALERT_L pin %s\n", value ? "Disabled" : "Enabled");
		break;
	case 1209:
		ret = sbtsi_set_configwr(soc_die_num, value,
					 RUNSTOP_MASK);
		if (ret != OOB_SUCCESS) {
			printf("Failed: to set tsi config runstop_mask, "
				"Err[%d]: %s\n", ret,
				esmi_get_err_msg(ret));
			return ret;
		}
		printf("runstop bit %s\n", value ? "Comparisions Disabled"
		       : "Comparisions Enabled");
		break;
	case 1210:
		ret = sbtsi_set_configwr(soc_die_num, value,
					   READORDER_MASK);
		if (ret != OOB_SUCCESS) {
			printf("Failed: to set tsi config readorder_mask, "
				"Err[%d]: %s\n", ret,
				esmi_get_err_msg(ret));
			return ret;
		}
		printf("Atomic read bit %s\n", value ? "Decimal Latches "
			"Integer" : "Integer Latches Decimal");
		break;
	case 1211:
		ret = sbtsi_set_configwr(soc_die_num, value,
					   ARA_MASK);
		if (ret != OOB_SUCCESS) {
			printf("Failed: to set tsi config ara_mask, "
				"Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
			return ret;
		}
		printf("ARA Disable bit %s\n", value ? "Disabled" : "Enabled");
	}
	return OOB_SUCCESS;
}

static oob_status_t get_apml_rmi_access(uint8_t soc_die_num)
{
	struct processor_info plat_info;
	int i, range;
	uint64_t alert_mask_offset = 0;
	uint8_t buf, rev;
	uint8_t *buffer;
	oob_status_t ret;
	bool is_rsdn = false;
	bool is_brhdn = false;

	ret = validate_apml_sbrmi_module(soc_die_num);
	if (ret)
		return ret;

	printf("------------------------------------------------------------"
		"----\n");
	printf("\n\t\t\t *** SB-RMI REGISTER SUMMARY ***\n");
	printf("------------------------------------------------------------"
		"----\n");
	printf("\t FUNCTION [register] \t\t\t| Value [Units]\n");
	printf("------------------------------------------------------------"
		"----\n");
	ret = read_sbrmi_revision(soc_die_num, &rev);
	if (ret != 0) {
		printf("Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("_RMI_REVISION [0x%x]		\t\t| %#4x\n",
	       SBRMI_REVISION, rev);

	usleep(APML_SLEEP);
	if (read_sbrmi_control(soc_die_num, &buf) == 0)
		printf("_RMI_CONTROL [0x%x]		\t\t| %#4x\n",
		       SBRMI_CONTROL, buf);

	usleep(APML_SLEEP);
	if (read_sbrmi_status(soc_die_num, &buf) == 0)
		printf("_RMI_STATUS [0x%x]		\t\t| %#4x\n",
		       SBRMI_STATUS, buf);

	usleep(APML_SLEEP);
	if (read_sbrmi_readsize(soc_die_num, &buf) == 0)
		printf("_RMI_READSIZE [0x%x]		\t\t| %#4x\n",
		       SBRMI_READSIZE, buf);

	usleep(APML_SLEEP);
	if (rev == 0x10) {
		range = sizeof(thread_en_reg_v10);
	} else if (rev == 0x20) {
		range = sizeof(thread_en_reg_v20);
	} else if (rev == 0x21) {
		ret = esmi_get_processor_info(soc_die_num, &plat_info);
		if (ret)
			return ret;
		if (plat_info.family == 0x1A && plat_info.model >= 0x10
		    && plat_info.model <= 0x1F) {
			is_brhdn = true;
			range = ARRAY_SIZE(thread_en_reg_v21_dense);
		} else {
			range = sizeof(thread_en_reg_v20);
		}
	} else {
		range = sizeof(thread_en_reg_v20);
	}

	buffer = malloc(range * sizeof(uint8_t));
	if (read_sbrmi_multithreadenablestatus(soc_die_num,
					       buffer) == 0) {
		printf("_RMI_THREADENSTATUS \t\t\t\t|\n");
		for (i = 0; i < range; i++)
			printf("\t[0x%x] Thread[%d:%d]	\t\t| %#4x\n",
			       (is_brhdn ? thread_en_reg_v21_dense[i] : thread_en_reg_v20[i]),
			       (i * 8) + 7, i * 8, buffer[i]);
	}
	free(buffer);
	buffer = NULL;

	if (rev == 0x20) {
		ret = esmi_get_processor_info(soc_die_num, &plat_info);
		if (ret)
			return ret;
		if (plat_info.family == 0x19 && plat_info.model >= 0xA0
		    && plat_info.model <= 0xAF)
			is_rsdn = true;
	}

	usleep(APML_SLEEP);
	if (is_brhdn)
		range = ARRAY_SIZE(alert_status_v21_dense);
	else
		range = MAX_ALERT_REG;
	buffer = malloc(range * sizeof(uint8_t));
	if (!buffer)
		return OOB_NO_MEMORY;
	if (read_sbrmi_alert_status(soc_die_num, range, &buffer) == 0) {
			printf("_RMI_ALERTSTATUS [0x%x ~ 0x%x] [0x%x ~ 0x%x] \t|\n",
				SBRMI_ALERTSTATUS0, SBRMI_ALERTSTATUS15,
				SBRMI_ALERTSTATUS16, SBRMI_ALERTSTATUS31);
			if (is_brhdn)
				printf("\t\t [0x%x ~ 0x%x] \t\t|\n",
					SBRMI_ALERTSTATUS32, SBRMI_ALERTSTATUS47);
			for (i = 0; i < range; i++) {
				printf("\t[ ");
				if ( i < MAX_ALERT_REG) {
					for (int j = 15; j >= 0; j--) {
						switch (j % 16) {
						case 4 ... 7:
							if (i / 16)
								printf("%3d ", 16 * (j % 16) + (i - 16));
							break;
						case 12 ... 15:
							if (i / 16 && rev != 0x10)
								if ((rev == 0x20 && is_rsdn)
								     || rev == 0x21)
									printf("%3d ",
										16 * (j % 16) + (i - 16));
							break;
						case 0 ... 3:
							if (i / 16 == 0)
								printf("%3d ", 16 * (j % 16) + i);
							break;
						case 8 ... 11:
							if (i / 16 == 0 && rev != 0x10)
								printf("%3d ", 16 * (j % 16) + i);
							break;
						}
					}
				} else {
					for (int j = 7; j >= 0; j--)
						printf("%3d ", 16 * j + (256 + (i - MAX_ALERT_REG)));
				}
				if (rev != 0x10)
					if  (i > 15 && (!is_rsdn && rev !=0x21))
						printf("] \t\t\t| %#4x\n", buffer[i]);
					else
						printf("] \t| %#4x\n", buffer[i]);
				else
					printf("]        \t\t| %#4x\n", buffer[i]);
			}
	}
	free(buffer);
	buffer = NULL;

	usleep(APML_SLEEP);
	if (is_brhdn)
		range = ARRAY_SIZE(alert_mask_v21_dense);
	else
		range = sizeof(alert_mask);
	buffer = malloc(range * sizeof(uint8_t));
	if (!buffer)
		return OOB_NO_MEMORY;
	if (read_sbrmi_alert_mask(soc_die_num, range, &buffer) == 0) {
			printf("_RMI_ALERTMASK [0x%x ~ 0x%x] [0x%x ~ 0x%x] \t|\n",
			       SBRMI_ALERTMASK0, SBRMI_ALERTMASK15,
			       SBRMI_ALERTMASK16, SBRMI_ALERTMASK31);
			if (is_brhdn)
				printf("\t       [0x%x ~ 0x%x] \t\t\t|\n",
					SBRMI_ALERTMASK32, SBRMI_ALERTMASK47);
			for (i = 0; i < range; i++) {
				printf("\t[ ");
				if (i < sizeof(alert_mask)) {
					for (int j = 15; j >= 0; j--) {
						switch (j % 16) {
						case 4 ... 7:
							if (i / 16)
								printf("%3d ", 16 * (j % 16) + (i - 16));
							break;
						case 12 ... 15:
							if (i / 16 && rev != 0x10)
								if ((rev == 0x20 && is_rsdn)
								     || rev == 0x21)
									printf("%3d ",
										16 * (j % 16) + (i - 16));
							break;
						case 0 ... 3:
							if (i / 16 == 0)
								printf("%3d ", 16 * (j % 16) + i);
							break;
						case 8 ... 11:
							if (i / 16 == 0 && rev != 0x10)
								printf("%3d ", 16 * (j % 16) + i);
							break;
						}
					}
				} else {
					for (int j = 7; j >= 0; j--) {
						/* Calculate the alert mask offset for extended range */
						alert_mask_offset = 16 * j + (256 + (i - sizeof(alert_mask)));
						printf("%3" PRIu64 " ", alert_mask_offset);
					}
				}
				if (rev != 0x10)
					if (i > 15 && (!is_rsdn && rev !=0x21))
						printf("] \t\t\t| %#4x\n", buffer[i]);
					else
						printf("] \t| %#4x\n", buffer[i]);
				else
					printf("]        \t\t| %#4x\n", buffer[i]);
			}
	}
	free(buffer);
	buffer = NULL;

	usleep(APML_SLEEP);
	range = SBRMI_OUTBNDMSG7 - SBRMI_OUTBNDMSG0 + 1;
	buffer = malloc(range * sizeof(uint8_t));
	if (!buffer)
		return OOB_NO_MEMORY;
	if (read_sbrmi_outbound_msg(soc_die_num, buffer) == 0) {
		printf("_RMI_OUTBOUNDMSG [0x%x ~ 0x%x]	\t\t|\n",
		       SBRMI_OUTBNDMSG0, SBRMI_OUTBNDMSG7);
		for (i = 0; i < range; i++)
			printf("\tOUTBNDMSG[%d]	\t\t\t| %#4x\n", i, buffer[i]);
	}
	free(buffer);
	buffer = NULL;

	usleep(APML_SLEEP);
	range = SBRMI_INBNDMSG7 - SBRMI_INBNDMSG0 + 1;
	buffer = malloc(range * sizeof(uint8_t));
	if (!buffer)
		return OOB_NO_MEMORY;
	if (read_sbrmi_inbound_msg(soc_die_num, buffer) == 0) {
		printf("_RMI_INBOUNDMSG [0x%x ~ 0x%x]	\t\t|\n",
		       SBRMI_INBNDMSG0, SBRMI_INBNDMSG7);
		for (i = 0; i < range; i++)
			printf("\tINBNDMSG[%d]	\t\t\t| %#4x\n", i, buffer[i]);
	}
	free(buffer);
	buffer = NULL;

	usleep(APML_SLEEP);
	if (read_sbrmi_swinterrupt(soc_die_num, &buf) == 0)
		printf("_RMI_SWINTERRUPT [0x%x]	\t\t\t| %#4x\n",
		       SBRMI_SOFTWAREINTERRUPT, buf);

	usleep(APML_SLEEP);
	if (rev == 0x10) {
		if (read_sbrmi_threadnumber(soc_die_num, &buf) == 0)
			printf("_RMI_THREADNUMEBER [0x%x]	\t\t| %#4x\n",
			       SBRMI_THREADNUMBER, buf);
	} else {
		if (read_sbrmi_threadnumberlow(soc_die_num, &buf) == 0)
			printf("_RMI_THREADNUMEBERLOW [0x%x]	\t\t| %#4x\n",
			       SBRMI_THREADNUMBERLOW, buf);
		if (read_sbrmi_threadnumberhi(soc_die_num, &buf) == 0)
			printf("_RMI_THREADNUMEBERHIGH [0x%x]	\t\t| %#4x\n",
			       SBRMI_THREADNUMBERHIGH, buf);
	}

	usleep(APML_SLEEP);
	if (read_sbrmi_thread_cs(soc_die_num, &buf) == 0)
		printf("_RMI_THREADCS [0x%x]	\t\t\t| %#4x\n",
		       SBRMI_THREAD128CS, buf);

	usleep(APML_SLEEP);
	if (read_sbrmi_ras_status(soc_die_num, &buf) == 0)
		printf("_RMI_RASSTATUS [0x%x]	\t\t\t| %#4x\n",
		       SBRMI_RASSTATUS, buf);

	usleep(APML_SLEEP);
	range = SBRMI_MP0OUTBNDMSG7 - SBRMI_MP0OUTBNDMSG0 + 1;
	buffer = malloc(range * sizeof(uint8_t));
	if (!buffer)
		return OOB_NO_MEMORY;
	if (read_sbrmi_mp0_msg(soc_die_num, buffer) == 0) {
		printf("_RMI_MP0 [0x%x ~ 0x%x]	\t\t\t|\n",
		       SBRMI_MP0OUTBNDMSG0, SBRMI_MP0OUTBNDMSG7);
		for (i = 0; i < range; i++)
			printf("\tOUTBNDMSG[%d]	\t\t\t| %#4x\n", i, buffer[i]);
	}
	free(buffer);
	buffer = NULL;
	printf("------------------------------------------------------------"
		"----\n");
	return OOB_SUCCESS;
}

static oob_status_t get_apml_tsi_register_descriptions(uint8_t soc_die_num)
{
	float temp_value[3];
	float dec;
	float uprate;
	uint8_t lowalert, hialert;
	uint8_t al_mask, run_stop, read_ord, ara;
	uint8_t timeout;
	uint8_t intr;
	int8_t intr_offset;
	uint8_t id, buf;
	bool is_sbtsi = false;
	bool status = false;
	oob_status_t ret;

	ret = validate_apml_sbtsi_module(soc_die_num);
	if (ret)
		return ret;
	intr = 0;
	ret = read_sbtsi_max_hbm_temp_int(soc_die_num, &intr);
	if (ret)
		return ret;
	if (intr)
		status = true;
	usleep(APML_SLEEP);
	ret = sbtsi_get_cputemp(soc_die_num, &temp_value[0]);
	if (ret)
		return ret;

	usleep(APML_SLEEP);
	ret = read_sbtsi_cpuinttemp(soc_die_num, &intr);
	if (ret)
		return ret;
	ret = read_sbtsi_cputempdecimal(soc_die_num, &dec);
	if (ret)
		return ret;

	printf("\n\t\t *** SB-TSI REGISTER SUMMARY ***\n");
	printf("------------------------------------------------------------"
	       "-----------------------\n");
	printf(" FUNCTION/Reg Name\t| Reg offset\t| Hexa(0x)\t| Value [Units]\n");
	printf("------------------------------------------------------------"
	       "-------------------------------\n");
	printf("_PROCTEMP\t\t|\t\t|\t\t| %.3f °C\n", temp_value[0]);
	printf("\tPROC_INT \t| 0x%x \t\t| 0x%-5x\t| %u °C\n", SBTSI_CPUTEMPINT,
	       intr, intr);
	printf("\tPROC_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n", SBTSI_CPUTEMPDEC,
	       (uint8_t)(dec / TEMP_INC), dec);

	usleep(APML_SLEEP);
	ret = sbtsi_get_temp_status(soc_die_num, &lowalert, &hialert);
	if (ret)
		return ret;
	printf("_STATUS\t\t\t| 0x%x \t\t|\t\t| \n", SBTSI_STATUS);
	printf("\tPROC Temp Alert |\t\t|\t\t| ");
	if (lowalert)
		printf("PROC Temp Low Alert\n");
	else if (hialert)
		printf("PROC Temp Hi Alert\n");
	else
		printf("PROC No Temp Alert\n");

	if (status) {
		ret = get_hbm_temp_status(soc_die_num);
		if (ret)
			return ret;
	}

	usleep(APML_SLEEP);
	ret = sbtsi_get_config(soc_die_num, &al_mask, &run_stop,
			       &read_ord, &ara);
	if (ret)
		return ret;

	printf("_CONFIG\t\t\t| 0x%x \t\t|\t\t| \n", SBTSI_CONFIGURATION);
	printf("\tALERT_L pin\t|\t\t|\t\t| %s\n", al_mask ? "Disabled" : "Enabled");
	printf("\tRunstop\t\t|\t\t|\t\t| %s\n", run_stop ? "Comparison Disabled" :
	       "Comparison Enabled");
	printf("\tAtomic Rd order |\t\t|\t\t| %s\n", read_ord ? "Decimal Latches "
	       "Integer" : "Integer latches Decimal");
	if (!status)
		printf("\tARA response\t|\t\t|\t\t| %s\n", ara ? "Disabled"
		       : "Enabled");

	usleep(APML_SLEEP);
	ret = read_sbtsi_updaterate(soc_die_num, &uprate);
	if (ret)
		return ret;
	printf("_TSI_UPDATERATE \t| 0x%x \t\t|\t\t| %.3f Hz\n", SBTSI_UPDATERATE,
	       uprate);

	usleep(APML_SLEEP);
	ret = sbtsi_get_hitemp_threshold(soc_die_num, &temp_value[1]);
	if (ret)
		return ret;

	usleep(APML_SLEEP);
	ret = read_sbtsi_hitempint(soc_die_num, &intr);
	if (ret)
		return ret;

	usleep(APML_SLEEP);
	ret = read_sbtsi_hitempdecimal(soc_die_num, &dec);
	if (ret)
		return ret;

	printf("_HIGH_THRESHOLD_TEMP\t|\t\t|\t\t| %.3f °C\n", temp_value[1]);
	printf("\tHIGH_INT \t| 0x%x \t\t| 0x%-5x\t| %u °C\n", SBTSI_HITEMPINT,
	       intr, intr);
	printf("\tHIGH_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n", SBTSI_HITEMPDEC,
	       (uint8_t)(dec / TEMP_INC), dec);

	usleep(APML_SLEEP);
	ret = sbtsi_get_lotemp_threshold(soc_die_num, &temp_value[2]);
	if (ret)
		return ret;

	usleep(APML_SLEEP);
	ret = read_sbtsi_lotempint(soc_die_num, &intr);
	if (ret)
		return ret;
	ret = read_sbtsi_lotempdecimal(soc_die_num, &dec);
	if (ret)
		return ret;
	printf("_LOW_THRESHOLD_TEMP\t|\t\t|\t\t| %.3f °C\n", temp_value[2]);
	printf("\tLOW_INT \t| 0x%x \t\t| 0x%-5x\t| %u °C\n", SBTSI_LOTEMPINT,
	       intr, intr);
	printf("\tLOW_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n", SBTSI_LOTEMPDEC,
	       (uint8_t)(dec / TEMP_INC), dec);

	if (status)
	{
		ret = get_apml_mi300_tsi_register_descriptions(soc_die_num);
		if (ret)
			return ret;
	}

	ret = read_sbtsi_cputempoffset(soc_die_num, &dec);
	if (ret)
		return ret;
	printf("_TEMP_OFFSET\t\t|\t\t|\t\t| %.3f °C\n", dec);

	usleep(APML_SLEEP);
	ret = read_sbtsi_cputempoffint(soc_die_num, &intr_offset);
	if (ret)
		return ret;

	usleep(APML_SLEEP);
	ret = read_sbtsi_cputempoffdec(soc_die_num, &dec);
	if (ret)
		return ret;
	printf("\tOFF_INT \t| 0x%x \t\t| 0x%-5x\t| %u °C\n",
	       SBTSI_CPUTEMPOFFINT, intr_offset, intr_offset);
	printf("\tOFF_DEC \t| 0x%x \t\t| 0x%-5x\t| %.3f °C\n",
	       SBTSI_CPUTEMPOFFDEC, (uint8_t)(dec / TEMP_INC), dec);

	usleep(APML_SLEEP);
	if (!status) {
		ret = sbtsi_get_timeout(soc_die_num, &timeout);
		if (ret)
			return ret;
		printf("_TIMEOUT_CONFIG \t| 0x%x \t\t|\t\t| %s\n",
		       SBTSI_TIMEOUTCONFIG, timeout ? "Enabled" : "Disabled");
	}
	usleep(APML_SLEEP);
	ret = read_sbtsi_alertthreshold(soc_die_num, &buf);
	if (ret)
		return ret;
	printf("_THRESHOLD_SAMPLE\t| 0x%x \t\t|\t\t| \n",
	       SBTSI_ALERTTHRESHOLD);
	printf("\tPROC Alert TH \t|\t\t|\t\t| %u\n", buf);
	if (status) {
		ret = read_sbtsi_hbm_alertthreshold(soc_die_num, &buf);
		if (ret)
			return ret;
		printf("\tHBM Alert TH \t|\t\t|\t\t| %u\n", buf);
	}

	usleep(APML_SLEEP);
	ret = read_sbtsi_alertconfig(soc_die_num, &buf);
	if (ret)
		return ret;
	printf("_TSI_ALERT_CONFIG\t| 0x%x \t\t|\t\t| \n",
	       SBTSI_ALERTCONFIG);
	printf("\tPROC Alert CFG \t|\t\t|\t\t| %s\n",
	       buf ? "Enabled" : "Disabled");
	if (status) {
		usleep(APML_SLEEP);
		ret = get_sbtsi_hbm_alertconfig(soc_die_num, &buf);
		if (ret)
			return ret;
		printf("\tHBM Alert CFG \t|\t\t|\t\t| %s\n",
		       buf ? "Enabled" : "Disabled");
	}

	usleep(APML_SLEEP);
	ret = read_sbtsi_manufid(soc_die_num, &id);
	if (ret)
		return ret;
	printf("_TSI_MANUFACTURE_ID\t| 0x%x \t\t|\t\t| %#x\n", SBTSI_MANUFID, id);

	usleep(APML_SLEEP);
	ret = read_sbtsi_revision(soc_die_num, &id);
	if (ret)
		return ret;
	printf("_TSI_REVISION \t\t| 0x%x \t\t|\t\t| %#x\n", SBTSI_REVISION, id);

	printf("------------------------------------------------------------"
	       "-----------------------\n");
	return OOB_SUCCESS;
}

static oob_status_t get_apml_tsi_access(uint8_t soc_die_num)
{
	oob_status_t ret;

	printf("------------------------------------------------------------"
		"----\n");
	ret = get_apml_tsi_register_descriptions(soc_die_num);
	if (ret)
		printf("Failed: TSI Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
	return ret;
}

static void apml_set_dimm_power(uint8_t soc_die_num, struct dimm_power d_soc_die_num)
{
	uint32_t buffer;
	oob_status_t ret;

	ret = write_bmc_report_dimm_power(soc_die_num, d_soc_die_num);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set dimm power, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}

	printf("Dimm power set successfully\n");
}

static int encode_dimm_temp(float temp, uint16_t *raw)
{
	if (temp >= 0 && temp <= 255.75)
		*raw = temp / SCALING_FACTOR;
	else if (temp < 0 && temp >= -256)
		*raw = 0x800 + (temp / SCALING_FACTOR);
	else
		return OOB_INVALID_INPUT;
	return 0;

}

static void apml_set_thermal_sensor(uint8_t soc_die_num,
				    struct dimm_thermal d_soc_die_num,
				    float temp)
{
	uint32_t buffer;
	uint16_t raw;
	oob_status_t ret;

	ret = encode_dimm_temp(temp, &raw);
	if (ret) {
		printf("Error: Temperature value out of range\n");
		return;
	}

	d_soc_die_num.sensor = raw;
	ret = write_bmc_report_dimm_thermal_sensor(soc_die_num, d_soc_die_num);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set dimm  thermal sensor, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("Dimm thermal sensor set successfully\n");
}

static void apml_get_ras_pcie_config_data(uint8_t soc_die_num,
					  struct pci_address pci_addr)
{
	uint32_t buffer;
	oob_status_t ret;

	ret = read_bmc_ras_pcie_config_access(soc_die_num, pci_addr, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get data from PCIe config space, Err[%d]:"
		       "%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------\n");
	printf("| Data PCIe | 0x%-17x |\n", buffer);
	printf("-----------------------------------\n");
}

static void apml_set_bmc_pcie_config(uint8_t soc_die_num,
				     struct pci_address pci_addr,
				     uint32_t pcie_data)
{
	uint32_t response = 0;
	oob_status_t ret;

	ret = write_bmc_pcie_config(soc_die_num, pci_addr, pcie_data, &response);
	if (ret) {
		printf("Failed to write data to config space,Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	if (!response)
		printf("Data to PCIE config space set successfully\n");
}

static void apml_get_ras_valid_mca_banks(uint8_t soc_die_num)
{
	oob_status_t ret;
	uint8_t p_type = 0;

	ret = get_proc_type(soc_die_num, &p_type);
	if (ret) {
		printf("Failed to get processor type for soc_die_num %d,"
		       "error: %d\n", soc_die_num, ret);
		return;
	}

	if (p_type == FAM_1A_MOD_50) {
		struct ras_df_err_chk err_chk;

		ret = read_ras_df_err_validity_check(soc_die_num, MCA_DEBUG_LOG_ID, &err_chk);
		if (ret && ret != OOB_MAILBOX_ADD_ERR_DATA) {
			printf("Failed to get MCA banks with valid status"
			       "after a fatal error Err[%d]:%s\n",
			       ret, esmi_get_err_msg(ret));
			return;
		}

		printf("----------------------------------------------------\n");
		if (ret == OOB_MAILBOX_ADD_ERR_DATA) {
			printf("| MB error:0x%x additional error data | 0x%x|\n",
			       ret, err_chk.add_err_data);
		} else {
			printf("| Valid MCA banks |");
			printf(" %-17u |\n", err_chk.df_block_instances);
			printf("| Bytes per bank  |");
			printf(" %-17u |\n", err_chk.err_log_len);
		}
		printf("----------------------------------------------------\n");

	} else {
		uint16_t bytespermca;
		uint16_t numbanks;
		ret = read_bmc_ras_mca_validity_check(soc_die_num,
						      &bytespermca, &numbanks);
		if (ret != OOB_SUCCESS) {
			printf("Failed to get MCA banks with valid status "
			       "after a fatal error, Err[%d]:%s\n",
			       ret, esmi_get_err_msg(ret));
			return;
		}
		printf("---------------------------------------\n");
		printf("| Valid MCA banks |");
		printf(" %-17u |\n", numbanks);
		printf("| Bytes per bank  |");
		printf(" %-17u |\n", bytespermca);
		printf("---------------------------------------\n");
	}
}

static void apml_gpio_assertion_on_mailbox(uint8_t soc_die_num, uint8_t disable_alert_l_signal)
{
	oob_status_t ret;

	ret = gpio_assertion_on_mailbox(soc_die_num, disable_alert_l_signal);
	if (ret != OOB_SUCCESS) {
		printf("Failed to assert GPIO through mailbox, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("GPIO assertion through mailbox is %s successfully\n", disable_alert_l_signal ? "disabled" : "enabled");
}

static void apml_gpio_assertion_for_async_alerts(uint8_t soc_die_num, uint8_t dis_alert)
{
	oob_status_t ret;

	ret = gpio_assertion_for_async_alerts(soc_die_num, dis_alert);
	if (ret != OOB_SUCCESS) {
		printf("Failed to assert GPIO for async alerts, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("GPIO assertion for async alerts is %s successfully\n", dis_alert ? "disabled" : "enabled");
}

static void apml_get_ras_mca_msr(uint8_t soc_die_num, struct mca_bank mca_dump)
{
	oob_status_t ret;
	uint8_t p_type = 0;

	ret = get_proc_type(soc_die_num, &p_type);
	if (ret) {
		printf("Failed to get processor type for soc_die_num %d,"
		       "error: %d\n", soc_die_num, ret);
		return;
	}

	if (p_type == FAM_1A_MOD_50) {
		union ras_df_err_dump df_err;
		uint32_t data = 0;

		df_err.input[0] = mca_dump.offset;
		df_err.input[1] = MCA_DEBUG_LOG_ID;
		df_err.input[2] = mca_dump.index;

		ret = read_ras_df_err_dump(soc_die_num, df_err, &data);
		if (ret) {
			printf("Failed to get MCA bank data offset[%d] "
			       "Err[%d]:%s\n", df_err.input[0], ret,
			esmi_get_err_msg(ret));
			return;
		}
		printf("-----------------------------------------------\n");
		printf("| Data MCA bank | 0x%-17x |\n", data);
		printf("-----------------------------------------------\n");
	} else {
		uint32_t buffer;
		ret = read_bmc_ras_mca_msr_dump(soc_die_num, mca_dump, &buffer);
		if (ret != OOB_SUCCESS) {
			printf("Failed to get MCA bank data, Err[%d]:%s\n",
			       ret, esmi_get_err_msg(ret));
			return;
		}
		printf("---------------------------------------\n");
		printf("| Data MCA bank | 0x%-17x |\n", buffer);
		printf("---------------------------------------\n");
	}
}

static void apml_get_fch_reset_reason(uint8_t soc_die_num, uint32_t fchid)
{
	uint32_t buffer;
	char *fch_status;
	oob_status_t ret;

	fch_status = fchid ? "FCH Previous Breakevent"
		     : "FCH Previous S5 reset status";
	ret = read_bmc_ras_fch_reset_reason(soc_die_num, fchid, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get previous reset reason, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-------------------------------------------------------\n");
	printf("| %-30s | 0x%-17x |\n", fch_status, buffer);
	printf("-------------------------------------------------------\n");
}

static void apml_get_temp_range_and_refresh_rate(uint8_t soc_die_num,
						 uint8_t dimm_addr)
{
	struct temp_refresh_rate rate;
	oob_status_t ret;

	ret = read_dimm_temp_range_and_refresh_rate(soc_die_num, dimm_addr, &rate);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get dimm temp range and refresh rate, "
			"Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| Range\t\t\t |");
	printf(" %-17u |\n", rate.range);
	printf("| Refresh rate\t\t |");
	printf(" %-17u |\n", rate.ref_rate);
	printf("----------------------------------------------\n");
}

static void apml_get_hottest_dimm_temp_range_ref_rate(uint8_t soc_die_num)
{
	struct hottest_dimm_temp_refresh_rate rate = {0};
	oob_status_t ret;

	ret = get_hottest_dimm_temp_range_ref_rate(soc_die_num, &rate);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get hottest dimm address, temp range and refresh rate, "
			"Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| DIMM Addr\t\t | 0x%-15x |\n", rate.dimm_addr);
	printf("| Range\t\t\t | %-17u |\n", rate.range);
	printf("| Refresh rate\t\t | %-17u |\n", rate.ref_rate);
	printf("----------------------------------------------\n");
}

static void apml_get_dimm_power(uint8_t soc_die_num, uint8_t dimm_addr)
{
	struct dimm_power d_power;
	oob_status_t ret;

	ret = read_dimm_power_consumption(soc_die_num, dimm_addr, &d_power);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get dimm power, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| DIMM Power (mW)\t |");
	printf(" %-17u |\n", d_power.power);
	printf("| Update rate (ms)\t |");
	printf(" %-17u |\n", d_power.update_rate);
	printf("----------------------------------------------\n");
}

static void apml_get_per_dimm_power(uint8_t soc_die_num, struct dimm_pow_din d_in)
{
	struct dimm_pow_dout d_out = {0};
	char *dimm_str;
	oob_status_t ret;

	ret = get_dimm_pow_data(soc_die_num, d_in, &d_out);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get DIMM power, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	/* data */
	switch (d_in.pow_reporting_flag) {
		case 0:
			dimm_str = " DIMM Power (mW)";
			break;
		case 1:
			dimm_str = " DIMM Power Highest (mW)";
			break;
		case 2:
			dimm_str = " Total Power consumption (mW)";
			break;
		default:
			dimm_str = " Invalid Flag";
			break;
	}
	printf("------------------------------------------------------\n");
	printf("|%-30s\t | %-17u |\n", dimm_str, d_out.power);
	printf("| Update rate (ms)\t\t | %-17u |\n", d_out.update_rate);
	if (d_in.pow_reporting_flag != ALL_DIMM_REPORTING_FLAG)
		printf("| %-18s \t\t | 0x%-15x |\n", "DIMM ADDR", d_out.dimm_addr);
	printf("------------------------------------------------------\n");
}

static void apml_write_pcie_link_control(uint8_t soc_die_num, uint32_t pcie_link_control)
{
	oob_status_t ret;

	ret = write_pcie_link_control(soc_die_num, pcie_link_control);
	if (ret) {
		printf("Failed to %s the PCIe link control "
		       "Err[%d]:%s\n", pcie_link_control ? "disable" : "enable",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("PCIe link control %s successfully\n", pcie_link_control ? "disabled" : "enabled");
}

static void decode_dimm_temp(uint16_t raw, float *temp)
{
	if (raw <= 0x3FF)
		*temp = raw * SCALING_FACTOR;
	else
		*temp = (raw - 0x800) * SCALING_FACTOR;
}

static void apml_get_hottest_dimm_temp(uint8_t soc_die_num)
{
	struct dimm_thermal d_sensor = {0};
	struct dimm_thermal_din d_in = {0};
	oob_status_t ret;
	float temp;

	d_in.dimm_addr = 0;
	d_in.thermal_flag = 1;
	ret = get_dimm_thermal_sensor_data(soc_die_num, d_in, &d_sensor);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get hottest DIMM temp, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	decode_dimm_temp(d_sensor.sensor, &temp);

	printf("-------------------------------------------------------\n");
	printf("| Hottest Dimm Temp (ºC)(raw)\t | %-10.3f(0x%-4x) |\n", temp,
	       d_sensor.sensor);
	printf("| Update rate (ms)\t\t | %-17u  |\n", d_sensor.update_rate);
	printf("| DIMM Addr      \t\t | 0x%-17x|\n", d_sensor.dimm_addr);
	printf("-------------------------------------------------------\n");
}

static void apml_get_dimm_temp(uint8_t soc_die_num, uint8_t dimm_addr)
{
	struct dimm_thermal d_sensor;
	oob_status_t ret;
	float temp;

	ret = read_dimm_thermal_sensor(soc_die_num, dimm_addr, &d_sensor);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get dimm temp, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	decode_dimm_temp(d_sensor.sensor, &temp);
	printf("-----------------------------------------------\n");
	printf("| DIMM Temp (ºC)(raw)\t |");
	printf(" %-10.3f(0x%-4x) |\n", temp, d_sensor.sensor);
	printf("| Update rate (ms)\t |");
	printf(" %-17u  |\n", d_sensor.update_rate);
	printf("-----------------------------------------------\n");
}

static void display_freq_limit_src_names(char **source_type)
{
	uint8_t index = 0;

	while (source_type[index]) {
		printf(" %-24s ", source_type[index]);
		index++;
	}
	if (index == 0)
		printf(" %-24s ", "Reserved");
}

static void apml_get_freq_limit(uint8_t soc_die_num)
{
	uint16_t freq, source_len = 0;
	uint8_t p_type = 0;
	oob_status_t ret;

	ret = get_proc_type(soc_die_num, &p_type);
	if (ret) {
		printf("------------------------------------------------------\n");
		printf("Platform Identification failed.\n");
		printf("------------------------------------------------------\n");
		return;
	}

	if (p_type == FAM_1A_MOD_50)
		source_len = ARRAY_SIZE(freqlimitsrcnames_VER1);
	else
		source_len = ARRAY_SIZE(freqlimitsrcnames);
	char *source_type[source_len];

	memset(source_type, 0, sizeof(source_type));
	ret = read_pwr_current_active_freq_limit_socket(soc_die_num,
							&freq, source_type);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get socket freq limit, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-------------------------------------------------------------\n");
	printf("| Frequency (MHz)\t\t |");
	printf(" %-22u   |\n", freq);
	printf("| Source\t\t\t |");
	display_freq_limit_src_names(source_type);
	printf("|\n");
	printf("-------------------------------------------------------------\n");
}

static void apml_get_cclklimit(uint8_t soc_die_num, uint32_t thread)
{
	uint16_t buffer;
	oob_status_t ret;

	ret = read_pwr_current_active_freq_limit_core(soc_die_num,
						      thread, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get core freq limit, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| Frequency [%03u] (MHz)\t | %-17u |\n", thread, buffer);
	printf("----------------------------------------------\n");
}

static void apml_get_pwr_telemetry(uint8_t soc_die_num)
{
	uint32_t power;
	oob_status_t ret;

	ret = read_pwr_svi_telemetry_all_rails(soc_die_num, &power);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get svi based telemetry "
		       "for all rails, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("------------------------------------------------------------"
		"--\n");
	printf("| Telemetry Power (Watts)\t\t | %-17.03f |\n",
	       (float)power / 1000);
	printf("------------------------------------------------------------"
		"--\n");
}

static void apml_get_sock_freq_range(uint8_t soc_die_num)
{
	uint16_t fmax;
	uint16_t fmin;
	oob_status_t ret;

	ret = read_socket_freq_range(soc_die_num, &fmax, &fmin);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get Fmax and Fmin, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| Fmax (MHz)\t\t |");
	printf(" %-17u |\n", fmax);
	printf("| Fmin (MHz)\t\t |");
	printf(" %-17u |\n", fmin);
	printf("----------------------------------------------\n");
}

static void convert_to_upper_case(char *str)
{
	uint8_t index;

	for (index = 0; str[index]; index++) {
		if (str[index] >= 'a' && str[index] <= 'z')
			str[index] -= 32;
	}
}

static void convert_to_lower_case(char *str)
{
	uint8_t index;

	for (index = 0; str[index]; index++) {
		if (str[index] >= 'A' && str[index] <= 'Z')
			str[index] += 32;
	}
}

static oob_status_t validate_legacy_link_id(uint8_t size, char *link_id,
					    struct link_id_bw_type *link)
{
	uint8_t index = 0;

	for (index = 0; index < size; index++) {
		 if (strcmp(link_id, encodings[index].name) == 0) {
			 link->link_id = encodings[index].val;
			 return OOB_SUCCESS;
		 }
	}

	return OOB_INVALID_INPUT;
}

static oob_status_t validate_mi300A_link_id(uint8_t size, char *link_id,
					    struct link_id_bw_type *link)
{
	uint8_t index = 0;

	for (index = 0; index < size; index++) {
		if (strcmp(link_id, mi300A_encodings[index].name) == 0) {
			link->link_id = mi300A_encodings[index].val;
			return OOB_SUCCESS;
		}
	}

	return OOB_INVALID_INPUT;
}

static oob_status_t validate_sp7_link_id(char *link_id, uint16_t *link_id_val)
{
       uint8_t index = 0;
       uint8_t size = 0;

       size = ARRAY_SIZE(sp7_encodings);
       for (index = 0; index < size; index++) {
               if (strcmp(link_id, sp7_encodings[index].name) == 0) {
                       *link_id_val = sp7_encodings[index].val;
                       return OOB_SUCCESS;
               }
       }

       return OOB_INVALID_INPUT;
}

static oob_status_t validate_bw_link_id(uint8_t soc_die_num, char *link_id,
					char *bw_type, bool is_xgmi_bw,
					struct link_id_bw_type *link)
{
	const char *bw_type_list[3] = {"AGG_BW", "RD_BW", "WR_BW"};
	const char *io_bw_type = "AGG_BW";
	uint8_t index, arr_size, p_type = 0;
	oob_status_t ret;
	bool is_mi300 = false;

	ret = get_proc_type(soc_die_num, &p_type);
	if (p_type == NOT_SUPPORTED)
		return ret;

	if (p_type == FAM_19_MOD_90)
		is_mi300 = true;

	link->bw_type = 0;
	link->link_id = 0;
	convert_to_upper_case(link_id);
	convert_to_upper_case(bw_type);

	if (is_xgmi_bw)
		arr_size = ARRAY_SIZE(bw_type_list);
	else
		arr_size = 1;

	if (!is_xgmi_bw) {
		if (strcmp(bw_type, io_bw_type) == 0)
			link->bw_type = 1;
	} else {
		for (index = 0; index < arr_size; index++) {
			if (strcmp(bw_type, bw_type_list[index]) == 0) {
				link->bw_type = 1 << index;
				break;
			}
		}
	}

	if (is_mi300) {
		arr_size = ARRAY_SIZE(mi300A_encodings);
		return validate_mi300A_link_id(arr_size, link_id, link);
	} else if (p_type == FAM_1A_MOD_50) {
		return validate_sp7_link_id(link_id, &link->link_id);
	} else {
		arr_size = ARRAY_SIZE(encodings);
		return validate_legacy_link_id(arr_size, link_id, link);
	}
}

static void apml_set_pcie_link_training(uint8_t soc_die_num, char *link_id,
					uint32_t pcie_port, uint32_t eom)
{
	struct pcie_link_training d_in = {0};
	uint32_t d_out = 0;
	uint16_t link_id_val = 0;
	oob_status_t ret;

	if (pcie_port > UINT8_MAX || eom > 1) {
		printf("Err[%d]:%s\n", OOB_INVALID_INPUT, esmi_get_err_msg(OOB_INVALID_INPUT));
		return;
	}

	convert_to_upper_case(link_id);
	ret = validate_sp7_link_id(link_id, &link_id_val);
	if (ret) {
		printf("Not valid link ID, Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	d_in.link_id = link_id_val;
	d_in.pcie_port_mask = pcie_port;
	d_in.eom = eom;

	ret = set_pcie_link_training(soc_die_num, d_in, &d_out);
	if (ret) {
		printf("Failed to Halt PCIe Link Training for OOB device authentication, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	if (eom) {
		printf("------------------------------------------------------------\n");
		printf("| Device Authentication Status | %-25s |\n", d_out ? "Not Authenticated"
		       : "Authentications Success");
		printf("------------------------------------------------------------\n");
	} else {
		printf("--------------------------------------------------------\n");
		printf("| Device Acceptance Status | %-25s |\n", d_out ? "Rejected"
		       : "Accepted");
		printf("--------------------------------------------------------\n");
	}
}

static void apml_get_iobandwidth(uint8_t soc_die_num, char *link_id,
				 char *bw_type)
{
	struct link_id_bw_type link;
	uint32_t buffer;
	oob_status_t ret;

	ret = validate_bw_link_id(soc_die_num, link_id, bw_type, false, &link);
	if (ret) {
		printf("Failed to get current IO bandwidth, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	ret = read_current_io_bandwidth(soc_die_num, link, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get current IO bandwidth, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| IO bandwidth (Mbps)\t | %-17u |\n", buffer);
	printf("----------------------------------------------\n");
}

static void apml_get_xgmibandwidth(uint8_t soc_die_num, char *link_id,
				   char *bw_type)
{
	struct link_id_bw_type link;
	uint32_t buffer;
	oob_status_t ret;

	ret = validate_bw_link_id(soc_die_num, link_id, bw_type, true, &link);
	if (ret) {
		printf("Failed to get current bandwidth on xGMI link, "
		       "Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	ret = read_current_xgmi_bandwidth(soc_die_num, link, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get current  bandwidth on xGMI link, "
			"Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-------------------------------------------------------"
		"-------\n");
	printf("| xGMI Bandwidth (Mbps)\t\t\t | %-17u |\n",
		buffer);
	printf("--------------------------------------------------------"
		"------\n");
}

static void apml_set_gmi3link_width(uint8_t soc_die_num,
				    uint16_t minwidth,
				    uint16_t maxwidth)
{
	oob_status_t ret;

	ret = write_gmi3_link_width_range(soc_die_num, minwidth,
					  maxwidth);
	if (ret != OOB_SUCCESS) {
		printf("Failed to write GMI3 link width range, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("GMI3 link width set successfully\n");
}

static void apml_set_xgmilink_width(uint8_t soc_die_num,
				    uint16_t minwidth,
				    uint16_t maxwidth)
{
	oob_status_t ret;

	ret = write_xgmi_link_width_range(soc_die_num, minwidth,
					  maxwidth);
	if (ret != OOB_SUCCESS) {
		printf("Failed to write xGMI link width range, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("xGMI link width set successfully\n");
}

static void apml_set_dfpstate(uint8_t soc_die_num, uint8_t pstate)
{
	oob_status_t ret;
	bool prochot_asserted = false;

	ret = write_apb_disable(soc_die_num, pstate, &prochot_asserted);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set data fabric pstate, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	if (prochot_asserted)
		printf("PROCHOT_L is asserted,"
		       " lowest DF-Pstate is enforced.\n");
	else
		printf("Data fabric pstate set successfully\n");
}

static void apml_get_fclkmclkuclk(uint8_t soc_die_num)
{
	struct pstate_freq df_pstate;
	uint8_t clock_divider = 0, p_type = 0;
	oob_status_t ret;

	ret = read_current_dfpstate_frequency(soc_die_num, &df_pstate);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get data fabric clock, "
			"memory clock and UMC clock divider, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	ret = get_proc_type(soc_die_num, &p_type);
	if (p_type == FAM_1A_MOD_50)
		clock_divider = df_pstate.uclk ? BIT(2) : BIT(1);
	else
		clock_divider = df_pstate.uclk ? BIT(1) : BIT(0);

	printf("----------------------------------------------\n");
	printf("| FCLK (MHz)\t\t |");
	printf(" %-17u |\n", df_pstate.fclk);
	printf("| MEMCLK (MHz)\t\t |");
	printf(" %-17u |\n", df_pstate.mem_clk);
	printf("| UCLK (MHz)\t\t |");
	printf(" %-17u |\n", df_pstate.mem_clk / clock_divider);
	printf("----------------------------------------------\n");
}

static void apml_apb_enable(uint8_t soc_die_num)
{
	oob_status_t ret;
	bool prochot_asserted = false;

	ret = write_apb_enable(soc_die_num, &prochot_asserted);
	if (ret != OOB_SUCCESS) {
		printf("Failed to write apb enable, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	if (prochot_asserted)
		printf("PROCHOT_L is asserted,"
		       " lowest DF-Pstate is enforced.\n");
	else
		printf("Successfully set to dynamic data fabric pstate"
		       " control\n");
}

static void apml_set_lclk_dpm_level(uint8_t soc_die_num,
				    struct lclk_dpm_level_range lclk)
{
	oob_status_t ret;

	ret = write_lclk_dpm_level_range(soc_die_num, lclk);
	if (ret != OOB_SUCCESS) {
		printf("Failed to write dpm level, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("Lclk dpm level set successfully\n");
}

static void apml_get_cpu_base_freq(uint8_t soc_die_num)
{
	uint16_t buffer;
	oob_status_t ret;

	ret = read_bmc_cpu_base_frequency(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get cpu base freq, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------\n");
	printf("| Frequency (MHz) | %-17u |\n", buffer);
	printf("---------------------------------------\n");
}

static void apml_set_pciegen5_control(uint8_t soc_die_num, uint8_t val)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = read_bmc_control_pcie_gen5_rate(soc_die_num, val, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to write PCIegen5 rate control, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| Previous Mode\t\t | %-17u |\n", buffer);
	printf("----------------------------------------------\n");

}

static void apml_set_pwr_efficiency_mode(uint8_t soc_die_num, uint8_t mode)
{
	oob_status_t ret;

	ret = write_pwr_efficiency_mode(soc_die_num, mode);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set pwr efficiecy profile policy, "
			"Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("Mode set successfully\n");
}

static void apml_get_core_energy(uint8_t soc_die_num, uint32_t thread)
{
	double buffer;
	oob_status_t ret;

	ret = read_rapl_core_energy_counters(soc_die_num, thread,
					     &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get core energy, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("----------------------------------------------\n");
	printf("| Core[%03d] Energy (KJ)\t | %-17lf |\n", thread, buffer);
	printf("----------------------------------------------\n");
}

static void apml_get_pkg_energy(uint8_t soc_die_num)
{
	double buffer;
	oob_status_t ret;

	ret = read_rapl_pckg_energy_counters(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get package energy, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------------------------\n");
	printf("| Package energy (MJ)\t\t | %-17lf |\n", buffer);
	printf("-----------------------------------------------------\n");
}

static void apml_set_df_pstate_range(uint8_t soc_die_num, uint8_t max_pstate,
				     uint8_t min_pstate)
{
	oob_status_t ret;

	ret = write_df_pstate_range(soc_die_num, max_pstate, min_pstate);
	if (ret != OOB_SUCCESS) {
		printf("Failed to set data fabric pstate range, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("Data fabric pstate range set successfully\n");
}

static void read_register(uint8_t soc_die_num, uint32_t reg, char *file_name)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = esmi_oob_read_byte(soc_die_num, reg, file_name, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to read register %x, Err[%d]:%s\n",
		       reg, ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------");
	printf("\n| Register \t| Value \t|");
	printf("\n---------------------------------");
	printf("\n| 0x%-8x \t| 0x%x \t\t|", reg, buffer);
	printf("\n---------------------------------\n");
}

static void write_register(uint8_t soc_die_num, uint32_t reg, char *file_name,
			   uint32_t value)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = esmi_oob_write_byte(soc_die_num, reg, file_name, value);
	if (ret != OOB_SUCCESS) {
		printf("Failed to writeregister %x, Err[%d]:%s\n",
		       reg, ret, esmi_get_err_msg(ret));
		return;
	}
	printf("Write to register 0x%x is successful\n", reg);
}

static void read_rmi_register(uint8_t soc_die_num, uint32_t reg)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = esmi_oob_rmi_read_byte(soc_die_num, reg, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to read rmi register %x, Err[%d]:%s\n",
		       reg, ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------");
	printf("\n| Register \t| Value \t|");
	printf("\n---------------------------------");
	printf("\n| 0x%x \t\t| 0x%x \t\t|", reg, buffer);
	printf("\n---------------------------------\n");
}

static void read_tsi_register(uint8_t soc_die_num, uint32_t reg)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_die_num, reg, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to read tsi register %x, Err[%d]:%s\n",
		       reg, ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------");
	printf("\n| Register \t| Value \t|");
	printf("\n---------------------------------");
	printf("\n| 0x%-8x \t| 0x%x \t\t|", reg, buffer);
	printf("\n---------------------------------\n");
}

static void read_tsi_raw_register(uint8_t soc_die_num, uint32_t reg)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = esmi_oob_tsi_read_byte(soc_die_num, reg, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to read tsi register %x, Err[%d]:%s\n",
		       reg, ret, esmi_get_err_msg(ret));
		return;
	}
	printf("%02x\n", buffer);
}

static void write_rmi_register(uint8_t soc_die_num, uint32_t reg,
			       uint32_t value)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = esmi_oob_rmi_write_byte(soc_die_num, reg, value);
	if (ret != OOB_SUCCESS) {
		printf("Failed to write rmi register %x, Err[%d]:%s\n",
		       reg, ret, esmi_get_err_msg(ret));
		return;
	}
	printf("Write to register 0x%x is successful\n", reg);
}

static void write_tsi_register(uint8_t soc_die_num, uint32_t reg,
			       uint32_t value)
{
	uint8_t buffer;
	oob_status_t ret;

	ret = esmi_oob_tsi_write_byte(soc_die_num, reg, value);
	if (ret != OOB_SUCCESS) {
		printf("Failed to write tsi register %x, Err[%d]:%s\n",
		       reg, ret, esmi_get_err_msg(ret));
		return;
	}
	printf("Write to register 0x%x is successful\n", reg);
}
static void read_msr_register(uint8_t soc_die_num, uint32_t addr,
			      uint32_t thread)
{
	uint64_t buffer;
	oob_status_t ret;

	ret = esmi_oob_read_msr(soc_die_num, thread, addr, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to read MSR register, Err[%d]:%s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------\n");
	printf("| MSR register \t| Value\t\t\t|\n");
	printf("-----------------------------------------\n");
	printf("| [0x%08x]  | 0x%-17" PRIx64"\t|\n", addr, buffer);
	printf("-----------------------------------------\n");
}

static void read_cpuid_register(uint8_t soc_die_num, uint32_t func,
				uint32_t ex_func, uint32_t thread)
{
	uint32_t eax, ebx, ecx, edx;
	oob_status_t ret;

	eax = func;
	ecx = ex_func;

	ret = esmi_oob_cpuid(soc_die_num, thread, &eax, &ebx,
			     &ecx, &edx);
	if (ret != OOB_SUCCESS) {
		printf("Failed to read CPUID register[0x%x][0x%x], "
			"Err[%d]:%s\n", func, ex_func, ret,
			esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------------------------\n");
	printf("| CPUID register[0x%08x][0x%x]  | Value\t\t|\n", func, ex_func);
	printf("---------------------------------------------------------\n");
	printf("| \t\teax \t\t   | 0x%-17x|\n", eax);
	printf("| \t\tebx \t\t   | 0x%-17x|\n", ebx);
	printf("| \t\tecx \t\t   | 0x%-17x|\n", ecx);
	printf("| \t\tedx \t\t   | 0x%-17x|\n", edx);
	printf("---------------------------------------------------------\n");
}

static oob_status_t read_ccx_info(uint8_t soc_die_num,
				  uint16_t *max_cores_per_ccx,
				  uint16_t *ccx_instances)
{
	uint32_t threads_c, threads_s, threads_l3;
	oob_status_t ret;

	/* Get threads per core */
	ret = esmi_get_threads_per_core(soc_die_num, &threads_c);
	if (ret)
		return ret;

	/* Get maximum threads per l3 */
	ret = read_max_threads_per_l3(soc_die_num, &threads_l3);
	if (ret)
		return ret;

	/* Get Maximum threads per socket */
	ret = esmi_get_threads_per_socket(soc_die_num, &threads_s);
	if (ret)
		return ret;

	/* Max number of cores per ccx */
	*max_cores_per_ccx = threads_l3 / threads_c;
	/* Logical CCX instances */
	*ccx_instances = threads_s / threads_l3;

	return ret;
}

static void apml_get_iod_bist_status(uint8_t soc_die_num)
{
	uint32_t buffer;
	uint8_t p_type = 0;
	char *bist_status;
	oob_status_t ret;

	ret = read_iod_bist(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get the iod bist status, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	ret = get_proc_type(soc_die_num, &p_type);
	if (ret) {
		printf("Failed to get processor type for soc_die_num %d,"
		       "error: %d\n", soc_die_num, ret);
		return;
	}
	if (p_type == FAM_1A_MOD_50) {
		switch (buffer)
		{
		case 0:
			bist_status = "BIST pass on both IOD 0 and 1";
			break;
		case 1:
			bist_status = "BIST pass on IOD0";
			break;
		case 2:
			bist_status = "BIST pass on IOD1";
			break;
		case 3:
			bist_status = "BIST fail on both IOD 0 and 1";
			break;
		default:
			bist_status = "Undefined response";
			break;
		}
	} else {
		bist_status = buffer == 0 ? "BIST PASS" : "BIST FAIL";
	}
	printf("-------------------------------------------"
	       "---------------------\n");

	printf("| IOD/AID BIST STATUS | \t%-30s |\n",
	       bist_status);
	printf("-------------------------------------------"
	       "---------------------\n");

}

static void apml_get_ccd_bist_status(uint8_t soc_die_num, uint32_t instance)
{
	uint32_t buffer;
	oob_status_t ret;

	ret = read_ccd_bist_result(soc_die_num, instance, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get the ccd bist status, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-------------------------------------------\n");
	printf("| CCD/XCD BIST STATUS | \t%s |\n",
		buffer == 0 ? "BIST PASS" : "BIST FAIL");
	printf("-------------------------------------------\n");
}

static void apml_get_ccx_bist_status(uint8_t soc_die_num, uint32_t instance)
{
	uint32_t bist_res = 0, d_in = 0;
	uint16_t max_cores_per_ccx, ccx_instances;
	uint8_t index = 0, p_type = 0;
	bool status = false;
	oob_status_t ret;

	ret = read_ccx_bist_result(soc_die_num, instance, &bist_res);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get the ccx bist status, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	ret = get_proc_type(soc_die_num, &p_type);

	printf("---------------------------------\n");
	switch(p_type) {
	case NOT_SUPPORTED:
		printf("Platform identification failed\n");
		break;
	case LEGACY_PLATFORMS:
		printf("| CCX BIST RESULT | \t0x%-8x|\n", bist_res);
		break;
	case FAM_1A_MOD_00:
	case FAM_1A_MOD_10:
	case FAM_19_MOD_10:
	case FAM_19_MOD_90:
	case FAM_19_MOD_A0:
		if(p_type == FAM_19_MOD_90 )
			status = true;

		ret = read_ccx_info(soc_die_num, &max_cores_per_ccx,
				    &ccx_instances);
		if (ret) {
			printf("Failed to get the CCX info, Err[%d]:%s\n",
			       ret, esmi_get_err_msg(ret));
			return;
		}
		printf("| L3 BIST \t| %s\t|\n",
		       bist_res & 1 ? "Bist pass": "Bist fail");
		printf(status ? "" : "| L3 X3D  \t| %s\t|\n",
		       (status ? "" : (extract_val(bist_res, BIT(0)) & MASK(1)
		       ? "Bist pass": "Bist fail")));
		for (index = 0; index < max_cores_per_ccx; index++)
			printf("| CORE[%d] \t| %s\t|\n", index,
			       ((bist_res >> (index + 16)) & 1)
			       ? "Bist pass": "Bist fail");
		break;
	case FAM_1A_MOD_50:
				for (index = 0; index < 32; index++) {
					printf("| CORE[%d] \t| %s\t|\n", index,
							((bist_res >> index) & 1)
							? "Bist pass": "Bist fail");
				}
				// Get L3 BIST result
				d_in = shift_left_op(1, 31) | instance;
				ret = read_ccx_bist_result(soc_die_num, d_in, &bist_res);
				if (ret != OOB_SUCCESS) {
					printf("Failed to get the L3 bist status, Err[%d]:%s\n",
						ret, esmi_get_err_msg(ret));
					return;
				}
				printf("| L3 BIST \t| %s\t|\n",
						bist_res & 1 ? "Bist pass": "Bist fail");
				printf("| L3 X3D  \t| %s\t|\n",
				       ((bist_res & BIT(1)) >> 1) ? "Bist pass": "Bist fail");
		break;
	default:
		printf("| CCX BIST RESULT | \t0x%-8x|\n", bist_res);
		break;
	}
	printf("---------------------------------\n");
}

static void apml_get_nbio_error_log_reg(uint8_t soc_die_num,
					struct nbio_err_log nbio)
{
	uint32_t buffer;
	oob_status_t ret;

	ret = read_nbio_error_logging_register(soc_die_num, nbio, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get the nbio error log register,"
			"Err[%d]:%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------\n");
	printf("| NBIO ERROR LOG REG | \t%-10u |\n", buffer);
	printf("-----------------------------------\n");
}

static void apml_get_dram_throttle(uint8_t soc_die_num)
{
	uint32_t buffer;
	oob_status_t ret;

	ret = read_dram_throttle(soc_die_num, &buffer);

	if (ret != OOB_SUCCESS) {
		printf("Failed to get the dram throttle, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("------------------------------------\n");
	printf("| DRAM THROTTLE (%%) | \t%-10u |\n", buffer);
	printf("------------------------------------\n");
}

static void apml_get_prochot_status(uint8_t soc_die_num)
{
	uint32_t buffer;
	oob_status_t ret;

	ret = read_prochot_status(soc_die_num, &buffer);

	if (ret != OOB_SUCCESS) {
		printf("Failed to get the prochot status, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-------------------------------------------\n");
	printf("| PROCHOT STATUS | \t%-17s |\n", buffer ?
	       "PROCHOT" : "NOT_PROCHOT");
	printf("-------------------------------------------\n");
}

static void apml_get_prochot_residency(uint8_t soc_die_num)
{
	float buffer;
	oob_status_t ret;

	ret = read_prochot_residency(soc_die_num, &buffer);

	if (ret != OOB_SUCCESS) {
		printf("Failed to get the prochot residency, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------\n");
	printf("| PROCHOT RESIDENCY (%%) | \t%-10.2f |\n", buffer);
	printf("--------------------------------------------\n");
}

static void apml_get_lclk_dpm_level_range(uint8_t soc_die_num,
					  uint8_t nbio_id)
{
	struct dpm_level dpm;
	oob_status_t ret;

	ret = read_lclk_dpm_level_range(soc_die_num, nbio_id, &dpm);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get the lclk dpm level range, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("--------------------------------------------\n");
	printf("| MIN DPM \t\t| \t%-10u |\n", dpm.min_dpm_level);
	printf("| MAX DPM \t\t| \t%-10u |\n", dpm.max_dpm_level);
	printf("--------------------------------------------\n");
}

static void apml_do_recovery(uint8_t soc_die_num, uint8_t client)
{
	oob_status_t ret;

	ret = apml_recover_dev(soc_die_num, client);
	if (ret != OOB_SUCCESS) {
		printf("Failed to do recovery, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-------------------------------------------------\n");
	printf("| Socket %u | Recovery of  %s client successful |\n",
	       soc_die_num, client ? "TSI" : "RMI");
	printf("-------------------------------------------------\n");
}

static void apml_get_power_consumed(uint8_t soc_die_num)
{
	uint32_t pow;
	oob_status_t ret;

	ret = read_socket_power(soc_die_num, &pow);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get power, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("---------------------------------------------\n");
	printf("| Power (Watts)\t\t |");
	printf(" %-17.3f|\n", (double)pow/1000);
	printf("---------------------------------------------\n");
}

static void apml_get_smt_status(uint8_t soc_die_num)
{
	uint32_t threads_per_core;
	oob_status_t ret;

	ret = esmi_get_threads_per_core(soc_die_num, & threads_per_core);
	if (ret) {
		printf(" Failed to SMT status  Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------------\n");
	printf("| SMT STATUS \t\t | %15s  |\n",
	       threads_per_core > 1 ? "ENABLED" : "DISBALED");
	printf("---------------------------------------------\n");
}

static void apml_get_threads_per_core_and_soc(uint8_t soc_die_num)
{
	uint32_t threads_per_core, threads_per_soc;
	oob_status_t ret;

	ret = esmi_get_threads_per_core(soc_die_num, &threads_per_core);
	if (ret) {
		printf("\n Failed to get threads per core Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	ret = esmi_get_threads_per_socket(soc_die_num, &threads_per_soc);
	if (ret) {
		printf("\n Failed to get threads per socket Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------\n");
	printf("| THREADS PER CORE \t | %17d  |\n", threads_per_core);
	printf("| THREADS PER SOCKET \t | %17d  |\n", threads_per_soc);
	printf("-----------------------------------------------\n");
}

static void apml_get_ccx_info(uint8_t soc_die_num)
{
	uint16_t max_cores_per_ccx, ccx_instances;
	oob_status_t ret;

	ret = read_ccx_info(soc_die_num, &max_cores_per_ccx, &ccx_instances);
	if (ret) {
		printf("\n Failed to get the ccx information Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("----------------------------------------------\n");
	printf("| No of cores per CCX \t | %17d |\n", max_cores_per_ccx);
	printf("| No of CCX instances \t | %17d |\n", ccx_instances);
	printf("----------------------------------------------\n");
}

static void apml_get_ucode_rev(uint8_t soc_die_num)
{
	uint32_t ucode;
	oob_status_t ret;

	ret = read_ucode_revision(soc_die_num, &ucode);
	if (ret) {
		printf("Failed to read ucode revision, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-------------------------------------------------------\n");
	printf("| ucode revision | 0x%-32x |\n", ucode);
	printf("-------------------------------------------------------\n");
}

static void apml_get_ras_df_validity_chk(uint8_t soc_die_num, uint8_t blk_id) {
	struct ras_df_err_chk err_chk;
	oob_status_t ret;

	ret = read_ras_df_err_validity_check(soc_die_num, blk_id, &err_chk);
	if (ret && ret != OOB_MAILBOX_ADD_ERR_DATA) {
		printf("Failed to read RAS DF validity check, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("----------------------------------------------------\n");
	if (ret == OOB_MAILBOX_ADD_ERR_DATA) {
		printf("| MB error:0x%x additional error data | 0x%x|\n",
		       ret, err_chk.add_err_data);
	} else {
		printf("| Err log length\t\t| %-17u|\n", err_chk.err_log_len);
		printf("| DF Block instances\t\t| %-17u|\n", err_chk.df_block_instances);
	}
	printf("----------------------------------------------------\n");
}

static void apml_get_ras_df_err_dump(uint8_t soc_die_num, union ras_df_err_dump df_err)
{
	uint32_t data = 0;
	oob_status_t ret;

	ret = read_ras_df_err_dump(soc_die_num, df_err, &data);
	if (ret) {
		printf("Failed to read RAS error dump for offset[%d] "
		       "Err[%d]:%s\n", df_err.input[0], ret,
		       esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------------"
	       "-------------------\n");
	printf("| Data from offset[%03d]\t\t| 0x%-32x|\n", df_err.input[0], data);
	printf("--------------------------------------------------"
	       "-------------------\n");
}

static void apml_reset_on_sync_flood(uint8_t soc_die_num)
{
	uint32_t ack_resp = 0;
	oob_status_t ret;

	ret = reset_on_sync_flood(soc_die_num, &ack_resp);
	if (ret) {
		printf("Failed to reset after sync flood, Err[%d]: "
		       "%s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("----------------------------------------------\n");
	printf("| %-42s |\n", ack_resp  == 1 ?
	       "ACK: SMU FW will proceed with reset"
	       : "NACK: SMU FW will not proceed with reset");
	printf("----------------------------------------------\n");
}

static void apml_override_delay_reset_on_sync_flood(uint8_t soc_die_num,
						    struct ras_override_delay in)
{
	bool ack_resp;
	oob_status_t ret;

	ret = override_delay_reset_on_sync_flood(soc_die_num, in, &ack_resp);
	if (ret) {
		printf("Failed to override delay value reset on sync flood,"
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("----------------------------------------------------\n");
	printf("| %-48s |\n", ack_resp  == 1 ?
	       "ACK: SMU FW will honor the override request"
	       : "NACK: SMU FW will not honor the override request");
	printf("----------------------------------------------------\n");
}

static void apml_get_post_code(uint8_t soc_die_num, char *offset)
{
	uint32_t post_code = 0, code_offset = 0, is_digit = 0;
	uint8_t index = 0;
	oob_status_t ret;

	is_digit  = isdigit(offset[0]);
	if (is_digit) {
		code_offset = atoi(offset);
		ret = get_post_code(soc_die_num, code_offset, &post_code);
		if (ret) {
			printf("Failed to get post code for a given offset,"
			       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
			return;
		}

		printf("---------------------------------------\n");
		printf("| Post code [%u]\t | 0x%-17x |\n", code_offset,
		       post_code);
		printf("---------------------------------------\n");
		return;
	}
	if ((strcmp(offset, "s") == 0) || (strcmp(offset, "summary") == 0)) {
		for (index = 0; index < 8; index++) {
			ret = get_post_code(soc_die_num, index, &post_code);
			if (ret) {
				printf("Failed to get post code for a given"
				       "offset[%u],Err[%d]: %s\n", index, ret,
				       esmi_get_err_msg(ret));
				return;
			}
			if (index == 0)
				printf("-----------------------------------"
				       "-----\n");
			printf("| Post code [%u]\t | 0x%-17x |\n", index,
			       post_code);
		}
		printf("----------------------------------------\n");
	}
	else {
		printf("Failed to get post code for a given offset,"
		       "Err[%d]: %s\n", OOB_INVALID_INPUT,
		       esmi_get_err_msg(OOB_INVALID_INPUT));
		return;
	}
}

static void apml_clear_ras_status_register(uint8_t soc_die_num, uint8_t value)
{
	uint8_t reg_value;
	oob_status_t ret;

	ret = clear_sbrmi_ras_status(soc_die_num, value);
	if (ret) {
		printf("Failed to clear RAS status register"
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("Required RAS status register bit cleared successfully\n");

	return;
}

static void apml_clear_sw_async_alert_status(uint8_t soc_die_num)
{
	oob_status_t ret;

	ret = clear_sw_async_alert_status(soc_die_num);
	if (ret) {
		printf("Failed to clear sw async alert status, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("Software async alert status cleared successfully\n");
}

static void apml_clear_shut_down_err(uint8_t soc_die_num)
{
	oob_status_t ret;

	ret = clear_shut_down_err(soc_die_num);
	if (ret) {
		printf("Failed to clear shut down error, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("CPU shut down status cleared successfully\n");
}

static void apml_clear_mp0_alert_status(uint8_t soc_die_num)
{
	oob_status_t ret;

	ret = clear_MP0_alert_status(soc_die_num);
	if (ret) {
		printf("Failed to clear mp0 alert status, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("MP0 alert status cleared successfully\n");
}

static void apml_set_sw_async_alert_mask(uint8_t soc_die_num, bool sw_async_alert_mask)
{
	oob_status_t ret;

	ret = set_sw_async_alert_mask(soc_die_num, sw_async_alert_mask);
	if (ret) {
		printf("Failed to set sw async alert mask, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("Alert_L signaling is %s successfully\n", sw_async_alert_mask ? "disabled" : "enabled");

}

static void apml_set_power_efficiency_mode_selection(uint8_t soc_die_num, struct pow_eff_mode d_in) {
	struct pow_eff_mode d_out = {0};
	oob_status_t ret;

	ret = set_power_efficiency_mode_selection(soc_die_num, d_in, &d_out);
	if (ret) {
		printf("Failed to set power efficiency mode policy, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------\n");
	printf("| Power Efficiency Mode  | %-10u |\n", d_out.mode);
	if (d_out.mode == 4 || d_out.mode == 5) {
		printf("| Utilization point \t | %-10u%%|\n", d_out.utilization_point);
		printf("| PPT Mode (mW)\t\t | %-10u |\n", d_out.ppt_limit);
	}
	printf("---------------------------------------\n");
}

static void apml_get_power_efficiency_mode_selection(uint8_t soc_die_num)
{
	struct pow_eff_mode d_out = {0};
	oob_status_t ret;

	ret = get_power_efficiency_mode_selection(soc_die_num, &d_out);
	if (ret) {
		printf("Failed to get power efficiency mode policy, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------\n");
	printf("| Power Efficiency Mode  | %-10u |\n", d_out.mode);
	if (d_out.mode == 4 || d_out.mode == 5) {
		printf("| Utilization point \t | %-10u%%|\n", d_out.utilization_point);
		printf("| PPT Mode (mW)\t\t | %-10u |\n", d_out.ppt_limit);
	}
	printf("---------------------------------------\n");
}

static void apml_get_bmc_ras_rt_err_validity_check(uint8_t soc_die_num,
						   struct ras_rt_err_req_type rt_err_category)
{
	struct ras_rt_valid_err_inst inst;
	oob_status_t ret;
	char *err_catg;

	ret = get_bmc_ras_run_time_err_validity_ck(soc_die_num, rt_err_category,
						   &inst);
	if (ret) {
		printf("Failed to get bmc ras runtime error validity check,"
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	switch (rt_err_category.err_type) {
	case 0:
		err_catg = "MCA";
		break;
	case 1:
		err_catg = "DRAM CECC";
		break;
	case 2:
		err_catg = "PCIe";
		break;
	default:
		err_catg = "RSVD";
		break;
	}

	printf("----------------------------------------------------"
	       "----------------\n");
	printf("| %-9s: Number of valid err Instance \t| %16u |\n",
	       err_catg, inst.number_of_inst);
	printf("| %-9s: Number of bytes\t\t\t| %16u |\n",
	       err_catg, inst.number_bytes);
	printf("----------------------------------------------------"
	       "----------------\n");
}

static void apml_get_ras_runtime_err_info(uint8_t soc_die_num,
					  struct run_time_err_d_in d_in)
{
	uint32_t d_out;
	uint16_t err_count;
	uint8_t ch_num;
	uint8_t sub_ch;
	uint8_t chip_sel_num;
	uint8_t rank_mult, p_type = 0;
	oob_status_t ret;

	ret = get_bmc_ras_run_time_error_info(soc_die_num, d_in, &d_out);
	if (ret) {
		printf("Failed to get bmc ras runtime error info,"
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	if (d_in.category == DRAM_ECC_ERR_CAT && d_in.offset == ZERO_OFFSET) {
		err_count = d_out;
		ret = get_proc_type(soc_die_num, &p_type);
		if (ret) {
			printf("Failed to fetch processor type,"
					"Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
			return;
		}
		if (p_type == FAM_1A_MOD_50) {
			uint8_t ch_mask = 0x01F;
			ch_num = (d_out >> CH_NUM_POS) & ch_mask;
			sub_ch = (d_out >> (SUB_CH_POS + BIT(0))) & BIT_MASK;
			chip_sel_num = (d_out >> (CHIP_SEL_NUM_POS + BIT(0))) & TRIBBLE_BITS;
			rank_mult = (d_out >> (RANK_MUL_NUM_POS + BIT(0))) & RANK_MUL_MASK;
		} else {
			ch_num = (d_out >> CH_NUM_POS) & NIBBLE_MASK;
			sub_ch = (d_out >> SUB_CH_POS) & BIT_MASK;
			chip_sel_num = (d_out >> CHIP_SEL_NUM_POS) & TRIBBLE_BITS;
			rank_mult = (d_out >> RANK_MUL_NUM_POS) & RANK_MUL_MASK;
		}
		printf("------------------------------------\n");
		printf("|Error Count  | %-16u   |\n", err_count);
		printf("|CHAN Number  | 0x%-16x |\n", ch_num);
		printf("|SUB Channel  | 0x%-16x |\n", sub_ch);
		printf("|Chip sel num | 0x%-16x |\n", chip_sel_num);
		printf("|Rank Mul num | 0x%-16x |\n", rank_mult);
		printf("------------------------------------\n");
		return;
	}
	printf("--------------------------------------\n");
	printf("| Data\t\t| 0x%-16x |\n", d_out);
	printf("--------------------------------------\n");
}

static void apml_set_ras_err_threshold(uint8_t soc_die_num,
				       struct run_time_threshold th)
{
	oob_status_t ret;

	ret = set_bmc_ras_err_threshold(soc_die_num, th);
	if (ret) {
		printf("Failed to set bmc ras error threshold "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("BMC RAS error threshold set successfully\n");
}

static void apml_set_ras_oob_config(uint8_t soc_die_num,
				    struct oob_config_d_in d_in)
{
	oob_status_t ret;

	ret = set_bmc_ras_oob_config(soc_die_num, d_in);
	if (ret) {
		printf("Failed to set ras oob configuration "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("BMC RAS oob configuration set successfully\n");
}

static void apml_get_ras_oob_config(uint8_t soc_die_num)
{
	oob_status_t ret;
	uint32_t d_out;

	ret = get_bmc_ras_oob_config(soc_die_num, &d_out);
	if (ret) {
		printf("Failed to get ras oob configuration "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------------------------"
	       "--------\n");
	printf("| MCA OOB Err Counter\t\t\t\t | %-8s |\n", (d_out & BIT_MASK)
	       ? "Enabled" : "Disabled");
	printf("| DRAM CECC OOB CECC Err Counter Mode\t\t | %-8u |\n",
	       (d_out >> DRAM_CECC_OOB_EC_MODE & TRIBBLE_BITS));
	printf("| DRAM CECC OOB Leak Rate\t\t\t | 0x%-6x |\n",
	       (d_out >> DRAM_CECC_LEAK_RATE & DRAM_CECC_LEAK_RATE_MASK));
	printf("| PCIe OOB Error Reporting Enable\t\t | %-8s |\n",
	       (d_out >> PCIE_ERR_REPORT_EN & BIT_MASK) ?
	       "Enabled" : "Disabled");
	printf("| MCA Thresholding Interrupt Enable\t\t | %-8s |\n",
	       (d_out >> MCA_TH_INTR & BIT_MASK) ? "Enabled" : "Disabled");
	printf("| DRAM CECC Thresholding Interrupt Enable\t | %-8s |\n",
	       (d_out >> CECC_TH_INTR & BIT_MASK) ? "Enabled" : "Disabled");
	printf("| PCIE Thresholding Interrupt Enable\t\t | %-8s |\n",
	       (d_out >> PCIE_TH_INTR & BIT_MASK) ? "Enabled" : "Disabled");
	printf("| MCA Max Interrupt Rate\t\t\t | 0x%-6x |\n",
	       (d_out >> MCA_MAX_INTR_RATE & NIBBLE_MASK));
	printf("| DRAM CECC Max Interrupt Rate\t\t\t | 0x%-6x |\n",
	       (d_out >> DRAM_CECC_MAX_INTR_RATE & NIBBLE_MASK));
	printf("| PCIe Max Interrupt Rate  \t\t\t | 0x%-6x |\n",
	       (d_out >> PCIE_MAX_INTR_RATE & NIBBLE_MASK));
	printf("| MCA OOB Error Reporting Enable\t\t | %-8s |\n",
	       (d_out >> MCA_ERR_REPORT_EN & BIT_MASK)
	       ? "Enabled" : "Disabled");
	printf("------------------------------------------------------"
	       "-------\n");
}

static oob_status_t apml_get_ppin_fuse(uint8_t soc_die_num)
{
	uint64_t data = 0;
	oob_status_t ret;

	ret = read_ppin_fuse(soc_die_num, &data);
	if (ret) {
		printf("Failed to get the PPIN fuse data, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}

	printf("------------------------------------------------------------"
	       "---------------------\n");
	printf("| PPIN Fuse | 0x%-64" PRIx64 " |\n", data);
	printf("------------------------------------------------------------"
	       "---------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_get_cclk_freqlimit(uint8_t soc_die_num)
{
	uint32_t buffer = 0;
	oob_status_t ret;

	ret = read_cclk_freq_limit(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get cclk_freqlimit, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("-----------------------------------------------------\n");
	printf("| cclk_freqlimit (MHz)\t\t | %-16u |\n", buffer);
	printf("-----------------------------------------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_get_sockc0_residency(uint8_t soc_die_num)
{
	uint32_t buffer = 0;
	oob_status_t ret;

	ret = read_socket_c0_residency(soc_die_num, &buffer);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get c0_residency, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("----------------------------------------------\n");
	printf("| c0_residency (%%)\t |  %-16u |\n", buffer);
	printf("----------------------------------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_get_rtc(uint8_t soc_die_num)
{
	uint64_t rtc_val = 0;
	oob_status_t ret = OOB_SUCCESS;

	ret = read_rtc(soc_die_num, &rtc_val);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get rtc timer, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("------------------------------------------------------------"
	       "-------\n");
	printf("| RTC timer (YYYYMMDDhhmmss)  |  %-32" PRIx64 " |\n", rtc_val);
	printf("------------------------------------------------------------"
	       "-------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_get_dimm_serial_num(uint8_t soc_die_num, uint8_t dimm_addr)
{
	oob_status_t ret = OOB_SUCCESS;
	uint32_t serial_num = 0x0;

	ret = get_dimm_serial_num(soc_die_num, dimm_addr, &serial_num);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get dimm addr, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("-----------------------------------\n");
	printf("| DIMM addr | DIMM serial number  |\n");
	printf("|---------------------------------|\n");
	printf("| 0x%x      |  0x%-16x |\n", dimm_addr, serial_num);
	printf("-----------------------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_get_spd_sb_data(uint8_t soc_die_num, struct dimm_spd_d_in spd_d_in)
{
	oob_status_t ret = OOB_SUCCESS;
	uint32_t spd_data = 0x0;

	ret = read_dimm_spd_register(soc_die_num, spd_d_in, &spd_data);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get spd data, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("------------------------------\n");
	printf("| DIMM spd data | 0x%-8x |\n", spd_data);
	printf("------------------------------\n");

	return OOB_SUCCESS;
}

static oob_status_t apml_get_dimm_sb_data(uint8_t soc_die_num, struct dimm_sb_reg_d_in d_in)
{
	oob_status_t ret = OOB_SUCCESS;
	uint32_t dimm_data = 0x0;

	ret = get_dimm_sb_register(soc_die_num, d_in, &dimm_data);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get DIMM register data, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	printf("------------------------------\n");
	printf("| DIMM reg data | 0x%-8x |\n", dimm_data);
	printf("------------------------------\n");

	return OOB_SUCCESS;
}

static void apml_get_smu_fw_version(uint8_t soc_die_num)
{
	uint32_t fw_ver = 0;
	oob_status_t ret = OOB_SUCCESS;
	ret = read_smu_fw_ver(soc_die_num, &fw_ver);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get smu fw version, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------------------\n");
	printf("| SMU FW VERSION\t | 0x%-16x |\n", fw_ver);
	printf("-----------------------------------------------\n");
}

static void apml_set_xgmi_pstate_range(uint8_t soc_die_num,
					       uint8_t min_xgmi_pstate,
					       uint8_t max_xgmi_pstate)
{
	oob_status_t ret;
	ret = set_xgmi_pstate_range(soc_die_num, min_xgmi_pstate, max_xgmi_pstate);
	if (ret) {
		printf("Failed to set XGMI p-state range "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("XGMI pstate range set successfully\n");
}

static void apml_set_cpu_rail_iso_freq_policy(uint8_t soc_die_num,
						      uint8_t policy)
{
	oob_status_t ret;
	ret = set_cpu_rail_iso_freq_policy(soc_die_num, policy);
	if (ret) {
		printf("Failed to set cpu rail iso frequency policy "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("CPU rail iso frequency policy set successfully\n");
}

static void apml_set_dfc_enable(uint8_t soc_die_num, uint8_t state)
{
	oob_status_t ret;

	ret = set_dfc_enable(soc_die_num, state);
	if (ret) {
		printf("Failed to set DFC enable "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("DFC State set successfully\n");
}

static void apml_get_cpu_rail_iso_freq_policy(uint8_t soc_die_num)
{
	uint8_t policy = 0;
	oob_status_t ret;

	ret = get_cpu_rail_iso_freq_policy(soc_die_num, &policy);
	if (ret) {
		printf("Failed to get cpu rail iso frequency policy "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------"
		"------------------\n");
	printf("| CURRENT POLICY\t | %-36s |\n", policy ?
			"Each rail has different freq limit":
			"Both rails have same freq limit");
	printf("-----------------------------------------------"
		"------------------\n");
}

static void apml_get_dfc_enable(uint8_t soc_die_num)
{
	uint8_t state = 0;
	oob_status_t ret;

	ret = get_dfc_enable(soc_die_num, &state);
	if (ret) {
		printf("Failed to get dfc enable "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-------------------------------------\n");
	printf("| DFC State\t | %-16s |\n", state ? "ENABLED" : "DISABLED");
	printf("-------------------------------------\n");
}

static void apml_get_avg_dram_throttle(uint8_t soc_die_num)
{
        uint32_t dram_throttle = 0;
        oob_status_t ret;

        ret = get_avg_dram_throttle(soc_die_num, &dram_throttle);
        if (ret) {
                printf("Failed to get avg dram throttle for all channels "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("------------------------------------------\n");
        printf("| Avg DRAM Throttle | %-3u %-14s |\n", dram_throttle, "%");
        printf("------------------------------------------\n");
}

static void apml_get_ch_dram_throttle(uint8_t soc_die_num, uint8_t dimm_addr)
{
        uint32_t dram_throttle = 0;
        oob_status_t ret;

        ret = get_ch_dram_throttle(soc_die_num, dimm_addr, &dram_throttle);
        if (ret) {
                printf("Failed to get dram throttle for the specified channel "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("-------------------------------------------------\n");
	printf("| Dimm Addr\t\t | DRAM Throttle\t|\n");
        printf("-------------------------------------------------\n");
        printf("| 0x%-16x\t | %-3u %-14s   |\n", dimm_addr, dram_throttle, "%");
        printf("-------------------------------------------------\n");
}

static void apml_get_avg_dram_thr_with_status(uint8_t soc_die_num)
{
	struct dram_thr_with_status thr_info = {0};
	oob_status_t ret;

	ret = get_avg_dram_thr_with_status(soc_die_num, &thr_info);
	if (ret) {
		printf("Failed to get avg DRAM throttle with status Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("----------------------------------------------------\n");
	printf("| Avg DRAM Throttle\t\t| %-3u %-12s |\n", thr_info.thr_pct, "%");
	printf("| TSOD thermal throttle Status\t| %-16s |\n", thr_info.tsod_th_thr_stat ? "Active" : "Not Active");
	printf("| TSOD Enable Status\t\t| %-16s |\n", thr_info.tsod_en_stat ? "Enabled" : "Disabled");
	printf("| ODTS thermal throttle Status\t| %-16s |\n", thr_info.odts_th_thr_stat ? "Active" : "Not Active");
	printf("| ODTS Enable Status\t\t| %-16s |\n", thr_info.odts_en_stat ? "Enabled" : "Disabled");
	printf("----------------------------------------------------\n");
}

static void apml_get_dram_thr_with_status(uint8_t soc_die_num, uint8_t dimm_addr)
{
	struct dram_thr_with_status thr_info = {0};
	oob_status_t ret;

	ret = get_ch_dram_thr_with_status(soc_die_num, dimm_addr, &thr_info);
	if (ret) {
		printf("Failed to get channel DRAM throttle with status Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------------------\n");
	printf("| Dimm Addr\t\t\t | 0x%-14x |\n", dimm_addr);
	printf("| DRAM Throttle\t\t\t | %-3u %-12s |\n", thr_info.thr_pct, "%");
	printf("| TSOD thermal throttle Status\t | %-16s |\n", thr_info.tsod_th_thr_stat ? "Active" : "Not Active");
	printf("| TSOD Enable Status\t\t | %-16s |\n", thr_info.tsod_en_stat ? "Enabled" : "Disabled");
	printf("| ODTS thermal throttle Status\t | %-16s |\n", thr_info.odts_th_thr_stat ? "Active" : "Not Active");
	printf("| ODTS Enable Status\t\t | %-16s |\n", thr_info.odts_en_stat ? "Enabled" : "Disabled");
	printf("-----------------------------------------------------\n");
}

static void apml_get_xgmi_pstate_range(uint8_t soc_die_num)
{
        uint8_t min_xgmi_pstate = 0, max_xgmi_pstate = 0;
        oob_status_t ret;

        ret = get_xgmi_pstate_range(soc_die_num, &min_xgmi_pstate,
                                    &max_xgmi_pstate);
        if (ret) {
                printf("Failed to xgmi pstate range "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("---------------------------------------------\n");
        printf("| Min XGMI P-State\t | %-16d |\n", min_xgmi_pstate);
        printf("| Max XGMI P-State\t | %-16d |\n", max_xgmi_pstate);
        printf("---------------------------------------------\n");
}

static void apml_get_xgmi_link_width_range(uint8_t soc_die_num)
{
        uint8_t min_link_width = 0, max_link_width = 0;
        oob_status_t ret;

        ret = get_xgmi_link_width_range(soc_die_num, &min_link_width,
                                        &max_link_width);
        if (ret) {
                printf("Failed to xgmi link width range "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("---------------------------------------------\n");
        printf("| Min link width\t | %-16d |\n", min_link_width);
        printf("| Max link width\t | %-16d |\n", max_link_width);
        printf("---------------------------------------------\n");
}

static void apml_get_apb_state(uint8_t soc_die_num)
{
        uint8_t apb_state = 0, df_pstate = 0;
        oob_status_t ret;

        ret = get_apb_state(soc_die_num, &apb_state, &df_pstate);
        if (ret) {
                printf("Failed to apb state "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("-------------------------------------\n");
        printf("| APB State\t | %-16s |\n", apb_state ? "APB Disabled" : "APB Enabled");
        if (apb_state)
                printf("| DF P-State\t | %-16d |\n", df_pstate);
	else
		printf("| DF P-State\t | %-16s |\n", "NA");
        printf("-------------------------------------\n");
}

static void apml_get_df_pstate_range(uint8_t soc_die_num)
{
        uint8_t min_pstate = 0, max_pstate = 0;
        oob_status_t ret;

        ret = get_df_pstate_range(soc_die_num, &max_pstate, &min_pstate);
        if (ret) {
                printf("Failed to DF P-state range "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("-------------------------------------\n");
        printf("| Min p-state\t | %-16d |\n", min_pstate);
        printf("| Max p-state\t | %-16d |\n", max_pstate);
        printf("-------------------------------------\n");
}

static void apml_set_dimm_register_data(uint8_t soc_die_num, struct dimm_sb_reg_write d_in)
{
        oob_status_t ret;

        ret = set_dimm_sb_register_data(soc_die_num, d_in);
        if (ret) {
                printf("Failed to set DIMM register data"
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("DIMM register data set successfully\n");
}

static void apml_set_dimm_spd_register(uint8_t soc_die_num, struct dimm_spd_write d_in)
{
        oob_status_t ret;

        ret = write_dimm_spd_register(soc_die_num, d_in);
        if (ret) {
                printf("Failed to set dimm spd register "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("SPD SB set successfully\n");
}

static void apml_set_pc6_enable(uint8_t soc_die_num, uint8_t pc6_state)
{
        uint8_t updated_pc6_state = 0;
        oob_status_t ret;

        ret = set_pc6_enable(soc_die_num, pc6_state, &updated_pc6_state);
        if (ret) {
                printf("Failed to set PC6 enable "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        if (updated_pc6_state == pc6_state)
                printf("PC6 control set successfully\n");
        else
                printf("PC6 control not set\n");
}
static void apml_get_pc6_enable(uint8_t soc_die_num)
{
        uint8_t pc6_state = 0;
        oob_status_t ret;

        ret = get_pc6_enable(soc_die_num, &pc6_state);
        if (ret) {
                printf("Failed to get PC6 enable "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("-------------------------------------\n");
        printf("| PC6 Control\t | %-16s |\n", pc6_state ? "Enabled" : "Disabled");
        printf("-------------------------------------\n");
}

static void apml_set_cc6_enable(uint8_t soc_die_num, uint8_t cc6_state)
{
        uint8_t updated_cc6_state = 0;
        oob_status_t ret;

        ret = set_cc6_enable(soc_die_num, cc6_state, &updated_cc6_state);
        if (ret) {
                printf("Failed to set CC6 control "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        if (updated_cc6_state == cc6_state)
                printf("CC6 control set successfully\n");
        else
                printf("CC6 control not set\n");
}

static void apml_get_cc6_enable(uint8_t soc_die_num)
{
        uint8_t cc6_state = 0;
        oob_status_t ret;

        ret = get_cc6_enable(soc_die_num, &cc6_state);
        if (ret) {
                printf("Failed to get CC6 enable "
                       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
                return;
        }

        printf("-------------------------------------\n");
        printf("| CC6 Control\t | %-16s |\n", cc6_state ? "Enabled" : "Disabled");
        printf("-------------------------------------\n");
}

static void apml_get_ccd_power_consumption(uint8_t soc_die_num, uint32_t logical_core_id)
{
	uint32_t power = 0;
	oob_status_t ret;

	ret = get_ccd_power_consumption(soc_die_num, logical_core_id, &power);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get ccd power, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}
	printf("---------------------------------------------");
	printf("\n| Power (Watts)\t\t |");
	printf(" %-17.3f|", (double)power/1000);
	printf("\n---------------------------------------------\n");
}

static void apml_get_tdelta(uint8_t soc_die_num)
{
	uint32_t resp = 0;
	char *err_response = "Thermal Solution is out of expected range";
	char *success_response = "Thermal Solution behavior is normal";
        oob_status_t ret;

        ret = get_tdelta(soc_die_num, &resp);
        if (ret != OOB_SUCCESS) {
                printf("Failed to get tdelta, Err[%d]: %s\n",
                        ret, esmi_get_err_msg(ret));
                return;
        }
        printf("----------------------------------------------------------"
	       "-----------------------");
        printf("\n| TDelta \t\t\t |");
        printf(" %-45s |\n", resp ? err_response : success_response);
        printf("----------------------------------------------------------"
	       "-----------------------");
}

static void apml_get_svi3_vr_controller_temp_by_rail(uint8_t soc_die_num, struct svi3_vr_cont_data_in d_in)
{
	struct svi3_vr_cont_data_out d_out = {0};
	uint32_t resp = 0;
	oob_status_t ret;

	ret = get_svi3_vr_controller_temp_by_rail(soc_die_num, d_in, &d_out);
	if (ret != OOB_SUCCESS) {
		printf("Failed to get SVI3 vr controller temperature by rail, Err[%d]: %s\n",
			ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------------------");
	printf("\n| SVI3 Rail Temperature \t | %-17d °C|", d_out.svi3_temp);
	printf("\n| SVI3 Rail Index \t\t | %-17d   |\n", d_out.svi3_rail_index);

	printf("--------------------------------------------------------");
}

static void apml_get_supporting_error_types(uint8_t soc_die_num)
{
	struct oob_err_inj_types err_types = {0};
	uint8_t index = 0, offset = 0;
	oob_status_t ret;

	ret = get_supporting_error_types(soc_die_num, &err_types);
	if (ret) {
		printf("Failed to get supporting error types, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("--------------------------------------------------------");
	printf("\n| Generic Error Types \t\t | 0x%-16x |", err_types.err_types_generic);
	printf("\n| Vendor Error Size \t\t | 0x%-16x |", err_types.err_size);
	printf("\n| Vendor Flag \t\t\t | %-18s |", err_types.vendor_flag ? "Enabled (1)" : "Disabled (0)");
	for (index = 8; index <=err_types.err_size; index=index+4) {
		printf("\n| Vendor Error Types[%d] \t | 0x%-16x |", offset, err_types.err_types_vendor[offset]);
		offset++;
	}
	printf("\n--------------------------------------------------------");
}

static void apml_get_bmc_ras_run_time_error_info_all_offsets(uint8_t soc_die_num, uint8_t category)
{
	struct ras_rt_err_req_type err_category = {0};
	struct ras_rt_valid_err_inst inst = {0};
	struct bmc_ras_runtime_din din = {0};
	struct ras_runtime_response resp;
	uint32_t mca_offset_size = 0;
	oob_status_t ret;

	err_category.err_type = category;
	err_category.req_type = 0;
	ret = get_bmc_ras_run_time_err_validity_ck(soc_die_num, err_category, &inst);
	if (ret) {
		printf("Failed to get ras run time error info, Err[%d]: %s\n",
			   ret, esmi_get_err_msg(ret));
		return;
	}

	din.err_category = err_category.err_type;
	din.number_of_instance = inst.number_of_inst;
	din.number_of_offsets = inst.number_bytes / 4;

	// Allocate buffer data
	if (din.number_of_instance) {
		resp.buffer_data = (uint32_t **)malloc(din.number_of_instance * sizeof(uint32_t *));
	} else {
		resp.buffer_data = NULL;
		printf("Failed to allocate buffer data as number of instances returned is zero\n");
		return;
	}
	if (!resp.buffer_data) {
		printf("Failed to allocate buffer data, Err[%d]: %s\n",
			   OOB_ARG_PTR_NULL, esmi_get_err_msg(OOB_ARG_PTR_NULL));
		return;
	}

	for (uint32_t i = 0; i < din.number_of_instance; i++) {
		resp.buffer_data[i] = (uint32_t *)malloc(din.number_of_offsets * sizeof(uint32_t));
		if (!resp.buffer_data[i]) {
			printf("Failed to allocate buffer data, Err[%d]: %s\n",
				   OOB_ARG_PTR_NULL, esmi_get_err_msg(OOB_ARG_PTR_NULL));
			// Free previously allocated buffers
			for (uint32_t j = 0; j < i; j++)
				free(resp.buffer_data[j]);
			free(resp.buffer_data);
			return;
		}
	}

	switch (category) {
	case MCA_ERR_CATEGORY:
		ret = get_bmc_ras_mca_run_time_error_info(soc_die_num, din, &resp);
		break;
	case DRAM_ECC_ERR_CATEGORY:
		ret = get_bmc_ras_dram_ecc_run_time_error_info(soc_die_num, din, &resp);
		break;
	case PCIE_ERR_CATEGORY:
		ret = get_bmc_ras_pcie_run_time_error_info(soc_die_num, din, &resp);
		break;
	default:
		ret = OOB_INVALID_INPUT;
		break;
	}
	if (ret) {
		printf("Failed to read bmc ras run time error info, Err[%d]: %s\n",
			   ret, esmi_get_err_msg(ret));
		for (uint32_t i = 0; i < din.number_of_instance; i++)
			free(resp.buffer_data[i]);
		free(resp.buffer_data);
		return;
	}

	// Print header row
	printf("------------");
	for (uint32_t i = 0; i < resp.num_of_instances; i++)
		printf("--------------");
	printf("\n");

	printf("%-12s |", "| Offset");
	for (uint32_t i = 0; i < resp.num_of_instances; i++)
		printf(" Instance %u |", i);
	printf("\n");

	// Print a separator line
	printf("------------");
	for (uint32_t i = 0; i < resp.num_of_instances; i++)
		printf("--------------");
	printf("\n");

	// Print data rows
	for (uint32_t j = 0; j < resp.buffer_size_per_instance; j++) {
		printf("| %-10u |", j * BIT(2));  // Offset column
		for (uint32_t i = 0; i < resp.num_of_instances; i++)
			printf(" 0x%08X |", resp.buffer_data[i][j]);
		printf("\n");
	}

	printf("------------");
	for (uint32_t i = 0; i < resp.num_of_instances; i++)
		printf("--------------");
	printf("\n");

	// Release the memory
	for (uint32_t i = 0; i < resp.num_of_instances; i++)
		free(resp.buffer_data[i]);
	free(resp.buffer_data);
}

static void apml_get_apml_floor_core_limit(uint8_t soc_die_num, uint16_t core_id)
{
	uint16_t floor_core_limit = 0;
	oob_status_t ret;

	ret = get_floor_core_limit(soc_die_num, core_id, &floor_core_limit);
	if (ret) {
		printf("Failed to get APML floor core limit, Err[%d]: %s\n",
			   ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-----------------------------------------\n");
	printf("| Floor Core Limit\t | %-8u MHz |\n", floor_core_limit);
	printf("-----------------------------------------\n");
}

static void apml_set_apml_floor_core_limit(uint8_t soc_die_num,
					   struct core_floor_limit_din d_in)
{
	oob_status_t ret;

	ret = set_core_floor_limit(soc_die_num, d_in);
	if (ret) {
		printf("Failed to set APML floor core limit, Err[%d]: %s\n",
			   ret, esmi_get_err_msg(ret));
		return;
	}
	printf("APML floor core limit set successfully\n");
}

static void apml_set_apml_floor_limit_for_all_cores(uint8_t soc_die_num,
						    uint32_t floor_limit)
{
	oob_status_t ret;

	ret = set_floor_limit_for_all_cores(soc_die_num,
					    floor_limit);
	if (ret) {
		printf("Failed to set APML floor limit for all cores, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("APML floor limit set successfully in all cores\n");
}

static void apml_get_sbrmi_ready_status(uint8_t soc_die_num)
{
	bool status = false;
	oob_status_t ret;

	ret = read_sbrmi_ready_status(soc_die_num, &status);
	if (ret) {
		printf("Failed to read sbrmi ready status, Err[%d]: %s\n",
			    ret, esmi_get_err_msg(ret));
		return;
	}

	printf("-----------------------------------------\n");
	printf("| SBRMI ready status | %16u |\n", status);
	printf("-----------------------------------------\n");
}

static void apml_get_effective_floor_freq_per_core(uint8_t soc_die_num,
						   uint16_t core_id)
{
	uint16_t eff_core_limit = 0;
	oob_status_t ret;

	ret = get_effective_floor_freq_per_core(soc_die_num,
						core_id,
						&eff_core_limit);
	if (ret) {
		printf("Failed to get APML floor core limit, "
		       "Err[%d]: %s\n", ret, esmi_get_err_msg(ret));
		return;
	}
	printf("------------------------------------------------\n");
	printf("| Effective Floor Core Limit\t | %-8u MHz |\n",
		eff_core_limit);
	printf("------------------------------------------------\n");
}

static void apml_get_enabled_hsmp_commands(uint8_t soc_die_num, struct get_hsmp_cmds_din d_in)
{
	struct get_hsmp_cmds_dout d_out;
	oob_status_t ret;

	ret = get_enabled_hsmp_commands(soc_die_num, d_in, &d_out);
	if (ret) {
		printf("Failed to get enabled HSMP commands, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("-------------------------------------------------");
	printf("\n| HSMP Commands Offset Id\t| %-6d\t|", d_out.offset);
	printf("\n| HSMP Commands BitMask\t\t| 0x%06X\t|", d_out.bitmask);
	printf("\n| HSMP Commands Mask Type\t| %s\t|\n", (d_in.rmask == 1) ? "Read Mask" : "Write Mask");
	printf("-------------------------------------------------\n");
}

static void apml_set_enabled_hsmp_commands(uint8_t soc_die_num, struct set_hsmp_cmds_din d_in)
{
	oob_status_t ret;

	ret = set_enabled_hsmp_commands(soc_die_num, d_in);
	if (ret) {
		printf("Failed to set enabled HSMP commands, Err[%d]: %s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}
	printf("HSMP commands enabled successfully\n");
}

static void apml_get_ras_action_status(uint8_t soc_die_num,
				       struct get_ras_action_data_in data_in)
{
	struct ras_action_status status = {0};
	oob_status_t ret;

	ret = get_bmc_ras_action_status(soc_die_num, data_in, &status);
	if (ret) {
		printf("Failed to get ras action status, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("---------------------------------------------\n");
	printf("| Repair result \t | %-16u |\n", status.repair_result);
	printf("| Repair entry num \t | %-16u |\n",
	       status.repair_entry_num);
	printf("| RAS action ID \t | %-16u |\n", status.ras_action_id);
	printf("---------------------------------------------\n");
}

static void apml_set_ras_action(uint8_t soc_die_num,
				struct set_ras_action_data_in data_in)
{
	uint32_t resp = 0;
	oob_status_t ret;

	ret = set_bmc_ras_action_status(soc_die_num, data_in, &resp);
	if (ret) {
		printf("Failed to set ras action, Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return;
	}

	printf("---------------------------------------------\n");
	printf("| RAS action response\t | %-16u |\n", resp);
	printf("---------------------------------------------\n");
}


static void show_usage(char *exe_name)
{
	printf("Usage: %s [soc_die_num] [Option<s> / [--help] "
		"[module-name]\n", exe_name);
	printf("Where:  soc_die_num : "
	       "[7:0], [3:0]: Socket Number, [7:4]: Die Number\n");
	printf("Description:\n");
	printf("%s -v \t\t\t\t\t- Displays tool version\n", exe_name);
	printf("%s [SOC_DIE_NUM] --showdependency \t- Displays module dependency\n",
	       exe_name);
	printf("%s --help <MODULE>\t\t\t- Displays help on the options for "
		"the specified module\n", exe_name);
	printf("%s <option/s>\t\t\t\t- Runs the specified option/s."
	       "\nUsage: %s [soc_die_num]"
	       " [Option] params\n\n", exe_name, exe_name);
	printf("\tMODULES:\n");
	printf("\t1. mailbox\n");
	printf("\t2. sbrmi\n");
	printf("\t3. sbtsi\n");
	printf("\t4. reg-access\n");
	printf("\t5. cpuid\n");
	printf("\t6. recovery\n");
}

static void fam_19_common_mailbox_commands(void)
{
	printf("  --shownbioerrorloggingregister\t  "
	       "[QUADRANT(HEX)][OFFSET(HEX)]\t\t Show nbio error "
	       "logging register\n");
}

static void fam_19_mod_00_specific_mailbox_commands(void)
{
	printf("  --showvddiomempower\t\t\t  \t\t\t\t\t "
	       "Show vddiomem power\n");
}

static void fam_1A_mod_50_mailbox_commands()
{
        printf("  --getxgmilinkwidthrange\t\t\t  \t\t\t\t "
               "Show XGMI link width range\n"
               "  --getapbstate\t\t\t  \t\t\t\t\t\t "
               "Show apb state\n"
               "  --getdfpstaterange\t\t\t  \t\t\t\t\t "
               "Show df p-state range\n"
               "  --getxgmipstaterange\t\t\t  \t\t\t\t\t "
               "Show xgmi p-state range\n"
               "  --setdimmregdata\t\t\t  [DIMM_ADDR(HEX)][LID(HEX)]"
               "\n\t\t\t\t\t  [REG_OFFSET(HEX)][REG_SPACE]"
               "\n\t\t\t\t\t  [DATA(HEX)] \t\t\t\t"
               " Set DIMM register data.Valid LID codes are SPDHub(0xA),\n"
	       "\t\t\t\t\t\t\t\t\t\t PMIC0(0x9), TS0(0x2), TS1(0x6)\n"
               "  --getpc6control\t\t\t  \t\t\t\t\t "
               "Show PC6 enable state \n"
               "  --setpc6control\t\t\t  [STATE(0-1)] \t\t\t\t"
               " Set PC6 enable state\n"
               "  --getcc6control\t\t\t  \t\t\t\t\t "
               "Show CC6 enable state \n"
               "  --setcc6control\t\t\t  [STATE(0-1)] \t\t\t\t"
               " Set CC6 enable state\n"
	       "  --getccdpowconsumption\t\t  [COREID] \t\t\t\t"
	       " Show CCD power consumption in Watts\n"
	       "  --gettempdelta\t\t\t  \t\t\t\t\t "
	       "Show thermal solution health\n"
	       "  --pciehaltlinktraining\t\t  [LINK_ID(P0-P5, G0-G3)]"
	       "\n\t\t\t\t\t  [MASK_PCIE_PORT(per bit, 1=Mask)]"
	       "\n\t\t\t\t\t  [EOM]"
	       "\n\t\t\t\t\t\t\t\t\t\t PCIE halt link training for OOB device authentication\n"
	       "  --getsvi3vrtempbyrail\t\t\t  [TEMP_DATA_BIT(0-1)][RAIL_INDEX(0-4)]"
	       "\n\t\t\t\t\t\t\t\t\t\t Get SVI3 VR controller temperature by rail\n"
	       "  --getsupportingerrortypes\t\t\t  \t\t\t\t "
	       "Get supporting error types \n"
	       "  --getapmlcorefloorlimit\t\t  [CORE_ID] \t\t\t\t "
	       "Get apml core floor limit in MHz\n"
	       "  --getapmleffectivecorefloorlimit\t  [CORE_ID] \t\t\t\t "
	       "Get apml effective core floor limit in MHz\n"
	       "  --setapmlcorefloorlimit\t\t  [CORE_ID][FLOOR_LIMIT] \t\t "
	       "Set apml core floor limit in MHz\n"
	       "  --setapmlallcoresfloorlimit\t\t  [FLOOR_LIMIT] \t\t\t "
	       "Set apml floor limit for all cores in MHz\n"
	       "  --setpwrefficiencyprofile\t\t  [MODE(0 - 5)]"
	       "[UTILIZATIONPOINT(%%)][PPTLimit(mW)\n\t\t\t\t\t\t\t\t\t\t "
	       "Set power efficiency profile policy mode, Utilization point(%%) "
	       "and ppt\n\t\t\t\t\t\t\t\t\t\t limit(mW). Note : Utlization "
	       "point and ppt modes are applicable only for \n\t\t\t\t"
	       "\t\t\t\t\t\t modes 4 and 5.\n"
	       "  --getpwrefficiencyprofile\t\t\t  \t\t\t\t "
	       "Get power efficiency profile policy mode, Utilization point(%%) "
	       "and ppt\n\t\t\t\t\t\t\t\t\t\t limit(mW). Note : Utlization "
	       "point and ppt modes are applicable only for \n\t\t\t\t\t\t"
	       "\t\t\t\t modes 4 and 5.\n"
	       "  --showsocdimmpowerlimit\t\t\t\t\t\t\t "
	       "Get SOC+DIMM combined power limit in Watts\n"
	       "  --setsocdimmpowerlimit\t\t  [POWER]\t\t\t\t "
	       "Set SOC+DIMM combined power limit in mWatts (0=disable)\n"
	       "  --getenabledHSMPcommands\t\t  [RMASK(0-1)][OFFSET(0-15)]"
	       "\n\t\t\t\t\t\t\t\t\t\t Get enabled HSMP commands\n"
	       "  --setenabledHSMPcommands\t\t  [RMASK(0-1)][OFFSET(0-15)][BITMASK(HEX)]"
	       "\n\t\t\t\t\t\t\t\t\t\t Set enabled HSMP commands\n"
	       "  --setbmcrasaction\t\t\t  [PAYLOAD(HEX)]"
	       "[OFFSET]\n\t\t\t\t\t  [REPAIRENTRYNUMBER]"
	       "[RASACTIONID][EOM]\t Set BMC RAS action\n"
	       "  --getbmcrasactionstatus\t\t"
	       "  [REPAIRENTRYNUMBER][RASACTIONID]\t"
	       " Get BMC RAS action status\n"
		);
}

static void fam_1A_mod_50_common_mailbox_commands(void)
{
        printf("  --showppinfuse\t\t\t\t\t\t\t\t Show 64bit PPIN"
               " fuse data\n"
               "  --getpostcode\t\t\t\t  [POST_CODE_OFFSET(0 - 7 or s"
               "/summary)] Get post code for the given offset or"
               " recent 8 offsets\n"
               "  --showPCIeconfigspacedata\t\t  [SEGMENT][OFFSET]\n"
               "\t\t\t\t\t  [BUS(HEX)][DEVICE(HEX)][FUNC]\t\t Show "
               "32 bit data from extended PCI config space\n"
               "  --setpcieconfig\t\t\t  [SEGMENT][OFFSET][BUS(HEX)]\n"
               "\t\t\t\t\t  [DEVICE(HEX)][FUNC(HEX)][WR_DATA(HEX)] "
               "Set 32 bit data to extended PCI config space\n"
               "  --showvalidmcabanks\t\t\t\t\t\t\t\t "
               "Show number of MCA banks & bytes/bank with valid\n"
               "\t\t\t\t\t\t\t\t\t\t status after a fatal error\n"
               "  --showrasmcamsr\t\t\t  [MCA_BANK_INDEX][OFFSET]"
               "\t\t Show 32 bit data from specified MCA bank and "
               "offset\n"
               "  --showfchresetreason\t\t\t  [FCHID(0 or 1)]\t\t"
               "\t Show previous reset reason from FCH register\n"
               "  --showdimmtemprangeandrefreshrate\t  [DIMM_ADDR]"
               "\t\t\t\t Show per dimm temp range and refresh rate\n"
               "  --showdimmpower\t\t\t  [DIMM_ADDR]\t\t\t\t "
               "Show per dimm power consumption\n"
               "  --showhipowerdimm\t\t\t\t\t\t\t\t "
               "Show highest power consumption and dimm address.\n"
               "  --showalldimmpower\t\t\t\t\t\t\t\t "
               "Show all dimm power consumption.\n"
               "  --showdimmthermalsensor\t\t  [DIMM_ADDR]\t\t\t"
               "\t Show per dimm thermal sensor\n"
               "  --showhottestdimmthermalsensor\t\t  \t\t\t"
               "\t Show hottest dimm thermal sensor\n"
               "  --showsktfreqlimit\t\t\t\t\t\t\t\t "
               "Show per socket current active freq limit\n"
               "  --showcclklimit\t\t\t  [THREAD]\t\t\t\t "
               "Show core clock limit\n"
               "  --showsvitelemetryallrails\t\t\t\t\t\t\t "
               "Show svi based pwr telemetry for all rails\n"
               "  --showsktfreqrange\t\t\t\t\t\t\t\t "
               "Show per socket fmax fmin\n"
               "  --showiobandwidth\t\t\t  "
               "[LINKID(P0-P5,G0-G3)][BW(AGG_BW)]"
               "\t Show IO bandwidth\n"
               "  --showxGMIbandwidth\t\t\t  [LINKID(P0-P5,G0-G3)]\n"
               "\t\t\t\t          [BW(AGG_BW,RD_BW,WR_BW)]"
               "\t\t Show current xGMI bandwidth\n"
               "  --setxGMIlinkwidthrange\t\t  [MIN(0,1,2)]"
               "[MAX(0,1,2)]\t\t Set xGMIlink width, max value >= "
               "min value\n"
               "  --APBDisable\t\t\t\t  [PSTATE(0,1,2)]\t\t\t"
               " APB Disable specifies DFP-State, 0 is highest & 2 is\n"
               "\t\t\t\t\t\t\t\t\t\t the lowest DF P-state\n"
               "  --enabledfpstatedynamic\t\t  \t\t\t\t\t "
               "Set df pstate dynamic\n"
               "  --showfclkmclkuclk\t\t\t  \t\t\t\t\t "
               "Show df clock, memory clock and umc clock frequencies\n"
               "  --setlclkdpmlevel\t\t\t  [LCLKID(0-3)][MAXDPM]"
               "[MINDPM]\t\t Set max and min lclk dpm level on a given NBIO per socket, valid DPM values\n"
               "\t\t\t\t\t\t\t\t\t\t from 0 - 3, max value >= min value\n"
               "  --showraplcore\t\t\t  [THREAD]\t\t\t\t "
               "Show running average power on specified core\n"
               "  --showraplpkg\t\t\t\t  \t\t\t\t\t "
               "Show running average power on pkg\n"
               "  --showprocbasefreq\t\t\t  \t\t\t\t\t "
               "Show processor base frequency\n"
               "  --setdfpstaterange\t\t\t  [MAX_PSTATE]"
               "[MIN_PSTATE]\t\t Set data fabric pstate range, valid values\n"
               "\t\t\t\t\t\t\t\t\t\t from 0 - 2. max pstate <= min pstate\n"
               "  --showlclkdpmlevelrange\t\t  [LCLKID(0~3)]\t\t\t\t "
               "Show max and min LCLK DPM level range on a given NBIO per socket\n"
               "  --showucoderevision\t\t\t  \t\t\t\t\t "
               "Show micro code revision number\n"
               "  --rasoverridedelay\t\t\t"
               "  [DELAYVALUE(5 -120 mins)\n\t\t\t\t\t  "
               "[DISABLEDELAY(0 - 1)][STOPDELAY(0 -1)] "
               "Override delay reset cpu on sync flood\n"
               "  --rasresetonsyncflood\t\t\t \t\t\t\t\t "
               "Request warm reset after sync flood\n"
               "  --showrasdferrvaliditycheck\t\t  [DF_BLOCK_ID]\t\t\t\t "
               "Show RAS DF error validity check for a given blockID\n"
               "  --showrasdferrdump\t\t\t  [OFFSET][BLK_ID][BLK_INST]\t\t "
               "Show RAS DF error dump\n"
	       "  --showrtc\t\t\t\t\t\t\t\t	 Show RTC timer value\n"
	       "  --showrasrterrvalidityck\t\t  [ERR_CATEGORY(0-2)][REQ_TYPE(0-1)]\t "
	       "BMC RAS runtime error validity check\n"
	       "  --showrasrterrinfo\t\t\t  [OFFSET][CATEGORY][VALID_INST]\t "
	       "BMC RAS runtime error Info\n"
	       "  --setraserrthreshold\t\t\t  [CATEGORY(0-3)][ERR_CT][MAX_INTR_RATE] "
	       "BMC RAS error threshold. Valid category values are 0: MCA, 1: DRAM CECC, "
	       "\n\t\t\t\t\t\t\t\t\t\t 2: PCIE, 3: UMC MCA\n"
	       "  --setrasoobconfig\t\t\t  [MCA_MISC0_ERR_CNTR_EN(0,1)]"
	       "\n\t\t\t\t\t  [DRAM_ERR_CNTR_MD(0 - 2)]"
	       "\n\t\t\t\t\t  [DRAM_LEAK_RATE(0 - 31)]"
	       "\n\t\t\t\t\t  [PCIE_ERR_RPRT_EN(0,1)]"
	       "\n\t\t\t\t\t  [MCA_ERR_RPRT_EN]"
	       "\t\t\t Configures OOB state infrastructure in SoC\n"
	       "  --getrasoobconfig\t\t\t  \t\t\t\t\t "
	       "Show BMC ras oob configuration\n"
	       "  --getrasmcaruntimerrinfo\t\t  \t\t\t\t\t "
	       "Show BMC ras mca runtime error info\n"
	       "  --getraspcieruntimerrinfo\t\t  \t\t\t\t\t "
	       "Show BMC ras pcie runtime error info\n"
	       "  --getrasdrameccruntimerrinfo\t\t  \t\t\t\t\t "
	       "Show BMC ras dram ecc runtime error info\n"
	       "  --getdimmserialnum\t\t\t  [DIMM_ADDR(HEX)]\t\t\t"
	       " Show DIMM serial number\n"
	       "  --getdimmregdata\t\t\t  [DIMM_ADDR(HEX)][LID(HEX)]"
	       "\n\t\t\t\t\t  [REG_OFFSET(HEX)][REG_SPACE] \t\t"
	       " Show DIMM register data.Valid LID codes are SPDHub(0xA),\n"
	       "\t\t\t\t\t\t\t\t\t\t PMIC0(0x9), TS0(0x2), TS1(0x6)\n"
	       "  --getsmufwversion\t\t\t  \t\t\t\t\t "
	       "Show SMC FW version\n"
	       "  --setcpurailfreqpolicy\t\t  [POLICY(0-1)] \t\t\t"
	       " Set CPU rail frequency Policy. Valid values are 0 - 1\n"
	       "  --getcpurailfreqpolicy\t\t\t  \t\t\t\t "
	       "Show CPU rail frequency policy\n"
	       "  --setdfcenable\t\t\t  [STATE(0-1)] \t\t\t\t"
	       " Set DFC enable \n"
	       "  --getdfcenable\t\t\t  \t\t\t\t\t "
	       "Show dfc enable state \n"
	       "  --getavgdramthrottle\t\t\t  \t\t\t\t\t "
	       "Show avg dram throttle for all channels \n"
	       "  --getchdramthrottle\t\t\t  [DIMM_ADDR(HEX)] \t\t\t "
	       "Show channel dram throttle \n"
		"  --getavgdramthrwithstatus\t\t\t  \t\t\t\t "
	       "Show avg dram throttle, ODTS enable status, ODTS thermal throttle status,\n"
	       "\t\t\t\t\t\t\t\t\t\t TSOD enable status and TSOD thermal throttle status for all channels \n"
	       "  --getchdramthrwithstatus\t\t  [DIMM_ADDR(HEX)] \t\t\t "
	       "Show channel dram throttle, ODTS enable status, ODTS thermal throttle \n"
	       "\t\t\t\t\t\t\t\t\t\t status, TSOD enable status and TSOD thermal throttle status \n"
	       "  --gethottestdimmtemprangeandrefreshrate\t\t"
	       "\t\t\t Show hottest dimm address, temp range, refresh rate\n"
	       "  --writepcielinkcontrol\t\t  [PCIE_LINK_CONTROL(0,1)] \t\t "
	       "Enable(0)/Disable(1) PCIe link control\n");
}

static void fam_19_mod_10_mailbox_commands(void)
{
	printf("  --showppinfuse\t\t\t\t\t\t\t\t Show 64bit PPIN"
	       " fuse data\n"
	       "  --getpostcode\t\t\t\t  [POST_CODE_OFFSET(0 - 7 or s"
	       "/summary)] Get post code for the given offset or"
	       " recent 8 offsets\n"
	       "  --setdimmpower\t\t\t  [DIMM_ADDR][POWER(mW)]"
	       "[UPDATERATE(ms)] Set dimm power reported"
	       " by bmc\n"
	       "  --setdimmthermalsensor\t\t  [DIMM_ADDR][TEMP(°C)]"
	       "[UPDATERATE(ms)]  Set dimm temperature "
	       "reported by bmc\n"
	       "  --showPCIeconfigspacedata\t\t  [SEGMENT][OFFSET]\n"
	       "\t\t\t\t\t  [BUS(HEX)][DEVICE(HEX)][FUNC]\t\t Show "
	       "32 bit data from extended PCI config space\n"
	       "  --setpcieconfig\t\t\t  [SEGMENT][OFFSET][BUS(HEX)]\n"
	       "\t\t\t\t\t  [DEVICE(HEX)][FUNC(HEX)][WR_DATA(HEX)] "
	       "Set 32 bit data to extended PCI config space\n"
	       "  --showvalidmcabanks\t\t\t\t\t\t\t\t "
	       "Show number of MCA banks & bytes/bank with valid\n"
	       "\t\t\t\t\t\t\t\t\t\t status after a fatal error\n"
	       "  --showrasmcamsr\t\t\t  [MCA_BANK_INDEX][OFFSET]"
	       "\t\t Show 32 bit data from specified MCA bank and "
	       "offset\n"
	       "  --showfchresetreason\t\t\t  [FCHID(0 or 1)]\t\t"
	       "\t Show previous reset reason from FCH register\n"
	       "  --showdimmtemprangeandrefreshrate\t  [DIMM_ADDR]"
	       "\t\t\t\t Show per dimm temp range and refresh rate\n"
	       "  --showdimmpower\t\t\t  [DIMM_ADDR]\t\t\t\t "
	       "Show per dimm power consumption\n"
	       "  --showdimmthermalsensor\t\t  [DIMM_ADDR]\t\t\t"
	       "\t Show per dimm thermal sensor\n"
	       "  --showsktfreqlimit\t\t\t\t\t\t\t\t "
	       "Show per socket current active freq limit\n"
	       "  --showcclklimit\t\t\t  [THREAD]\t\t\t\t "
	       "Show core clock limit\n"
	       "  --showsvitelemetryallrails\t\t\t\t\t\t\t "
	       "Show svi based pwr telemetry for all rails\n"
	       "  --showsktfreqrange\t\t\t\t\t\t\t\t "
	       "Show per socket fmax fmin\n"
	       "  --showiobandwidth\t\t\t  "
	       "[LINKID(P0-P3,G0-G3)][BW(AGG_BW)]"
	       "\t Show IO bandwidth\n"
	       "  --showxGMIbandwidth\t\t\t  [LINKID(P0-P3,G0-G3)]\n"
	       "\t\t\t\t          [BW(AGG_BW,RD_BW,WR_BW)]"
	       "\t\t Show current xGMI bandwidth\n"
	       "  --setGMI3linkwidthrange\t\t  [MIN(0,1,2)]"
	       "[MAX(0,1,2)]\t\t Set GMI3link width, max value >= "
	       "min value\n"
	       "  --setxGMIlinkwidthrange\t\t  [MIN(0,1,2)]"
	       "[MAX(0,1,2)]\t\t Set xGMIlink width, max value >= "
	       "min value\n"
	       "  --APBDisable\t\t\t\t  [PSTATE(0,1,2)]\t\t\t"
	       " APB Disable specifies DFP-State, 0 is highest & 2 is\n"
	       "\t\t\t\t\t\t\t\t\t\t the lowest DF P-state\n"
	       "  --enabledfpstatedynamic\t\t  \t\t\t\t\t "
	       "Set df pstate dynamic\n"
	       "  --showfclkmclkuclk\t\t\t  \t\t\t\t\t "
	       "Show df clock, memory clock and umc clock frequencies\n"
	       "  --setlclkdpmlevel\t\t\t  [NBIOID(0-3)][MAXDPM]"
	       "[MINDPM]\t\t Set dpm level range, valid dpm values\n"
	       "\t\t\t\t\t\t\t\t\t\t from 0 - 3, max value >= min value\n"
	       "  --showprocbasefreq\t\t\t  \t\t\t\t\t "
	       "Show processor base frequency\n"
	       "  --setPCIegenratectrl\t\t\t  [MODE(0,1,2)]\t\t\t\t "
	       "Set PCIe link rate control\n"
	       "  --setpwrefficiencymode\t\t  [MODE(0 - 5)]\t\t\t\t "
	       "Set power efficiency profile policy\n"
	       "  --showraplcore\t\t\t  [THREAD]\t\t\t\t "
	       "Show runnng average power on specified core\n"
	       "  --showraplpkg\t\t\t\t  \t\t\t\t\t "
	       "Show running average power on pkg\n"
	       "  --showprocbasefreq\t\t\t  \t\t\t\t\t "
	       "Show processor base frequency\n"
	       "  --setPCIegenratectrl\t\t\t  [MODE(0,1,2)]\t\t\t\t "
	       "Set PCIe link rate control\n"
	       "  --setpwrefficiencymode\t\t  [MODE(0,1,2)]\t\t\t\t "
	       "Set power efficiency profile policy\n"
	       "  --setdfpstaterange\t\t\t  [MAX_PSTATE]"
	       "[MIN_PSTATE]\t\t Set data fabric pstate range, valid values\n"
	       "\t\t\t\t\t\t\t\t\t\t from 0 - 2. max pstate <= min pstate\n"
	       "  --showlclkdpmlevelrange\t\t  [NBIOID(0~3)]\t\t\t\t "
	       "Show LCLK DPM level range\n"
	       "  --showucoderevision\t\t\t  \t\t\t\t\t "
	       "Show micro code revision number\n"
	       "  --rasoverridedelay\t\t\t"
	       "  [DELAYVALUE(5 -120 mins)\n\t\t\t\t\t  "
	       "[DISABLEDELAY(0 - 1)][STOPDELAY(0 -1)] "
	       "Override delay reset cpu on sync flood\n"
	       "  --rasresetonsyncflood\t\t\t \t\t\t\t\t "
	       "Request warm reset after sync flood\n"
	       "  --showrasdferrvaliditycheck\t\t  [DF_BLOCK_ID]\t\t\t\t "
	       "Show RAS DF error validity check for a given blockID\n"
	       "  --showrasdferrdump\t\t\t  [OFFSET][BLK_ID][BLK_INST]\t\t "
	       "Show RAS DF error dump\n");
}

static void fam_1A_mod_00_mailbox_commands(void)
{
	printf("  --showrtc\t\t\t\t\t\t\t\t	 Show RTC timer value\n"
	       "  --showrasrterrvalidityck\t\t  [ERR_CATERGORY(0-2)]"
	       "REQ_TYPE(0-1)]\t BMC RAS runtime error validity check\n"
	       "  --showrasrterrinfo\t\t\t  [OFFSET][CATEGORY][VALID_INST]\t "
	       "BMC RAS runtime error Info\n"
	       "  --setraserrthreshold\t\t\t  [CATEGORY(0-2)][ERR_CT][MAX_INTR_RATE] "
	       "BMC RAS error threshold\n"
	       "  --setrasoobconfig\t\t\t  [MCA_MISC0_ERR_CNTR_EN(0,1)]"
	       "\n\t\t\t\t\t  [DRAM_ERR_CNTR_MD(0 - 2)]"
	       "\n\t\t\t\t\t  [DRAM_LEAK_RATE(0 - 31)]"
	       "\n\t\t\t\t\t  [PCIE_ERR_RPRT_EN(0,1)]"
	       "\n\t\t\t\t\t  [MCA_ERR_RPRT_EN]"
	       "\t\t\t Configures OOB state infrastructure in SoC\n"
	       "  --getrasoobconfig\t\t\t  \t\t\t\t\t "
	       "Show BMC ras oob configuration\n"
	       "  --getrasmcaruntimerrinfo\t\t  \t\t\t\t\t "
	       "Show BMC ras mca runtime error info\n"
	       "  --getraspcieruntimerrinfo\t\t  \t\t\t\t\t "
	       "Show BMC ras pcie runtime error info\n"
	       "  --getrasdrameccruntimerrinfo\t\t  \t\t\t\t\t "
	       "Show BMC ras dram ecc runtime error info\n"
	       "  --getdimmserialnum\t\t\t  [DIMM_ADDR(HEX)]\t\t\t"
	       " Show DIMM serial number\n"
	       "  --getspddata\t\t\t\t  [DIMM_ADDR(HEX)][LID(HEX)]"
	       "\n\t\t\t\t\t  [REG_OFFSET(HEX)][REG_SPACE] \t\t"
	       " Show DIMM SPD register data\n"
	       "  --getsmufwversion\t\t\t  \t\t\t\t\t "
	       "Show SMC FW version\n"
	       "  --setxgmipstaterange\t\t\t  [MIN_PSTATE][MAX_PSTATE]\t\t"
	       " Set xgmi pstate range.Valid values are 0 -1. Max value \n"
	       "\t\t\t\t\t\t\t\t\t\t must be <= min value\n"
	       "  --setcpurailfreqpolicy\t\t  [POLICY(0-1)] \t\t\t"
	       " Set CPU rail frequency Policy. Valid values are 0 - 1\n"
	       "  --getcpurailfreqpolicy\t\t\t  \t\t\t\t "
	       "Show CPU rail frequency policy\n"
	       "  --setdfcenable\t\t\t  [STATE(0-1)] \t\t\t\t"
	       " Set DFC enable \n"
	       "  --getdfcenable\t\t\t  \t\t\t\t\t "
	       "Show dfc enable state \n"
	       "  --getavgdramthrottle\t\t\t  \t\t\t\t\t "
	       "Show avg dram throttle for all channels \n"
	       "  --getchdramthrottle\t\t\t  [DIMM_ADDR(HEX)] \t\t\t "
	       "Show channel dram throttle \n"
		"  --getavgdramthrwithstatus\t\t\t  \t\t\t\t "
	       "Show avg dram throttle, ODTS enable status, ODTS thermal throttle status,\n"
	       "\t\t\t\t\t\t\t\t\t\t TSOD enable status and TSOD thermal throttle status for all channels \n"
	       "  --getchdramthrwithstatus\t\t  [DIMM_ADDR(HEX)] \t\t\t "
	       "Show channel dram throttle, ODTS enable status, ODTS thermal throttle \n"
	       "\t\t\t\t\t\t\t\t\t\t status, TSOD enable status and TSOD thermal throttle status \n"
	       "  --writepcielinkcontrol\t\t  [PCIE_LINK_CONTROL(0,1)] \t\t "
	       "Enable(0)/Disable(1) PCIe link control\n");
}

static void get_common_mailbox_commands(char *exe_name)
{
	printf("Usage: %s  [SOC_DIE_NUM] [Option]"
	       "\nOption:\n"
	       "\n< MAILBOX COMMANDS [params] >:\n"
	       "  --showmailboxsummary\t\t\t\t\t\t\t\t "
	       "Get summary of the mailbox commands\n"
	       "  -p, (--showpower)\t\t\t\t\t\t\t\t "
	       "Get Power for a given socket in Watts\n"
	       "  -t, (--showtdp)\t\t\t\t\t\t\t\t "
	       "Get TDP for a given socket in Watts\n"
	       "  -s, (--setpowerlimit)\t\t\t  [POWER]\t\t\t\t "
	       "Set powerlimit for a given socket in mWatts\n"
	       "  -b, (--showboostlimit)\t\t  [THREAD]\t\t\t\t "
	       "Get APML and BIOS boostlimit for a given core index "
	       "in MHz\n"
	       "  -d, (--setapmlboostlimit)\t\t  [THREAD]"
	       "[BOOSTLIMIT]\t\t\t Set APML boostlimit for a given "
	       "core in MHz\n"
	       "  -a, (--setapmlsocketboostlimit)\t  [BOOSTLIMIT]"
	       "\t\t\t\t Set APML boostlimit for all cores in a "
	       "socket in MHz\n"
	       "  --showdramthrottle\t\t\t  \t\t\t\t\t "
	       "Show dram throttle\n"
	       "  --set_and_verify_dramthrottle\t\t  [0 to 80%%]"
	       "\t\t\t\t Set DRAM THROTTLE for a given socket\n"
	       "  --showprochotstatus\t\t\t  \t\t\t\t\t "
	       "Show prochot status\n"
	       "  --showprochotresidency\t\t  \t\t\t\t\t "
	       "Show prochot residency\n"
	       "  --showiodbist\t\t\t\t  \t\t\t\t\t "
	       "Show IOD bist status\n"
	       "  --showccdbist\t\t\t\t  [CCDINSTANCE]\t\t\t\t "
	       "Show CCD bist status\n"
	       "  --showccxbist\t\t\t\t  [CCXINSTANCE]\t\t\t\t "
	       "Show CCX bist status\n"
	       "  --showcclkfreqlimit\t\t\t\t\t\t\t\t Get "
	       "cclk freqlimit for a given socket in MHz\n"
	       "  --showc0residency\t\t\t\t\t\t\t\t Show "
	       "c0_residency for a given socket\n"
	       "  --showddrbandwidth\t\t\t\t\t\t\t\t Show "
	       "DDR Bandwidth of a system\n"
	       "  --showpowerconsumed\t\t\t  \t\t\t\t\t "
	       "Show consumed power\n", exe_name);
}

static void get_rmi_commands(char *exe_name)
{
	printf("Usage: %s [SOC_DIE_NUM] [Option]"
	       "\nOption:\n"
	       "\n< SB-RMI COMMANDS >:\n"
	       "  --showrmiregisters\t\t\t\t\t\t\t Get "
	       "values of SB-RMI reg commands for a given socket\n"
	       "  --clearrasstatusregister\t\t  [RAS_STATUS_VALUE]\t\t "
	       "Clear the RAS status register value\n"
	       "  --clearmp0alertstatus\t\t\t\t\t\t\t Clear "
	       "MP0 alert status\n"
	       , exe_name);
}

static void get_tsi_commands(char *exe_name)
{
	printf("Usage: %s [SOC_DIE_NUM] [Option]"
	       "\nOption:\n"
	       "\n< SB-TSI COMMANDS [params] >:\n"
	       "  --showtsiregisters\t\t\t  \t\t\t\t\t Get "
	       "values of SB-TSI reg commands for a given socket\n"
	       "  --set_verify_updaterate\t	  [UPDATERATE]"
	       "\t\t\t\t Set APML Freq Update rate."
	       "Valid values are 2^i, i=[-4,6]\n"
	       "  --sethightempthreshold\t	  [TEMP(°C)]\t\t"
	       "\t\t Set APML High Temp Threshold\n"
	       "  --setlowtempthreshold\t\t	  [TEMP(°C)]\t\t"
	       "\t\t Set APML Low Temp Threshold\n"
	       "  --settempoffset\t\t	  [VALUE]\t\t\t\t Set "
	       "APML processor Temp Offset, VALUE = [-CPU_TEMP(°C), 127 "
	       "°C]\n"
	       "  --settimeoutconfig\t\t	  [VALUE]\t\t"
	       "\t\t Set/Reset APML processor timeout config, VALUE = 0 or "
	       "1\n"
	       "  --setalertthreshold\t\t\t  [VALUE]\t\t\t\t "
	       "Set APML processor alert threshold sample, VALUE = 1 to 8\n"
	       "  --setalertconfig\t\t	  [VALUE]\t\t\t\t "
	       "Set/Reset APML processor alert config, VALUE = 0 or 1\n"
	       "  --setalertmask\t\t	  [VALUE]\t\t\t\t "
	       "Set/Reset APML processor alert mask, VALUE = 0 or 1\n"
	       "  --setrunstop\t\t\t	  [VALUE]\t\t\t\t "
	       "Set/Reset APML processor runstop, VALUE = 0 or 1\n"
	       "  --setreadorder\t\t	  [VALUE]\t\t\t\t "
	       "Set/Reset APML processor read order, VALUE = 0 or 1\n"
	       "  --setara\t\t\t	  [VALUE]\t\t\t\t "
	       "Set/Reset APML processor ARA, VALUE = 0 or 1\n"
	       "  --readtsiraw\t\t\t\t  [REGISTER(hex)]\t\t\t "
	       "Read TSI raw register value in hexa\n", exe_name);
}

static void get_reg_access_commands(char *exe_name)
{
	printf("Usage: %s [SOC_DIE_NUM] [Option]"
	       "\nOption:\n"
	       "\n< REG-ACCESS [params] >:\n"
	       "  --readregister\t\t\t  [sbrmi/sbtsi][REGISTER(hex)]\t\t\t "
	       "Read a register\n"
	       "  --writeregister\t\t\t  [sbrmi/sbtsi][REGISTER(hex)]"
	       "[VALUE(int)]\t Write to a register\n"
	       "  --readrmiregister\t\t\t  [REGISTER(hex)]\t\t\t\t "
	       "Read a rmi register\n"
	       "  --readtsiregister\t\t\t  [REGISTER(hex)]\t\t\t\t "
	       "Read a tsi register\n"
	       "  --writermiregister\t\t\t  [REGISTER(hex)]"
	       "[VALUE(int)]\t\t\t Write to a rmi register\n"
	       "  --writetsiregister\t\t\t  [REGISTER(hex)]"
	       "[VALUE(int)]\t\t\t Write to a tsi register\n"
	       "  --readmsrregister\t\t\t  [REGISTER(hex)]"
	       "[thread]\t\t\t Read MSR register\n"
	       "  --readcpuidregister\t\t\t  [FUN(hex)]"
	       "[EXT_FUN(hex)][thread]\t\t Read CPUID register\n", exe_name);
}

static void get_cpuid_access_commands(char *exe_name)
{
	printf("Usage: %s [SOC_DIE_NUM] [Option]"
	       "\nOption:\n"
	       "\n< CPUID [params] >:\n"
	       "  --showthreadspercoreandsocket\t  \t\t\t\t "
	       "Show threads per core and socket\n"
	       "  --showccxinfo\t\t\t\t\t "
	       "\t\t Show max num of cores per ccx and "
	       "ccx instances\n"
	       "  --showSMTstatus\t\t\t  \t\t\t "
	       "Show SMT enabled status\n", exe_name);
}

static void get_recovery_commands(char *exe_name)
{
	printf("Usage: %s [SOC_DIE_NUM] [Option]"
	       "\nOption:\n"
	       "\n< RECOVERY [params] >:\n"
	       "  --apml_recovery \t\t[client(0,1)]\t\t "
	       "Recovers APML client from bad state. client 0 ->"
	       " SBRMI, 1 -> SBTSI\n", exe_name);
}

static void fam_1A_mod_50_rmi_commands(void) {
	printf("  --clearswasyncalertstatus\t\t\t\t\t\t Clear "
	       "software async alert status\n"
	       "  --setswasyncalertmask\t\t\t  [ALERT_MASK(0,1]\t\t Set "
	       "to 0(Enable) or 1 (disable) Alert-L signaling\n"
		   "  --getsbrmireadystatus\t\t\t\t\t\t\t Get "
		   " SBRMI ready status\n"
		   "  --clearcpushutdownerror\t\t\t\t\t\t Clear "
		   "CPU shut down error\n"
	       "  --gpioassertionforasyncalert\t\t  [CTRL_ALERTL(0,1)]\t\t GPIO Assertion for async alert\n"
	       "\t\t\t\t\t\t\t\t\t Set to 0(Enable) or 1 (disable) Alert-L signaling\n"
	       "  --gpioassertionformailbox\t\t  [CTRL_ALERTL(0,1)]\t\t GPIO Assertion for mailbox alert\n"
	       "\t\t\t\t\t\t\t\t\t Set to 0(Enable) or 1 (disable) Alert-L signaling\n");

}

static oob_status_t show_module_commands(char *exe_name, char *command)
{
	struct processor_info plat_info[1];
	uint8_t soc_die_num = 0, hbm_temp = 0, p_type = 0;
	oob_status_t ret = OOB_SUCCESS;

	if (!strcmp(command, "mailbox") || !strcmp(command, "1")) {
		ret = get_proc_type(soc_die_num, &p_type);

		switch(p_type) {
		case NOT_SUPPORTED:
			printf(RED"Note: Help section not available as platform "
			       "identification failed, will not be able to \n"
			       "run the RMI messages.\n"RESET);
			return ret;
		case FAM_19_MOD_10:
		case FAM_19_MOD_A0:
			get_common_mailbox_commands(exe_name);
			fam_19_common_mailbox_commands();
			fam_19_mod_10_mailbox_commands();
			break;
		case FAM_19_MOD_90:
			// MI300A mailbox commands
			get_mi300_mailbox_commands(exe_name);
			break;
		case FAM_1A_MOD_00:
		case FAM_1A_MOD_10:
			get_common_mailbox_commands(exe_name);
			fam_19_mod_10_mailbox_commands();
			fam_1A_mod_00_mailbox_commands();
			break;
		case FAM_1A_MOD_50:
			get_common_mailbox_commands(exe_name);
			fam_1A_mod_50_common_mailbox_commands();
			fam_1A_mod_50_mailbox_commands();
			break;
		default:
			get_common_mailbox_commands(exe_name);
			fam_19_common_mailbox_commands();
			fam_19_mod_00_specific_mailbox_commands();
			if (ret)
				printf(RED"\nNOTE: Limited functionality has been displayed as platform "
				       "identification has failed. Please recover the RMI device."RESET);
			break;
		}
	} else if (!strcmp(command, "sbrmi") || !strcmp(command, "2")) {
		ret = get_proc_type(soc_die_num, &p_type);
		switch(p_type) {
		case FAM_1A_MOD_50:
			get_rmi_commands(exe_name);
			fam_1A_mod_50_rmi_commands();
			break;
		default:
			get_rmi_commands(exe_name);
			break;
		}
	} else if (!strcmp(command, "sbtsi") || !strcmp(command, "3")) {

		/* Read hbm max temp to verify whether the platform is mi300 or not */
		ret = read_sbtsi_max_hbm_temp_int(soc_die_num, &hbm_temp);
		if (ret) {
			printf(RED"Note: Help section not available as sbtsi module "
			       "has failed, will not be able to \n"
			       "run the TSI messages.\n"RESET);
			return ret;
		}
		if (hbm_temp)
			/* MI300A TSI commands */
			get_mi300_tsi_commands(exe_name);
		else
			get_tsi_commands(exe_name);

	} else if (!strcmp(command, "reg-access") || !strcmp(command, "4")) {
		get_reg_access_commands(exe_name);
	} else if (!strcmp(command, "cpuid") || !strcmp(command, "5")) {
		get_cpuid_access_commands(exe_name);
	} else if (!strcmp(command, "recovery") || !strcmp(command, "6")) {
		get_recovery_commands(exe_name);
	} else {
		printf("Failed: Invalid command, Err[%d]: %s\n",
		       OOB_INVALID_INPUT,
		       esmi_get_err_msg(OOB_INVALID_INPUT));
		ret = OOB_INVALID_INPUT;
	}
	return ret;
}

static oob_status_t show_apml_mailbox_cmds(uint8_t soc_die_num)
{
	struct max_ddr_bw max_ddr;
	struct nbio_err_log nbio;
	struct pstate_freq df_pstate;
	double energy;
	float uprat, prochot_res;
	uint32_t core_id, instance, nbio_reg, buffer;
	uint32_t power_avg, power_cap, power_max;
	uint32_t tdp_avg, tdp_min, tdp_max;
	uint32_t cclk, residency, threads_per_core;
	uint32_t max_bw, utilized_bw, utilized_pct;
	uint32_t bios_boost, esb_boost, threads_per_soc;
	uint32_t dram_thr, prochot, power;
	uint16_t ccx_instances, max_cores_per_ccx;
	uint32_t nbio_data, iod, ccd, ccx_res;
	uint16_t bytespermca;
	uint16_t numbanks;
	uint16_t freq;
	uint16_t urate;
	uint16_t fmax;
	uint16_t fmin;
	uint16_t fclk;
	uint16_t mclk;
	uint8_t uclk, index, rev, p_type = 0;
	char *source_type[ARRAY_SIZE(freqlimitsrcnames)] = {NULL};
	char *bist_status;
	bool is_mi300 = false;
	oob_status_t ret;

	nbio.quadrant = 0x03;
	nbio.offset = 0x20;

	printf("\t\t *** SB-RMI MAILBOX SUMMARY ***\n");
	printf("------------------------------------------------------------"
	       "----\n");
	printf("| Function [INPUT VALUE] (UNITS)\t | VALUE");
	printf("\n------------------------------------------------------------"
	       "----\n");

	ret = get_proc_type(soc_die_num, &p_type);
	if (p_type == NOT_SUPPORTED) {
		printf("Failed to get platform info  Err[%d]:%s\n",
		       ret, esmi_get_err_msg(ret));
		return ret;
	}
	if (p_type == FAM_19_MOD_90)
		is_mi300 = true;
	usleep(APML_SLEEP);
	printf("| Power (Watts)\t\t\t\t |");
	ret = read_socket_power(soc_die_num, &power_avg);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.3f", (double)power_avg/1000);

	usleep(APML_SLEEP);
	printf("\n| PowerLimit (Watts)\t\t\t |");
	ret = read_socket_power_limit(soc_die_num, &power_cap);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.3f", (double)power_cap/1000);

	usleep(APML_SLEEP);
	printf("\n| PowerLimitMax (Watts)\t\t\t |");
	ret = read_max_socket_power_limit(soc_die_num, &power_max);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.3f", (double)power_max/1000);

	usleep(APML_SLEEP);
	printf("\n| TDP Avg (Watts)\t\t\t |");
	ret = read_tdp(soc_die_num, &tdp_avg);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.3f", (double)tdp_avg/1000);

	usleep(APML_SLEEP);
	printf("\n| TDP Min (Watts)\t\t\t |");
	ret = read_min_tdp(soc_die_num, &tdp_min);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.3f", (double)tdp_min/1000);

	usleep(APML_SLEEP);
	printf("\n| TDP Max (Watts)\t\t\t |");
	ret = read_max_tdp(soc_die_num, &tdp_max);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.3f", (double)tdp_max/1000);

	usleep(APML_SLEEP);
	if (!is_mi300) {
		printf("\n| DDR BANDWIDTH \t\t\t |");
		ret = read_ddr_bandwidth(soc_die_num, &max_ddr);
		if (ret) {
			printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
		} else {
			printf("\n| \tDDR Max BW (GB/s)\t\t |");
			printf(" %-17d", max_ddr.max_bw);
			printf("\n| \tDDR Utilized BW (GB/s)\t\t |");
			printf(" %-17d", max_ddr.utilized_bw);
			printf("\n| \tDDR Utilized Percent(%%)\t\t |");
			printf(" %-17d", max_ddr.utilized_pct);
		}
	}
	usleep(APML_SLEEP);
	core_id = 0x0;
	printf("\n| BIOS Boostlimit [0x%x] (MHz)\t\t |", core_id);
	ret = read_bios_boost_fmax(soc_die_num, core_id, &bios_boost);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17u", bios_boost);

	usleep(APML_SLEEP);
	printf("\n| APML Boostlimit [0x%x] (MHz)\t\t |", core_id);
	ret = read_esb_boost_limit(soc_die_num, core_id, &esb_boost);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17u", esb_boost);

	usleep(APML_SLEEP);
	if (!is_mi300) {
		printf("\n| DRAM_Throttle  (%%)\t\t\t |");
		ret = read_dram_throttle(soc_die_num, &dram_thr);
		if (ret)
			printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
		else
			printf(" %-17u", dram_thr);
	}

	usleep(APML_SLEEP);
	printf("\n| PROCHOT Status\t\t\t |");
	ret = read_prochot_status(soc_die_num, &prochot);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17s", prochot ? "PROCHOT" : "NOT_PROCHOT");

	usleep(APML_SLEEP);
	printf("\n| PROCHOT Residency (%%)\t\t\t |");
	ret = read_prochot_residency(soc_die_num, &prochot_res);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.2f", prochot_res);

	usleep(APML_SLEEP);
	nbio_reg = (((uint32_t)(nbio.quadrant) << 24) | nbio.offset);
	printf("\n| NBIO_Err_Log_Reg [0x%x]\t\t |", nbio_reg);
	ret = read_nbio_error_logging_register(soc_die_num, nbio, &nbio_data);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17u", nbio_data);

	usleep(APML_SLEEP);
	printf("\n| IOD/AID_Bist_Result\t\t\t |");
	ret = read_iod_bist(soc_die_num, &iod);

	if (ret) {
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	} else {
		if (p_type == FAM_1A_MOD_50) {
			switch (iod)
			{
			case 0:
				bist_status = "BIST pass on both IOD 0 and 1";
				break;
			case 1:
				bist_status = "Bist pass on IOD0";
				break;
			case 2:
				bist_status = "Bist pass on IOD1";
				break;
			case 3:
				bist_status = "Bist fail on both IOD 0 and 1";
				break;
			default:
				bist_status = "Undefined response";
				break;
			}
		} else {
			bist_status = (iod == 0 ? "Bist pass" : "Bist fail");
		}

		printf(" %-30s", bist_status);
	}
	usleep(APML_SLEEP);
	instance = 0x0;
	printf("\n| CCD/XCD_Bist_Result [0x%x]\t\t |", instance);
	ret = read_ccd_bist_result(soc_die_num, instance, &ccd);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17s", ccd ? "Bist fail" : "Bist pass");

	usleep(APML_SLEEP);
	printf("\n| CCX_Bist_Result [0x%x]\t\t\t |", instance);
	ret = read_ccx_bist_result(soc_die_num, instance, &ccx_res);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" 0x%-15x", ccx_res);
	usleep(APML_SLEEP);
	printf("\n| Curr_Active_Freq_Limit\t\t |");
	ret = read_pwr_current_active_freq_limit_socket(soc_die_num,
							&freq, source_type);

	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else {
		printf("\n| \tFreqlimit (MHz)\t\t\t | %u", freq);
		printf("\n| \tSource \t\t\t\t |");
		display_freq_limit_src_names(source_type);
	}
	usleep(APML_SLEEP);
	printf("\n| Power_Telemetry (Watts)\t\t |");
	ret = read_pwr_svi_telemetry_all_rails(soc_die_num, &power);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17.3f", (float)power / 1000);
	usleep(APML_SLEEP);
	printf("\n| Package_Energy_CORES (MJ)\t\t |");
	ret = read_rapl_pckg_energy_counters(soc_die_num, &energy);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17f", energy);

	usleep(APML_SLEEP);
	printf("\n| Socket_Freq_Range (MHz)\t\t |");
	ret = read_socket_freq_range(soc_die_num, &fmax, &fmin);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else {
		printf("\n| \tFmax \t\t\t\t | %u", fmax);
		printf("\n| \tFmin \t\t\t\t | %u", fmin);
	}
	usleep(APML_SLEEP);
	printf("\n| CPU_Base_Freq (MHz)\t\t\t |");
	ret = read_bmc_cpu_base_frequency(soc_die_num, &freq);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17u", freq);
	usleep(APML_SLEEP);
	printf("\n| Data_Fabric_Freq (MHz)\t\t |");
	ret = read_current_dfpstate_frequency(soc_die_num, &df_pstate);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else {
		printf("\n| \tFclk \t\t\t\t | %u", df_pstate.fclk);
		printf("\n| \tMclk \t\t\t\t | %u", df_pstate.mem_clk);
		printf("\n| \tUclk \t\t\t\t | %u",
		       df_pstate.uclk ? (df_pstate.mem_clk / 2)
		       : df_pstate.mem_clk);
	}

	if (is_mi300)
		get_mi_300_mailbox_cmds_summary(soc_die_num);
	usleep(APML_SLEEP);
	printf("\n| THREADS_PER_CORE\t\t\t |");
	ret = esmi_get_threads_per_core(soc_die_num, &threads_per_core);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17d", threads_per_core);

	usleep(APML_SLEEP);
	printf("\n| THREADS_PER_SOCKET\t\t\t |");
	ret = esmi_get_threads_per_socket(soc_die_num, &threads_per_soc);
	if (ret)
		printf(" Err[%d]:%s", ret, esmi_get_err_msg(ret));
	else
		printf(" %-17d", threads_per_soc);
	printf("\n------------------------------------------------------------"
	       "----\n");
	return OOB_SUCCESS;
}

static void show_smi_parameters(uint8_t soc_die_num)
{
	oob_status_t ret;

	ret = validate_apml_sbrmi_module(soc_die_num);
	if (ret)
		return;
	ret = show_apml_mailbox_cmds(soc_die_num);
	if (ret)
		printf("Failed: For RMI Err[%d]: %s\n", ret,
		       esmi_get_err_msg(ret));

	ret = get_apml_tsi_register_descriptions(soc_die_num);
	if (ret)
		printf("Failed: For TSI Err[%d]: %s\n", ret,
		       esmi_get_err_msg(ret));
}

static void show_smi_message(void)
{
	printf("\n================================= APML System Management "
	       "Interface ====================================\n");
}

static void show_smi_end_message(void)
{
	printf("\n========================================== End of APML SMI "
	       "============================================\n");
}

static void print_apml_usage(char *arg)
{
	printf("Usage: %s <soc_die_num>\n"
		"Where:  soc_die_num : "
	        "[7:0], [3:0]: Socket Number, [7:4]: Die Number\n", arg);
}

/*
 * returns 0 if the given string is a number for the given base, else 1.
 * Base will be 16 for hexdecimal value and 10 for decimal value.
 */
static oob_status_t validate_number(char *str, uint8_t base)
{
	uint64_t buffer_number = 0;
	char *endptr;

	if (base == 10 || base == 0) {
		if (str[0] < '0' || str[0] > '9' )
			return OOB_INVALID_INPUT;
	}

	buffer_number = strtol(str, &endptr, base);
	if (*endptr != '\0')
		return OOB_INVALID_INPUT;

	return OOB_SUCCESS;
}

static void validate_modules(uint8_t soc_die_num, bool *is_sbrmi,
			     bool *is_sbtsi)
{
	oob_status_t ret;

	ret = validate_apml_dependency(soc_die_num, is_sbrmi, is_sbtsi);
	if (ret) {
		if (!*is_sbrmi && !*is_sbtsi)
			printf(RED" SBRMI and SBTSI modules not present.Please insert "
			       "the modules" RESET"\n");
		else if (!*is_sbrmi)
			printf(RED" SBRMI module not present.Please insert "
			       "the module" RESET"\n");
		else if (!*is_sbtsi)
			printf(RED" SBTSI module not present.Please insert "
			       "the module" RESET"\n");
	}
}

/*
 * Parse command line parameters and set data for program.
 * @param argc number of command line parameters
 * @param argv list of command line parameters
 */
static oob_status_t parseesb_args(int argc, char **argv)
{
	long input_val1 = 0, input_val2 = 0, input_val3 = 0;
	struct get_hsmp_cmds_din get_hsmp_cmds_d_in = {0};
	struct set_hsmp_cmds_din set_hsmp_cmds_d_in = {0};
	union ras_df_err_dump df_err = {0};
	struct svi3_vr_cont_data_in svi3_d_in = {0};
	struct ras_rt_err_req_type err_category = {0};
	struct oob_config_d_in oob_config = {0};
	struct run_time_threshold th = {0};
	struct run_time_err_d_in err_d_in = {0};
	struct ras_override_delay d_in = {0};
	struct nbio_err_log nbio;
	struct lclk_dpm_level_range lclk;
	struct pci_address pci_addr;
	struct dimm_power dp_soc_die_num;
	struct dimm_thermal dt_soc_die_num;
	struct dimm_spd_d_in spd_in;
	struct dimm_spd_write spd_w_din = {0};
	struct core_floor_limit_din d_in_core_limit = {0};
	struct pow_eff_mode din_mode = {0};
	struct dimm_pow_din dimm_din = {0};
	struct dimm_sb_reg_d_in dimm_reg_din = {0};
	struct dimm_sb_reg_write dimm_reg_write = {0};
	struct set_ras_action_data_in set_ras_act_din = {0};
	struct get_ras_action_data_in get_ras_act_din = {0};
	uint8_t soc_die_num;
	uint8_t dimm_addr, type;
	struct mca_bank mca_dump;
	struct link_id_bw_type link;
	float uprate;
	float temp;
	int value;
	int power = 0;
	int opt = 0; /* option character */
	int i;
	uint32_t val1;
	uint32_t val2;
	uint32_t val3;
	uint32_t dram_thr;
	uint32_t boostlimit = 0, thread_ind = 0;
	char *val;
	char *end;
	char *link_name;
	char *bw_type;
	oob_status_t ret;
	bool is_sbrmi = false, is_sbtsi = false;
	char *endptr;
	bool sw_async_alert_mask = false;

	//Specifying the expected options
	static struct option long_options[] = {
		{"help",		no_argument,	0,	'h'},
		{"version",		no_argument,	0,	'v'},
		{"showmailboxsummary",  no_argument,    0,      'Y'},
		{"showpower",		no_argument,	0,	'p'},
		{"showtdp",		no_argument,	0,	't'},
		{"setpowerlimit",	required_argument,	0,	's'},
		{"showddrbandwidth",	no_argument,	&flag,	 3 },
		{"rasresetonsyncflood", no_argument, &flag,  4},
		{"showboostlimit",	required_argument,	0,	'b'},
		{"setapmlboostlimit",	required_argument,	0,	'd'},
		{"setapmlsocketboostlimit", required_argument,	0,	'a'},
		{"set_and_verify_dramthrottle", required_argument, 0,   'l'},
		{"showrmiregisters", no_argument,	&flag,   1 },
		{"showtsiregisters", no_argument,	&flag,   1200 },
		{"set_verify_updaterate",   required_argument,	&flag,	1201},
		{"sethightempthreshold", required_argument,	&flag,	1202},
		{"setlowtempthreshold",	required_argument,	&flag,	1203},
		{"settempoffset",	required_argument,	&flag,	1204},
		{"settimeoutconfig",	required_argument,	&flag,	1205},
		{"setalertthreshold",	required_argument,	&flag,	1206},
		{"setalertconfig",	required_argument,	&flag,	1207},
		{"setalertmask",	required_argument,	&flag,	1208},
		{"setrunstop",		required_argument,	&flag,	1209},
		{"setreadorder",	required_argument,	&flag,	1210},
		{"setara",		required_argument,	&flag,	1211},
		{"readtsiraw",		required_argument,	&flag,	1214},
		{"setdimmpower",			required_argument,	0,	'P'},
		{"setdimmthermalsensor",		required_argument,	0,	'T'},
		{"showdimmpower",			required_argument,	0,	'O'},
		{"showdimmthermalsensor",		required_argument,	0,	'E'},
		{"showdimmtemprangeandrefreshrate",	required_argument,	0,	'S'},
		{"showPCIeconfigspacedata",		required_argument,	0,	'R'},
		{"showvalidmcabanks",			no_argument,	&flag,	 5 },
		{"showrasmcamsr",			required_argument,	0,	'D'},
		{"showfchresetreason",			required_argument,	0,	'F'},
		{"showsktfreqlimit",			no_argument,	&flag,	 6 },
		{"showcclklimit",			required_argument,	0,	'C'},
		{"showsvitelemetryallrails",		no_argument,	&flag,	 7 },
		{"showsktfreqrange",			no_argument,	&flag,	 8 },
		{"showiobandwidth",			required_argument,	0,	'B'},
		{"showxGMIbandwidth",			required_argument,	0,	'G'},
		{"setGMI3linkwidthrange",		required_argument,	0,	'H'},
		{"setxGMIlinkwidthrange",		required_argument,	0,	'L'},
		{"APBDisable",				required_argument,	0,	'M'},
		{"enabledfpstatedynamic",		no_argument,	&flag,	 9 },
		{"showfclkmclkuclk",			no_argument,	&flag,	 10},
		{"setlclkdpmlevel",			required_argument,	0,	'N'},
		{"showprocbasefreq",			no_argument,	&flag,	 11},
		{"showraplcore",			required_argument,	0,	'J'},
		{"showraplpkg",				no_argument,	&flag,	 12},
		{"setPCIegenratectrl",			required_argument,	0,	'Z'},
		{"setpwrefficiencymode",		required_argument,	0,	'U'},
		{"setdfpstaterange",			required_argument,	0,	'V'},
		{"readregister",			required_argument,	0,	'e'},
		{"writeregister",			required_argument,	&flag,	 14},
		{"readmsrregister",			required_argument,	&flag,	 15},
		{"readcpuidregister",			required_argument,	&flag,	 16},
		{"showiodbist",				no_argument,	&flag,	 17},
		{"showccdbist",				required_argument,	&flag,	 18},
		{"showccxbist",                         required_argument,      &flag,   19},
		{"shownbioerrorloggingregister",	required_argument,      &flag,   20},
		{"showdramthrottle",			no_argument,	&flag,	21},
		{"showprochotstatus",			no_argument,	&flag,	22},
		{"showprochotresidency",		no_argument,	&flag,	23},
		{"showlclkdpmlevelrange",		required_argument,	&flag,	25},
		{"showucoderevision",		no_argument,		&flag,  26},
		{"showpowerconsumed",		no_argument,		&flag,	27},
		{"showSMTstatus",		no_argument,		&flag,	28},
		{"showthreadspercoreandsocket",	no_argument,		&flag,	29},
		{"showccxinfo",			no_argument,		&flag,	30},
		{"apml_recovery",		required_argument,	&flag,	31},
		{"rasoverridedelay",		required_argument,	&flag,  32},
		{"getpostcode",			required_argument,	&flag,  33},
		{"clearrasstatusregister",	required_argument,	&flag,	34},
		{"showrasrterrvalidityck",	required_argument,	&flag,	35},
		{"showrasrterrinfo",		required_argument,	&flag,	36},
		{"setraserrthreshold",		required_argument,	&flag,	37},
		{"setrasoobconfig",		required_argument,	&flag,	38},
		{"getrasoobconfig",		no_argument,		&flag,	39},
		{"showppinfuse",		no_argument,		&flag,  40},
		{"showrasdferrvaliditycheck",	required_argument,	&flag,  41},
		{"showrasdferrdump",		required_argument,	&flag,  42},
		{"showcclkfreqlimit",		no_argument,		&flag,  43},
		{"showc0residency",		no_argument,		&flag,  44},
		{"showdependency",		no_argument,		&flag,  45},
		{"readtsiregister",		required_argument,	&flag,	46},
		{"writetsiregister",		required_argument,	&flag,	47},
		{"readrmiregister",		required_argument,	&flag,	48},
		{"writermiregister",		required_argument,	&flag,	49},
		{"showrtc",			no_argument,		&flag,  51},
		{"getdimmserialnum",		required_argument,	&flag,	53},
		{"getspddata",			required_argument,	&flag,	54},
		{"getsmufwversion",		no_argument,		&flag,	56},
		{"setpcieconfig",		required_argument,	&flag,	57},
		{"setxgmipstaterange",		required_argument,	&flag,	58},
		{"setcpurailfreqpolicy",	required_argument,	&flag,	59},
		{"getcpurailfreqpolicy",	no_argument,		&flag,	60},
		{"setdfcenable",		required_argument,	&flag,	61},
		{"getdfcenable",		no_argument,		&flag,	62},
                {"getavgdramthrottle",          no_argument,            &flag,  63},
                {"getchdramthrottle",           required_argument,      &flag,  64},
                {"getcc6control",               no_argument,            &flag,  65},
                {"setcc6control",               required_argument,      &flag,  66},
                {"getxgmilinkwidthrange",       no_argument,            &flag,  67},
                {"getapbstate",                 no_argument,            &flag,  68},
                {"getdfpstaterange",            no_argument,            &flag,  69},
                {"getxgmipstaterange",          no_argument,            &flag,  70},
                {"setspddata",                  required_argument,      &flag,  71},
                {"getpc6control",               no_argument,            &flag,  72},
                {"setpc6control",               required_argument,      &flag,  73},
		{"getccdpowconsumption",	required_argument,	&flag,	75},
		{"gettempdelta",		no_argument,		&flag,	76},
		{"pciehaltlinktraining",        required_argument,      &flag,  77},
		{"getsvi3vrtempbyrail",			required_argument,      &flag,  78},
		{"getsupportingerrortypes",	no_argument,		&flag,	79},
		{"getrasmcaruntimerrinfo", no_argument, 		&flag, 80},
		{"getrasdrameccruntimerrinfo", no_argument, 		&flag, 81},
		{"getraspcieruntimerrinfo", no_argument, 		&flag, 82},
		{"getapmlcorefloorlimit",	required_argument,	&flag, 83},
		{"setapmlcorefloorlimit", required_argument,	&flag, 84},
		{"setapmlallcoresfloorlimit", required_argument, &flag, 85},
		{"clearswasyncalertstatus", no_argument,	&flag,	86},
		{"clearmp0alertstatus",		no_argument, 	&flag,	87},
		{"setswasyncalertmask",		required_argument, &flag, 88},
		{"setpwrefficiencyprofile",	required_argument, &flag, 89},
		{"getpwrefficiencyprofile", no_argument, 	&flag, 90},
		{"getenabledHSMPcommands",	required_argument,	&flag,	91},
		{"setenabledHSMPcommands",	required_argument,	&flag,	92},
		{"getsbrmireadystatus",	no_argument,		&flag,	93},
		{"getapmleffectivecorefloorlimit",	required_argument, &flag,	94},
		{"clearcpushutdownerror",	no_argument, 	&flag,	95},
		{"showsocdimmpowerlimit", no_argument,	&flag,	96},
		{"setsocdimmpowerlimit", required_argument,	&flag,	97},
		{"showhipowerdimm", no_argument,	&flag,	98},
		{"showhottestdimmthermalsensor", no_argument,	&flag,	99},
		{"showalldimmpower", no_argument,	&flag,	100},
		{"getdimmregdata",			required_argument,	&flag,	101},
		{"setdimmregdata",			required_argument,	&flag,  102},
		{"getavgdramthrwithstatus",		no_argument,		&flag,	103},
		{"getchdramthrwithstatus",		required_argument,	&flag,	104},
		{"gethottestdimmtemprangeandrefreshrate",	no_argument,	&flag,	105},
		{"writepcielinkcontrol", 		required_argument,	&flag,	106},
		{"gpioassertionforasyncalert", 		required_argument,	&flag,	107},
		{"gpioassertionformailbox", 		required_argument,	&flag,	108},
		{"setbmcrasaction",	      required_argument,  &flag,  405},
		{"getbmcrasactionstatus",     required_argument,  &flag,  406},
		{0,			0,			0,	0},
	};


	int long_index = 0;
	char *helperstring = "+:vhfYpts:b:d:a:u:X:w:x:y:g:j:k:m:n:o:";

	if (argc <= 1) {
		print_apml_usage(argv[0]);
		show_usage(argv[0]);
		return 0;
	}

	while ((opt = getopt_long(argc, argv,  helperstring, long_options, &long_index)) != EOF) {
		switch (opt) {
		case 'h':
			show_usage(argv[0]);
			if (argc == 2)
				return OOB_SUCCESS;
			continue;
		case 'v':
			printf("APML lib version : %d.%d.%d\n",
				apml64_VERSION_MAJOR, apml64_VERSION_MINOR,
				apml64_VERSION_PATCH);
			return OOB_SUCCESS;
		default:
			continue;
		}
	}

	soc_die_num = strtol(argv[1], &endptr, 0);
	if (argc > 2 && (!strcmp(argv[2], "--showdependency"))) {
		soc_die_num = strtol(argv[1], &endptr, 0);
		validate_modules(soc_die_num, &is_sbrmi, &is_sbtsi);
		if (is_sbrmi && is_sbtsi)
			printf(" Both SBRMI and SBTSI modules are present");
		return OOB_SUCCESS;
	}

	if (argc > 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		ret = show_module_commands(argv[0], argv[2]);
		return OOB_SUCCESS;
	}

	if (validate_number(argv[1], 0)) {
		print_apml_usage(argv[0]);
		return OOB_INVALID_INPUT;
	}
	soc_die_num = strtol(argv[1], &endptr, 0);

	if (argc == 2) {
		show_smi_parameters(soc_die_num);
		printf(RED "Try `%s --help' for more information." RESET"\n",
		       argv[0]);
		return 0;
	}

	optind = 2;

	while ((opt = getopt_long(argc, argv, helperstring,
				  long_options, &long_index)) != -1) {
		if (opt == 0 && (*long_options[long_index].flag >= 1200)) {
			ret = validate_apml_sbtsi_module(soc_die_num);
			if (ret) {
				show_smi_end_message();
				return ret;
			}
		} else if (opt == 0 && *long_options[long_index].flag == 31) {
			validate_modules(soc_die_num, &is_sbrmi, &is_sbtsi);
			if (!is_sbtsi || !is_sbrmi) {
				show_smi_end_message();
				return OOB_SUCCESS;
			}
		} else {
			if (opt != '?') {
				ret = validate_apml_sbrmi_module(soc_die_num);
				if (ret) {
					show_smi_end_message();
					return ret;
				}
			}
		}
	if (opt == 's' ||
	    opt == 'b' ||
	    opt == 'a' ||
	    opt == 'l' ||
	    opt == 'd' ||
	    opt == 'R' ||
	    opt == 'D' ||
	    opt == 'F' ||
	    opt == 'S' ||
	    opt == 'E' ||
	    opt == 'O' ||
	    opt == 'C' ||
	    opt == 'H' ||
	    opt == 'L' ||
	    opt == 'M' ||
	    opt == 'Z' ||
	    opt == 'U' ||
	    opt == 'J' ||
	    opt == 'W' ||
	    opt == 0 && ((*long_options[long_index].flag) == 18 ||
			 *(long_options[long_index].flag) == 19 ||
			 (*long_options[long_index].flag) == 31 ||
			 (*long_options[long_index].flag) == 34 ||
			 (*long_options[long_index].flag) == 35 ||
			 (*long_options[long_index].flag) == 41 ||
			 (*long_options[long_index].flag) == 46 ||
			 (*long_options[long_index].flag) == 48 ||
			 (*long_options[long_index].flag) == 53 ||
			 (*long_options[long_index].flag) == 57 ||
			 (*long_options[long_index].flag) == 58 ||
			 (*long_options[long_index].flag) == 59 ||
			 (*long_options[long_index].flag) == 61 ||
			 (*long_options[long_index].flag) == 64 ||
			 (*long_options[long_index].flag) == 66 ||
			 (*long_options[long_index].flag) == 73 ||
			 (*long_options[long_index].flag) == 75 ||
			 (*long_options[long_index].flag) == 83 ||
			 (*long_options[long_index].flag) == 85 ||
			 (*long_options[long_index].flag) == 88 ||
			 (*long_options[long_index].flag) == 89 ||
			 (*long_options[long_index].flag) == 94 ||
			 (*long_options[long_index].flag) == 97 ||
			 (*long_options[long_index].flag) == 104 ||
			 (*long_options[long_index].flag) == 106 ||
			 (*long_options[long_index].flag) == 107 ||
			 (*long_options[long_index].flag) == 108 ||
			 (*long_options[long_index].flag) == 1201 ||
			 (*long_options[long_index].flag) == 1202 ||
			 (*long_options[long_index].flag) == 1203 ||
			 (*long_options[long_index].flag) == 1204 ||
			 (*long_options[long_index].flag) == 1205 ||
			 (*long_options[long_index].flag) == 1206 ||
			 (*long_options[long_index].flag) == 1207 ||
			 (*long_options[long_index].flag) == 1208 ||
			 (*long_options[long_index].flag) == 1209 ||
			 (*long_options[long_index].flag) == 1210 ||
			 (*long_options[long_index].flag) == 1211 ||
			 (*long_options[long_index].flag) == 1214)) {
		// make sure optind is valid  ... or another option
		if ((optind - 1) >= argc) {
			printf("\nOption '-%c' require an argument"
				"\n\n", optopt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind - 1], 0) &&
		    (opt == 0 && *long_options[long_index].flag <= 1200)) {
			printf("Option '--%s' require 1st argument as valid"
			       " numeric value\n\n", long_options[long_index].name);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if ((opt == 0 && (*long_options[long_index].flag == 46
		     || *long_options[long_index].flag == 48
		     || *long_options[long_index].flag == 53
		     || *long_options[long_index].flag == 64
		     || *long_options[long_index].flag == 104
		     ||	*long_options[long_index].flag == 1214))
		     && validate_number(argv[optind - 1], 16)) {
			printf("Option  '-%c' requires argument as valid"
			       " hex value\n\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		} else if (opt == 0
			   && (*long_options[long_index].flag == 1201
			   || *long_options[long_index].flag == 1202
			   || *long_options[long_index].flag == 1203
			   || *long_options[long_index].flag == 1204)) {
			strtof(argv[optind - 1], &end);
			if (*end != '\0') {
				printf("\nOption '-%c' require argument as valid"
				       " decimal value\n\n", opt);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
		} else {
			if (opt != 'O' && opt != 'E' && opt != 'S'
			    && opt != 'T' && opt !='P'
			    && (opt == 0 && (*long_options[long_index].flag) != 46
			    && (*long_options[long_index].flag) != 48
			    && (*long_options[long_index].flag) != 53
			    && (*long_options[long_index].flag) != 64
			    && (*long_options[long_index].flag) != 104
			    && (*long_options[long_index].flag) != 1214)
			    && validate_number(argv[optind - 1], 10)) {
				printf("\nOption '-%c' require argument as valid"
				       " numeric value\n\n", opt);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
		}
	}

	if (opt == 'd' ||
	    opt == 'D' ||
	    opt == 'B' ||
	    opt == 'H' ||
	    opt == 'G' ||
	    opt == 'L' ||
	    opt == 'V' ||
	    opt == 'e' ||
           (opt == 0 && (*long_options[long_index].flag == 15 ||
	    *long_options[long_index].flag == 35 ||
	    *long_options[long_index].flag == 47 ||
	    *long_options[long_index].flag == 49 ||
	    *long_options[long_index].flag == 58 ||
		*long_options[long_index].flag == 78 ||
	    *long_options[long_index].flag == 84 ||
	    *long_options[long_index].flag == 91 ||
	    *long_options[long_index].flag == 406))) {
	       if (optind >= argc || *argv[optind] == '-') {
			printf("\nOption '%s' require TWO arguments\n", argv[optind - 2]);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}

	       if (opt == 0 && (*long_options[long_index].flag == 47 ||
		   *long_options[long_index].flag == 49)
		   && validate_number(argv[optind - 1], 16)) {
		       printf("Option  '-%c' requires 1st argument as valid"
			      " hex value\n\n", opt);
		       show_usage(argv[0]);
		       return OOB_SUCCESS;
		}

	       if ((opt == 'V' || (opt ==  0 && (*long_options[long_index].flag == 78))
				|| (opt ==  0 && (*long_options[long_index].flag == 84
				|| *long_options[long_index].flag == 91
				|| *long_options[long_index].flag == 406)))
			    && validate_number(argv[optind - 1], 10)){
			printf("Option '%s' require 1st argument as valid"
			       " numeric value\n\n", argv[optind - 2]);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}

		if ((validate_number(argv[optind], 10) && opt != 'B'
		     && opt != 'e' && opt != 'G')) {
			printf("Option '-%c' require 2nd argument as valid"
			       " numeric value\n\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (opt ==  0 && (*long_options[long_index].flag == 91
				  || *long_options[long_index].flag == 406)
                    && validate_number(argv[optind], 10)) {
                        printf("Option '%s' require 2nd argument as valid"
                               " numeric value\n\n", argv[optind - 2]);
                        show_usage(argv[0]);
                        return OOB_SUCCESS;
                }
	}

	if ((opt == 0 && *(long_options[long_index].flag) == 20)) {
		if (optind >= argc || *argv[optind] == '-') {
			printf("\nOption '-%c' require TWO arguments\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
	}

	if (opt == 'N' || (opt == 0 && *(long_options[long_index].flag) == 14)
		       || (opt == 0 && (*long_options[long_index].flag == 92 ||
					 *long_options[long_index].flag == 77))) {
		if ((optind + 1) >= argc || *argv[optind] == '-'
		     || *argv[optind + 1] == '-') {
			printf("\nOption '-%s' requires 3 arguments\n", long_options[long_index].name);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}

		if (opt == 'N' || (opt == 0 && *(long_options[long_index].flag) == 92)) {
			if (validate_number(argv[optind - 1], 10)) {
				printf("Option '-%s' requires 1st argument as valid"
				       " numeric value\n\n", long_options[long_index].name);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
			if (validate_number(argv[optind], 10)) {
				printf("Option '-%s' requires 2nd argument as valid"
				       " numeric value\n\n", long_options[long_index].name);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
		} else {
			if (validate_number(argv[optind], 16)) {
				printf("Option  '-%c' requires 2nd argument as valid"
				       " hex value\n\n", opt);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
		}
		if (opt == 'N' || (opt == 0 && *(long_options[long_index].flag) == 14)) {
			if (validate_number(argv[optind + 1], 10)) {
				printf("Option '-%c' requires 3rd argument as valid"
					" numeric value\n\n", opt);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
		}
		if (opt == 0 && *(long_options[long_index].flag) == 92) {
			if (validate_number(argv[optind + 1], 16)) {
				printf("Option '-%s' requires 3rd argument as valid"
					" hex value\n\n", long_options[long_index].name);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
		}
	}

	if (opt == 0 && (*long_options[long_index].flag == 16
			 || *long_options[long_index].flag == 32
			 || *long_options[long_index].flag == 36
			 || *long_options[long_index].flag == 37
			 || *long_options[long_index].flag == 42)) {
		if ((optind + 1) >= argc || *argv[optind] == '-'
		     || *argv[optind + 1] == '-') {
			printf("\nOption '-%c' requires 3 arguments\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (*long_options[long_index].flag != 16) {
			if (validate_number(argv[optind - 1], 10)) {
				printf("Option '-%c' requires 1st argument as "
				       "valid numeric value\n\n", opt);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
			if (validate_number(argv[optind], 10)) {
				printf("Option '-%c' requires 2nd argument as "
				       "valid numeric value\n\n", opt);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
		}
		if (validate_number(argv[optind + 1], 10)) {
			printf("Option '-%c' requires 3rd argument as valid"
				" numeric value\n\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
	}

	if (opt == 0 && *long_options[long_index].flag == 89) {
			if (validate_number(argv[optind - 1], 10)) {
				printf("Option '-%c' requires 1st argument as "
				       "valid numeric value\n\n", opt);
				show_usage(argv[0]);
				return OOB_SUCCESS;
			}
			if (atoi(argv[optind - 1]) == 4 || atoi(argv[optind - 1]) == 5) {
				if ((optind + 1) >= argc || *argv[optind] == '-'
						|| *argv[optind + 1] == '-') {
					printf("\nOption '-%c' requires 3 arguments\n", opt);
					show_usage(argv[0]);
					return OOB_SUCCESS;
				}
				if (validate_number(argv[optind], 10)) {
					printf("Option '-%c' requires 2nd argument as "
						"valid numeric value\n\n", opt);
					show_usage(argv[0]);
					return OOB_SUCCESS;
				}
				if (validate_number(argv[optind + 1], 10)) {
					printf("Option '-%c' requires 3rd argument as valid"
						" numeric value\n\n", opt);
					show_usage(argv[0]);
					return OOB_SUCCESS;
				}
			}
	}
	if (opt == 'P') {
		if ((optind + 1) >= argc || *argv[optind] == '-'
		     || *argv[optind + 1] == '-') {
			printf("\nOption '-%c' requires 3 arguments\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind], 10)
		    || validate_number(argv[optind + 1], 10)) {
			printf("Option '-%c' requires 2nd & 3rd argument"
			       " as valid numeric value\n\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
	}

	if (opt == 'T') {
		if ((optind + 1) >= argc
		     || *argv[optind + 1] == '-') {
			printf("\nOption '-%c' requires 3 arguments\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind + 1], 10)) {
			printf("Option '-%c' requires 2nd & 3rd argument"
			       " as valid numeric value\n\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
	}

	if (opt == 0 && (*long_options[long_index].flag == 54
		|| *long_options[long_index].flag == 101)) {
		if ((optind + 2) >= argc || *argv[optind] == '-'
		     || *argv[optind + 1] == '-' || *argv[optind + 2] == '-') {
			printf("\nOption '-%c' requires 4 arguments\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind - 1], 16) ||
		    validate_number(argv[optind], 16) ||
		    validate_number(argv[optind + 1], 16) ||
		    validate_number(argv[optind + 2], 16)) {
			printf("\nOption '%c' requires all arguments as "
			       "valid hex value\n", opt);
			return OOB_SUCCESS;
		}

	}

        if (opt == 0 && (*long_options[long_index].flag == 71
			|| *long_options[long_index].flag == 102)) {
                if ((optind + 3) >= argc || *argv[optind] == '-'
                     || *argv[optind + 1] == '-' || *argv[optind + 2] == '-'
                     || *argv[optind + 3] == '-') {
                        printf("\nOption '-%c' requires 5 arguments\n", opt);
                        show_usage(argv[0]);
                        return OOB_SUCCESS;
                }
                if (validate_number(argv[optind - 1], 16) ||
                    validate_number(argv[optind], 16) ||
                    validate_number(argv[optind + 1], 16) ||
                    validate_number(argv[optind + 2], 16) ||
                    validate_number(argv[optind + 3], 16))
                {
                        printf("\nOption '%c' requires all arguments as "
                               "valid hexa value\n", opt);
                        return OOB_SUCCESS;
                }

        }
	if (opt == 0 && (*long_options[long_index].flag == 405)) {
		if ((optind + 3) >= argc) {
			printf("Option '--%s' require 5"
			       " arguments\n",
			       long_options[long_index].name);
			return OOB_SUCCESS;
		}

		if (validate_number(argv[optind - 1], 0)) {
			printf("Option '--%s' require argument as"
			       "valid hex value\n\n",
			       long_options[long_index].name);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind], 10)
		    || validate_number(argv[optind + 1], 10)
		    || validate_number(argv[optind + 2], 10)
		    || validate_number(argv[optind + 3], 10)) {
			printf("Option '--%s' require argument as"
			       " valid numeric value\n",
			       long_options[long_index].name);
			return OOB_SUCCESS;
		}
	}

	if (opt == 'R') {
		if ((optind + 3) >= argc || *argv[optind] == '-'
		     || *argv[optind + 1] == '-' || *argv[optind + 2] == '-'
		     || *argv[optind] == '-' || *argv[optind + 3] == '-') {
			printf("\nOption '-%c' requires 5 arguments\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind], 10)
		    || validate_number(argv[optind + 3], 10)) {
			printf("Option '-%c' requires 2nd 5th argument"
				" as valid numeric value\n\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
	}

	if (opt == 0 && *long_options[long_index].flag == 57) {
		if ((optind + 4) >= argc || *argv[optind] == '-'
		     || *argv[optind + 1] == '-' || *argv[optind + 2] == '-'
		     || *argv[optind + 3] == '-' || *argv[optind + 4] == '-') {
			printf("\nOption '-%c' requires 6 arguments\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind], 10)) {
			printf("Option '-%c' requires 2nd argument  "
				" as valid numeric value\n\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind + 1], 16)
		    || validate_number(argv[optind + 2], 16)
		    || validate_number(argv[optind + 3], 16)
		    || validate_number(argv[optind + 4], 16)) {
			printf("Option '-%c' requires 3rd,4th, 5th and 6th "
				"arguments as valid hexa value\n", opt);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
	}

	if (opt == 0 && *long_options[long_index].flag == 38) {
		if ((optind + 3) >= argc || *argv[optind] == '-'
		     || *argv[optind + 1] == '-' || *argv[optind + 2] == '-'
		     || *argv[optind + 3] == '-') {
			printf("\nOption '--%s' requires 5 arguments\n",
			       long_options[long_index].name);
			show_usage(argv[0]);
			return OOB_SUCCESS;
		}
		if (validate_number(argv[optind - 1], 10) ||
		    validate_number(argv[optind], 10) ||
		    validate_number(argv[optind + 1], 10) ||
		    validate_number(argv[optind + 2], 10) ||
		    validate_number(argv[optind + 3], 10)) {
			printf("\nOption '%s' requires all arguments as "
			       "valid numeric value\n", argv[optind - 1]);
			return OOB_SUCCESS;
		}
	}
	switch (opt) {
	case 0:
		if (*(long_options[long_index].flag) == 1)
			get_apml_rmi_access(soc_die_num);
		else if (*(long_options[long_index].flag) == 1200) {
			ret = get_apml_tsi_access(soc_die_num);
			if (ret)
				return ret;
		} else if (*(long_options[long_index].flag) == 3)
			apml_get_ddr_bandwidth(soc_die_num);
		else if (*(long_options[long_index].flag) == 4)
			/* Request reset on sync flood */
			apml_reset_on_sync_flood(soc_die_num);
		else if (*(long_options[long_index].flag) == 5)
			/* get number of mca banks with valid */
			/* status after a fatal error */
			apml_get_ras_valid_mca_banks(soc_die_num);
		else if (*(long_options[long_index].flag) == 6)
			/* Get current active freq limit per socket */
			apml_get_freq_limit(soc_die_num);
		else if (*(long_options[long_index].flag) == 7)
			/* get svi based power telemetry for all rails */
			apml_get_pwr_telemetry(soc_die_num);
		else if (*(long_options[long_index].flag) == 8)
			/* Get Fmax and Fmin for socket */
			apml_get_sock_freq_range(soc_die_num);
		else if (*(long_options[long_index].flag) == 9)
			/* enable df pstate dynamic */
			apml_apb_enable(soc_die_num);
		else if (*(long_options[long_index].flag) == 10)
			/* ger current df pstate frequency */
			apml_get_fclkmclkuclk(soc_die_num);
		else if (*(long_options[long_index].flag) == 11)
			/* get base freq of socket */
			apml_get_cpu_base_freq(soc_die_num);
		else if (*(long_options[long_index].flag) == 12)
			/* get package energy */
			apml_get_pkg_energy(soc_die_num);
		else if (*(long_options[long_index].flag) == 14) {
			/* Write to sbrmi/sbtsi register */
			val = argv[optind - 1];
			val1 = strtoul(argv[optind++], &end, 16);
			val2 = atoi(argv[optind++]);
			write_register(soc_die_num, val1, val, val2);
		} else if (*(long_options[long_index].flag) == 15) {
			/* Read MSR register */
			val1 = strtoul(argv[optind - 1], &end, 16);
			val2 = atoi(argv[optind++]);
			read_msr_register(soc_die_num, val1, val2);
		} else if (*(long_options[long_index].flag) == 16) {
			/* Read CPUID register */
			/* CPUID Function register */
			val1 = strtoul(argv[optind - 1], &end, 16);
			/* CPUID Extended Function register */
			val2 = strtoul(argv[optind++], &end, 16);
			/* Thread Id */
			val3 = atoi(argv[optind++]);
			read_cpuid_register(soc_die_num, val1, val2,
					    val3);
		} else if (*(long_options[long_index].flag) == 17) {
			/* Read IOD Bist result */
			apml_get_iod_bist_status(soc_die_num);
		} else if (*(long_options[long_index].flag) == 18) {
			/* Read CCD Bist result */
			val1 = atoi(argv[optind - 1]);
			apml_get_ccd_bist_status(soc_die_num, val1);
		} else if (*(long_options[long_index].flag) == 19) {
			/* Read CCX Bist result */
			val1 = atoi(argv[optind - 1]);
			apml_get_ccx_bist_status(soc_die_num,
						 val1);
		} else if (*(long_options[long_index].flag) == 20) {
			/* Read NBIO error logging register */
			/* nbio quadrant */
			nbio.quadrant = strtoul(argv[optind - 1], &end, 16);
			/* register offset */
			nbio.offset = strtoul(argv[optind++], &end, 16);
			apml_get_nbio_error_log_reg(soc_die_num, nbio);
		} else if (*(long_options[long_index].flag) == 21) {
			/* Read DRAM Throttle */
			apml_get_dram_throttle(soc_die_num);
		} else if (*(long_options[long_index].flag) == 22) {
			/* Read Prochot status */
			apml_get_prochot_status(soc_die_num);
		} else if (*(long_options[long_index].flag) == 23) {
			/* Read Prochot Residency */
			apml_get_prochot_residency(soc_die_num);
		} else if (*(long_options[long_index].flag) == 25) {
			/* Read LCLK DPM Level range */
			val1 = atoi(argv[optind - 1]);
			apml_get_lclk_dpm_level_range(soc_die_num, val1);
		} else if (*(long_options[long_index].flag) == 26) {
			/* Read ucode revision */
			apml_get_ucode_rev(soc_die_num);
		} else if (*(long_options[long_index].flag) == 27) {
			/* Read power consumed for a socket */
			apml_get_power_consumed(soc_die_num);
		} else if (*(long_options[long_index].flag) == 28) {
			/* Read SMT enabled status */
			apml_get_smt_status(soc_die_num);
		} else if (*(long_options[long_index].flag) == 29) {
			/* Show threads per core and threads per socket */
			apml_get_threads_per_core_and_soc(soc_die_num);
		} else if (*(long_options[long_index].flag) == 30) {
			/* Show maximum number of cores per ccx
			 * and logical ccx instance numbers
			 */
			apml_get_ccx_info(soc_die_num);
		} else if (*(long_options[long_index].flag) == 31) {
			/* APML client reoovery */
			val1 = atoi(argv[optind - 1]);
			apml_do_recovery(soc_die_num, val1);
		} else if (*(long_options[long_index].flag) == 32) {
			/* override delay reset cpu on sync flood */
			/* override delay value */
			d_in.delay_val_override = atoi(argv[optind - 1]);
			/* disable delay counter bit */
			d_in.disable_delay_counter = atoi(argv[optind++]);
			/* stop delay counter bit */
			d_in.stop_delay_counter = atoi(argv[optind++]);
			apml_override_delay_reset_on_sync_flood(soc_die_num, d_in);
		} else if (*(long_options[long_index].flag) == 33) {
			/* Get Post code for 8 offsets */
			apml_get_post_code(soc_die_num, argv[optind - 1]);
		} else if (*(long_options[long_index].flag) == 34) {
			/* RAS Status register value */
			val1 = atoi(argv[optind - 1]);
			/* Set RAS Status Register */
			apml_clear_ras_status_register(soc_die_num, val1);
		} else if (*(long_options[long_index].flag) == 35) {
			/* RAS runtime error validity check */
			err_category.err_type = atoi(argv[optind - 1]);
			err_category.req_type = atoi(argv[optind++]);
			apml_get_bmc_ras_rt_err_validity_check(soc_die_num, err_category);
		} else if (*(long_options[long_index].flag) == 36) {
			/* RAS runtime error info */
			/* offset */
			err_d_in.offset = atoi(argv[optind - 1]);
			/* Category */
			err_d_in.category = atoi(argv[optind++]);
			/* Valid instance index */
			err_d_in.valid_inst_index = atoi(argv[optind++]);
			apml_get_ras_runtime_err_info(soc_die_num, err_d_in);
		} else if (*(long_options[long_index].flag) == 37) {
			/* Error/category type */
			th.err_type = atoi(argv[optind - 1]);
			/* Error count threshold */
			th.err_count_th = atoi(argv[optind++]);
			/* Max interrupt rate */
			th.max_intrupt_rate = atoi(argv[optind++]);
			/* RAS runtime error threshold */
			apml_set_ras_err_threshold(soc_die_num, th);
		} else if (*(long_options[long_index].flag) == 38) {
			/* RAS set oob config */
			/* MCA OOB MISC0 Error Counter Enable */
			oob_config.mca_oob_misc0_ec_enable = atoi(argv[optind - 1]);
			/* DRAM CECC OOB Error Counter Mode */
			oob_config.dram_cecc_oob_ec_mode = atoi(argv[optind++]);
			/* PCIe OOB Error Reporting Enable */
			oob_config.dram_cecc_leak_rate = atoi(argv[optind++]);
			/* PCIe OOB Error Reporting Enable */
			oob_config.pcie_err_reporting_en = atoi(argv[optind++]);
			/* MCA OOB Error Reporting Enable */
			oob_config.core_mca_err_reporting_en = atoi(argv[optind++]);
			apml_set_ras_oob_config(soc_die_num, oob_config);
		} else if (*(long_options[long_index].flag) == 39) {
			/* RAS GET OOB Configuration */
			apml_get_ras_oob_config(soc_die_num);
		} else if (*(long_options[long_index].flag) == 40) {
			/* show PPIN Fuse data */
			apml_get_ppin_fuse(soc_die_num);
		} else if (*(long_options[long_index].flag) == 41) {
			val1 = atoi(argv[optind - 1]);
			apml_get_ras_df_validity_chk(soc_die_num, val1);
		} else if (*(long_options[long_index].flag) == 42) {
			/* Offset */
			df_err.input[0] = atoi(argv[optind - 1]);
			/* DF block ID */
			df_err.input[1] = atoi(argv[optind++]);
			/* DF block ID instance */
			df_err.input[2] = atoi(argv[optind++]);
			apml_get_ras_df_err_dump(soc_die_num, df_err);
		} else if (*(long_options[long_index].flag) == 43) {
			/* show cclk frequency limit */
			apml_get_cclk_freqlimit(soc_die_num);
		} else if (*(long_options[long_index].flag) == 44) {
			/* show C0 residency */
			apml_get_sockc0_residency(soc_die_num);
		} else if (*(long_options[long_index].flag) == 46) {
			/* Read TSI register */
			val1 = strtoul(argv[optind - 1], &end, 16);
			read_tsi_register(soc_die_num, val1);
		} else if (*(long_options[long_index].flag) == 47) {
			/* Write TSI register */
			val1 = strtoul(argv[optind - 1], &end, 16);
			val2 = atoi(argv[optind++]);
			write_tsi_register(soc_die_num, val1, val2);
		} else if (*(long_options[long_index].flag) == 96) {
			/* Get SOC+DIMM combined power limit */
			apml_get_soc_dimm_power_limit(soc_die_num);
		} else if (*(long_options[long_index].flag) == 97) {
			/* Set SOC+DIMM combined power limit */
			power = atoi(argv[optind - 1]);
			apml_set_soc_dimm_power_limit(soc_die_num, power);
		} else if (*(long_options[long_index].flag) == 98) {
			/* Get highest DIMM  Power Consumption */
			dimm_din.dimm_addr = 0;
			dimm_din.pow_reporting_flag = HI_POW_DIMM_REPORT_FLAG;
			apml_get_per_dimm_power(soc_die_num, dimm_din);
		} else if (*(long_options[long_index].flag) == 99) {
			/* Get hottest DIMM thermal sensor */
			apml_get_hottest_dimm_temp(soc_die_num);
		} else if (*(long_options[long_index].flag) == 100) {
			/* Get all DIMM Power Consumption */
			dimm_din.dimm_addr = 0;
			dimm_din.pow_reporting_flag = ALL_DIMM_REPORTING_FLAG;
			apml_get_per_dimm_power(soc_die_num, dimm_din);
		} else if (*(long_options[long_index].flag) == 48) {
			/* read rmi register */
			val1 = strtoul(argv[optind - 1], &end, 16);
			read_rmi_register(soc_die_num, val1);
		} else if (*(long_options[long_index].flag) == 49) {
			/* Write to sbrmi register */
			val1 = strtoul(argv[optind - 1], &end, 16);
			val2 = atoi(argv[optind++]);
			write_rmi_register(soc_die_num, val1, val2);
		} else if (*(long_options[long_index].flag) == 51) {
			/* show RTC data */
			apml_get_rtc(soc_die_num);
		} else if (*(long_options[long_index].flag) == 53) {
			/* Get DIMM Address */
			dimm_addr = strtoul(argv[optind - 1], &end, 16);
			apml_get_dimm_serial_num(soc_die_num, dimm_addr);
		} else if (*(long_options[long_index].flag) == 54) {
			/* Get DIMM Address */
			spd_in.dimm_addr = strtoul(argv[optind - 1], &end, 16);
			spd_in.lid = strtoul(argv[optind++], &end, 16);
			spd_in.reg_offset = strtoul(argv[optind++], &end, 16);
			spd_in.reg_space = strtoul(argv[optind++], &end, 16);
			apml_get_spd_sb_data(soc_die_num, spd_in);
		} else if (*(long_options[long_index].flag) == 56) {
			apml_get_smu_fw_version(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 57) {
			/* Write data to PCIE config space */
			pci_addr.segment = atoi(argv[optind - 1]);
			pci_addr.offset = atoi(argv[optind++]);
			pci_addr.bus = strtoul(argv[optind++], &end, 16);
			pci_addr.device = strtoul(argv[optind++], &end, 16);
			pci_addr.func = strtoul(argv[optind++], &end, 16);
			value = strtoul(argv[optind++], &end, 16);
			apml_set_bmc_pcie_config(soc_die_num, pci_addr, value);
			break;
		} else if (*(long_options[long_index].flag) == 58) {
			/* Set XGMI pstate range */
			val1 = atoi(argv[optind - 1]);
			val2 = atoi(argv[optind++]);
			apml_set_xgmi_pstate_range(soc_die_num, val1, val2);
			break;
		} else if (*(long_options[long_index].flag) == 59) {
			/* APML set CPU rail iso frequency  policy */
			val1 = atoi(argv[optind - 1]);
			apml_set_cpu_rail_iso_freq_policy(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 60) {
			/* APML get cpu rail iso frequency policy */
			apml_get_cpu_rail_iso_freq_policy(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 61) {
			/* APML set DF C-State  */
			val1 = atoi(argv[optind - 1]);
			apml_set_dfc_enable(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 62) {
			/* APML set CPU rail iso frequency  policy */
			apml_get_dfc_enable(soc_die_num);
			break;
                } else if (*(long_options[long_index].flag) == 63) {
                        /* APML get average dram throttle for all channels */
                        apml_get_avg_dram_throttle(soc_die_num);
                        break;
                } else if (*(long_options[long_index].flag) == 64) {
                        /* APML get dram throttle for the given channel */
			dimm_addr = strtoul(argv[optind - 1], &end, 16);
                        apml_get_ch_dram_throttle(soc_die_num, dimm_addr);
                        break;
                } else if (*(long_options[long_index].flag) == 65) {
                        /* APML get CC6 control */
                        apml_get_cc6_enable(soc_die_num);
                        break;
                } else if (*(long_options[long_index].flag) == 66) {
                        /* APML set CC6 control */
                        val1 = atoi(argv[optind - 1]);
                        apml_set_cc6_enable(soc_die_num, val1);
                        break;
                } else if (*(long_options[long_index].flag) == 67) {
                        /* APML get xgmi link width range */
                        apml_get_xgmi_link_width_range(soc_die_num);
                        break;
                } else if (*(long_options[long_index].flag) == 68) {
                        /* APML get apb state */
                        apml_get_apb_state(soc_die_num);
                        break;
                } else if (*(long_options[long_index].flag) == 69) {
                        /* APML get df p-state range */
                        apml_get_df_pstate_range(soc_die_num);
                        break;
                } else if (*(long_options[long_index].flag) == 70) {
                        /* APML get xgmi pstate range */
                        apml_get_xgmi_pstate_range(soc_die_num);
                        break;
                } else if (*(long_options[long_index].flag) == 71) {
                        /* APML set SPD data */
                        spd_w_din.dimm_addr = strtoul(argv[optind - 1], &end, 16);
                        spd_w_din.lid = strtoul(argv[optind++], &end, 16);
                        spd_w_din.reg_offset = strtoul(argv[optind++], &end, 16);
                        spd_w_din.reg_space = atoi(argv[optind++]);
                        spd_w_din.w_data = strtoul(argv[optind++], &end, 16);
                        apml_set_dimm_spd_register(soc_die_num, spd_w_din);
                        break;
                } else if (*(long_options[long_index].flag) == 72) {
                        /* APML get pc6 control */
                        apml_get_pc6_enable(soc_die_num);
                        break;
                } else if (*(long_options[long_index].flag) == 73) {
                        /* APML set PC6 control */
                        val1 = atoi(argv[optind - 1]);
                        apml_set_pc6_enable(soc_die_num, val1);
                        break;
		} else if (*(long_options[long_index].flag) == 75) {
                        /* Get CCD power consumption */
                        val1 = atoi(argv[optind - 1]);
                        apml_get_ccd_power_consumption(soc_die_num, val1);
                        break;
		} else if (*(long_options[long_index].flag) == 76) {
                        /* Get Temperature Delta */
                        apml_get_tdelta(soc_die_num);
                        break;
		} else if (*(long_options[long_index].flag) == 77) {
                        /* Link ID */
			link_name = argv[optind - 1];
			val1 = strtoul(argv[optind++], &end, 0);
			val2 = atoi(argv[optind++]);
			apml_set_pcie_link_training(soc_die_num, link_name, val1, val2);
                        break;
		} else if (*(long_options[long_index].flag) == 78) {
			/* Get SVI3 vr controller temperature by rail */
			input_val1 = strtol(argv[optind - 1], &end, 10);
			input_val2 = strtol(argv[optind++], &end, 10);
			/* Validating SVI3 Rail mode */
			if (validate_bitfield_range(input_val1, SVI3_RAIL_MODE_BITS,
						     "SVI3 Rail Mode") != OOB_SUCCESS)
				break;

			/* Validating SVI3 Rail index */
			if (validate_bitfield_range(input_val2, SVI3_RAIL_INDEX_BITS,
						     "SVI3 Rail Index") != OOB_SUCCESS)
				break;
			svi3_d_in.rail_mode = (uint8_t)input_val1;
			svi3_d_in.svi3_rail_index = (uint8_t)input_val2;
			apml_get_svi3_vr_controller_temp_by_rail(soc_die_num, svi3_d_in);
                        break;
		} else if (*(long_options[long_index].flag) == 79) {
			/* Get supporting error types */
			apml_get_supporting_error_types(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 80) {
			/* Get BMC RAS MCA Runtime error info */
			apml_get_bmc_ras_run_time_error_info_all_offsets(soc_die_num, MCA_ERR_CATEGORY);
			break;
		} else if (*(long_options[long_index].flag) == 81) {
			/* Get BMC RAS DRAM ECC Runtime error info */
			apml_get_bmc_ras_run_time_error_info_all_offsets(soc_die_num, DRAM_ECC_ERR_CATEGORY);
			break;
		} else if (*(long_options[long_index].flag) == 82) {
			/* Get BMC RAS PCIE Runtime error info */
			apml_get_bmc_ras_run_time_error_info_all_offsets(soc_die_num, PCIE_ERR_CATEGORY);
			break;
		} else if (*(long_options[long_index].flag) == 83) {
			/* Get core apml floor limit */
			val1 = atoi(argv[optind - 1]);
			apml_get_apml_floor_core_limit(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 84) {
			/* Set core apml floor limit*/
			d_in_core_limit.core_id = atoi(argv[optind - 1]);
			d_in_core_limit.floor_limit = atoi(argv[optind++]);
			apml_set_apml_floor_core_limit(soc_die_num, d_in_core_limit);
                        break;
		} else if (*(long_options[long_index].flag) == 85) {
			/* Set apml floor limit for all cores*/
			val1 = atoi(argv[optind - 1]);
			apml_set_apml_floor_limit_for_all_cores(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 86) {
			/* clear_sw_async_alert_status */
			apml_clear_sw_async_alert_status(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 87) {
			/* clear MP0 alert status */
			apml_clear_mp0_alert_status(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 88) {
			/* Set sw async alert mask */
			sw_async_alert_mask = atoi(argv[optind - 1]);
			apml_set_sw_async_alert_mask(soc_die_num, sw_async_alert_mask);
			break;
		} else if (*(long_options[long_index].flag) == 89) {
			if (atoi(argv[optind - 1]) > MAX_PERF_MODE) {
				printf("Err[%d]:%s\n", OOB_INVALID_INPUT,
					esmi_get_err_msg(OOB_INVALID_INPUT));
				break;
			}
			din_mode.mode = atoi(argv[optind - 1]);
			if (din_mode.mode == 4 || din_mode.mode == 5) {
				din_mode.utilization_point = atoi(argv[optind++]);
				din_mode.ppt_limit = atoi(argv[optind++]);
			} else {
				din_mode.utilization_point = 0;
				din_mode.ppt_limit = 0;
			}
			apml_set_power_efficiency_mode_selection(soc_die_num, din_mode);
			break;
		} else if (*(long_options[long_index].flag) == 90) {
			apml_get_power_efficiency_mode_selection(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 93) {
			apml_get_sbrmi_ready_status(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 94) {
			val1 = atoi(argv[optind - 1]);
			apml_get_effective_floor_freq_per_core(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 95) {
			apml_clear_shut_down_err(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 91) {
			/* Get Enabled HSMP commands */
			input_val1 = strtol(argv[optind - 1], &end, 10);
			input_val2 = strtol(argv[optind++], &end, 10);

			/* Validate rmask (1-bit field: 0 or 1 only) */
			if (validate_bitfield_range(input_val1,
						     HSMP_CMDS_MASK_BITS,
						     "HSMP Commands Mask") != OOB_SUCCESS)
				break;

			/* Validate offset (4-bit field: 0-15) */
			if (validate_bitfield_range(input_val2,
						     HSMP_CMDS_OFFSET_BITS,
						     "HSMP Commands Offset") != OOB_SUCCESS)
				break;

			get_hsmp_cmds_d_in.rmask = (uint8_t)input_val1;
			get_hsmp_cmds_d_in.offset = (uint8_t)input_val2;
			apml_get_enabled_hsmp_commands(soc_die_num, get_hsmp_cmds_d_in);
			break;
		} else if (*(long_options[long_index].flag) == 92) {
			/* Set Enabled HSMP commands */
			input_val1 = strtol(argv[optind - 1], &end, 10);
			input_val2 = strtol(argv[optind++], &end, 10);
			input_val3 = strtoul(argv[optind++], &end, 16);

			/* Validate rmask (1-bit field: 0 or 1 only) */
			if (validate_bitfield_range(input_val1,
						     HSMP_CMDS_MASK_BITS,
						     "HSMP Commands Mask") != OOB_SUCCESS)
				break;

			/* Validate offset (4-bit field: 0-15) */
			if (validate_bitfield_range(input_val2,
						     HSMP_CMDS_OFFSET_BITS,
						     "HSMP Commands Offset") != OOB_SUCCESS)
				break;

			/* Validate bitmask (24-bit field: 0-0xFFFFFF) */
			if (validate_bitfield_range(input_val3,
						     BIT_MASK_SEGMENT_BITS,
						     "HSMP Commands BitMask") != OOB_SUCCESS)
				break;

			set_hsmp_cmds_d_in.rmask = (uint8_t)input_val1;
			set_hsmp_cmds_d_in.offset = (uint8_t)input_val2;
			set_hsmp_cmds_d_in.bitmask = (uint32_t)input_val3;
			apml_set_enabled_hsmp_commands(soc_die_num, set_hsmp_cmds_d_in);
			break;
		} else if (*(long_options[long_index].flag) == 101) {
			/* Get DIMM sb register data */
			dimm_reg_din.dimm_addr = strtoul(argv[optind - 1], &end, 16);
			dimm_reg_din.lid = strtoul(argv[optind++], &end, 16);
			dimm_reg_din.reg_offset = strtoul(argv[optind++], &end, 16);
			dimm_reg_din.reg_space = strtoul(argv[optind++], &end, 16);
			apml_get_dimm_sb_data(soc_die_num, dimm_reg_din);
		} else if (*(long_options[long_index].flag) == 102) {
			/* APML set dimm sb register data */
			dimm_reg_write.dimm_addr = strtoul(argv[optind - 1], &end, 16);
			dimm_reg_write.lid = strtoul(argv[optind++], &end, 16);
			dimm_reg_write.reg_offset = strtoul(argv[optind++], &end, 16);
			dimm_reg_write.reg_space = atoi(argv[optind++]);
			dimm_reg_write.w_data = strtoul(argv[optind++], &end, 16);
			apml_set_dimm_register_data(soc_die_num, dimm_reg_write);
		} else if (*(long_options[long_index].flag) == 103) {
			/* Get average DRAM throttle information with ODTS/TSOD status */
			apml_get_avg_dram_thr_with_status(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 104) {
			/* Get DRAM throttle information with ODTS/TSOD status */
			dimm_addr = strtoul(argv[optind - 1], &end, 16);
			apml_get_dram_thr_with_status(soc_die_num, dimm_addr);
			break;
		} else if (*(long_options[long_index].flag) == 105) {
			/* Get hottest temperature range, refresh rate and dimm address */
			apml_get_hottest_dimm_temp_range_ref_rate(soc_die_num);
			break;
		} else if (*(long_options[long_index].flag) == 106) {
			/* Enable/disable PCIe link control */
			val1 = atoi(argv[optind - 1]);
			apml_write_pcie_link_control(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 107) {
			/* gpio assertion on async alerts */
			input_val1 = strtol(argv[optind - 1], &end, 10);
			/* Validate CTRL_ALERTL (1-bit field: 0 or 1 only) */
			if (validate_bitfield_range(input_val1, BIT_LEN,
						    "CTRL_ALERTL") != OOB_SUCCESS)
				break;
			val1 = input_val1;
			apml_gpio_assertion_for_async_alerts(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 108) {
			/* gpio assertion on mailbox */
			input_val1 = strtol(argv[optind - 1], &end, 10);
			/* Validate CTRL_ALERTL (1-bit field: 0 or 1 only) */
			if (validate_bitfield_range(input_val1, BIT_LEN,
						    "CTRL_ALERTL") != OOB_SUCCESS)
				break;
			val1 = input_val1;
			apml_gpio_assertion_on_mailbox(soc_die_num, val1);
			break;
		} else if (*(long_options[long_index].flag) == 405) {
			/* Set bmc ras action */
			/* pay load */
			set_ras_act_din.payload.pay_load = strtol(argv[optind - 1], &end, 0);
			/* offset */
			set_ras_act_din.payload.offset = atoi(argv[optind++]);
			/* repair entry number */
			set_ras_act_din.payload.repair_entry_num = atoi(argv[optind++]);
			/* ras action ID */
			set_ras_act_din.ras_act_id = atoi(argv[optind++]);
			/* EOM flag */
			set_ras_act_din.eom_flag = atoi(argv[optind++]);
			apml_set_ras_action(soc_die_num, set_ras_act_din);
		} else if (*(long_options[long_index].flag) == 406) {
			/* Get bmc ras action status */
			/* Repair entry number */
			get_ras_act_din.pay_load.repair_entry_num = atoi(argv[optind - 1]);
			/* ras action id */
			get_ras_act_din.ras_action_id = atoi(argv[optind++]);
			apml_get_ras_action_status(soc_die_num, get_ras_act_din);
		} else if (*(long_options[long_index].flag) == 1201) {
			uprate = atof(argv[optind - 1]);
			set_and_verify_apml_socket_uprate(soc_die_num, uprate);
			break;
		} else if (*(long_options[long_index].flag) == 1202) {
			temp = atof(argv[optind - 1]);
			set_high_temp_threshold(soc_die_num, temp);
			break;
		} else if (*(long_options[long_index].flag) == 1203) {
			temp = atof(argv[optind - 1]);
			set_low_temp_threshold(soc_die_num, temp);
			break;
		} else if (*(long_options[long_index].flag) == 1204) {
			temp = atof(argv[optind - 1]);
			set_temp_offset(soc_die_num, temp);
			break;
		} else if (*(long_options[long_index].flag) == 1205) {
			value = atoi(argv[optind - 1]);
			set_timeout_config(soc_die_num, value);
			break;
		} else if (*(long_options[long_index].flag) == 1206) {
			value = atoi(argv[optind - 1]);
			set_alert_threshold(soc_die_num, value);
			break;
		} else if (*(long_options[long_index].flag) == 1207) {
			value = atoi(argv[optind - 1]);
			set_alert_config(soc_die_num, value);
			break;
		} else if (*(long_options[long_index].flag) == 1208) {
			value = atoi(argv[optind - 1]);
			set_tsi_config(soc_die_num, value,
				       *long_options[long_index].flag);
			break;
		} else if (*(long_options[long_index].flag) == 1209) {
			value = atoi(argv[optind - 1]);
			set_tsi_config(soc_die_num, value,
				       *long_options[long_index].flag);
			break;
		} else if (*(long_options[long_index].flag) == 1210) {
			value = atoi(argv[optind - 1]);
			set_tsi_config(soc_die_num, value,
				       *long_options[long_index].flag);
			break;
		} else if (*(long_options[long_index].flag) == 1211) {
			value = atoi(argv[optind - 1]);
			set_tsi_config(soc_die_num, value,
				       *long_options[long_index].flag);
			break;
		} else if (*(long_options[long_index].flag) == 1214) {
			/* Read TSI register */
			val1 = strtoul(argv[optind - 1], &end, 16);
			read_tsi_raw_register(soc_die_num, val1);
			break;
		}
		break;
	case 'Y':
		/* Get the summary of mailbox commands for a given bus_num, */
		/* addr */
		show_apml_mailbox_cmds(soc_die_num);
		break;
	case 'p':
		/* Get the power metrics for a given bus_num, addr */
		apml_get_sockpower(soc_die_num);
		break;
	case 't':
		/* Get tdp value for a given soc_die_num */
		apml_get_socktdp(soc_die_num);
		break;
	case 's':
		power = atoi(argv[optind - 1]);
		apml_setpower_limit(soc_die_num, power);
		break;
	case 'b':
		/* Get apml boostlimit for a given soc_die_num */
		/* and thread index */
		thread_ind = atoi(argv[optind - 1]);
		get_boostlimit(soc_die_num, thread_ind);
		break;
	case 'd':
		thread_ind = atoi(argv[optind - 1]);
		boostlimit = atoi(argv[optind++]);
		set_apml_boostlimit(soc_die_num, thread_ind, boostlimit);
		break;
	case 'a':
		boostlimit = atoi(argv[optind - 1]);
		set_apml_socket_boostlimit(soc_die_num, boostlimit);
		break;
	case 'l':
		dram_thr = atoi(argv[optind - 1]);
		set_and_verify_dram_throttle(soc_die_num, dram_thr);
		break;
	case 'P':
		/* Write BMC reported dim power and update rate value to */
		/* given socket die index and dimm id */
		dp_soc_die_num.dimm_addr = strtoul(argv[optind - 1], &end, 16);
		dp_soc_die_num.power = atoi(argv[optind++]);
		dp_soc_die_num.update_rate = atoi(argv[optind++]);
		apml_set_dimm_power(soc_die_num, dp_soc_die_num);
		break;
	case 'T':
		/* Write BMC reported dimm temperature and update rate value */
		/* to given socket die index and dimm id */
		dt_soc_die_num.dimm_addr = strtoul(argv[optind - 1], &end, 16);
		temp = atof(argv[optind++]);
		//dt_soc_die_num.sensor = strtoul(argv[optind++], &end, 16);
		dt_soc_die_num.update_rate = atoi(argv[optind++]);
		apml_set_thermal_sensor(soc_die_num, dt_soc_die_num, temp);
		break;
	case 'R':
		/* Read 32 bit data from extended pci config space */
		pci_addr.segment = atoi(argv[optind - 1]);
		pci_addr.offset = atoi(argv[optind++]);
		pci_addr.bus = strtoul(argv[optind++], &end, 16);
		pci_addr.device = strtoul(argv[optind++], &end, 16);
		pci_addr.func = atoi(argv[optind++]);
		apml_get_ras_pcie_config_data(soc_die_num, pci_addr);
		break;
	case 'D':
		/* read 32 bit data from MCA bank reported by */
		/* validity check mesg */
		mca_dump.index = atoi(argv[optind - 1]);
		mca_dump.offset = atoi(argv[optind++]);
		apml_get_ras_mca_msr(soc_die_num, mca_dump);
		break;
	case 'F':
		/* Get FCH reset reson code */
		val1 = atoi(argv[optind - 1]);
		apml_get_fch_reset_reason(soc_die_num, val1);
		break;
	case 'S':
		/* Get per dimm temperature range and refresh rate */
		/* from MR4 register */
		dimm_addr = strtoul(argv[optind - 1], &end, 16);
		apml_get_temp_range_and_refresh_rate(soc_die_num, dimm_addr);
		break;
	case 'O':
		/* Get dimm power when BMC doesn't own SPD side band bus */
		dimm_addr = strtoul(argv[optind - 1], &end, 16);
		apml_get_dimm_power(soc_die_num, dimm_addr);
		break;
	case 'E':
		/* Get dimm temperature when BMC doesn't own SPD side */
		/* band bus */
		dimm_addr = strtoul(argv[optind - 1], &end, 16);
		apml_get_dimm_temp(soc_die_num, dimm_addr);
		break;
	case 'C':
		/* Get current active cclk limit */
		val1 = atoi(argv[optind - 1]);
		apml_get_cclklimit(soc_die_num, val1);
		break;
	case 'B':
		/* get current bandwidth on io link */
		link_name = argv[optind - 1];
		bw_type = argv[optind++];
		apml_get_iobandwidth(soc_die_num, link_name, bw_type);
		break;
	case 'G':
		/* get current bandwidth on xgmi link */
		link_name = argv[optind - 1];
		bw_type = argv[optind++];
		apml_get_xgmibandwidth(soc_die_num, link_name, bw_type);
		break;
	case 'H':
		/* set gmi3 link width */
		val1 = atoi(argv[optind - 1]);
		val2 = atoi(argv[optind++]);
		apml_set_gmi3link_width(soc_die_num, val1, val2);
		break;
	case 'L':
		/* set xgmi link width */
		val1 = atoi(argv[optind - 1]);
		val2 = atoi(argv[optind++]);
		apml_set_xgmilink_width(soc_die_num, val1,  val2);
		break;
	case 'M':
		/* disable dynamic pstate of df and set the user */
		/* specified pstate */
		val1 = atoi(argv[optind - 1]);
		apml_set_dfpstate(soc_die_num, val1);
		break;
	case 'N':
		/* set the max and min lclk dpm level on given nbio */
		lclk.nbio_id = atoi(argv[optind - 1]);
		lclk.dpm.max_dpm_level = atoi(argv[optind++]);
		lclk.dpm.min_dpm_level = atoi(argv[optind++]);
		apml_set_lclk_dpm_level(soc_die_num, lclk);
		break;
	case 'Z':
		/* Control pcie rate on gen5 capable devices */
		val1 = atoi(argv[optind - 1]);
		apml_set_pciegen5_control(soc_die_num, val1);
		break;
	case 'U':
		/* select power efficiency profile policy */
		val1 = atoi(argv[optind - 1]);
		apml_set_pwr_efficiency_mode(soc_die_num, val1);
		break;
	case 'J':
		/* get core energy */
		val1 = atoi(argv[optind - 1]);
		apml_get_core_energy(soc_die_num, val1);
		break;
	case 'V':
		/* get data fabric pstate value */
		val1 = atoi(argv[optind - 1]);
		val2 = atoi(argv[optind++]);
		apml_set_df_pstate_range(soc_die_num, val1, val2);
		break;
	case 'e':
		/* read register */
		val = argv[optind - 1];
		val1 = strtoul(argv[optind++], &end, 16);
		read_register(soc_die_num, val1, val);
		break;
	case 'h':
		if (argc > 3 && (validate_number(argv[3], 10))) {
			show_module_commands(argv[0], argv[3]);
		} else
			show_usage(argv[0]);
		return OOB_SUCCESS;
	case ':':
		/* missing option argument */
		printf(RED "%s: option '%s' requires an argument."
			RESET"\n\n", argv[0], argv[optind - 1]);
		break;
	case '?':
		opterr = -1;
		ret = parseesb_mi300_args(argc, argv, soc_die_num);
		if (!ret)
			break;
		printf("Unrecognized option %s\n", argv[2]);
		printf(RED "Try `%s --help' for more"
		       " information." RESET "\n", argv[0]);
		return OOB_SUCCESS;
	default:
		printf(RED "Try `%s --help' for more information."
			RESET "\n\n", argv[0]);
		return OOB_SUCCESS;
	} // end of Switch
	}

	if (optind < argc) {
		printf(RED "\nExtra Non-option argument<s> passed : %s"
				RESET"\n", argv[optind]);
		printf(RED "Try `%s --help' for more information."
				RESET"\n", argv[0]);
	}

	return OOB_SUCCESS;
}


static void rerun_sudo(int argc, char **argv)
{
	static char *args[ARGS_MAX];
	char sudostr[] = "sudo";
	int i;

	args[0] = sudostr;
	for (i = 0; i < argc; i++)
		args[i + 1] = argv[i];
	args[i + 1] = NULL;
	execvp("sudo", args);
}

/*
 * Main program.
 * @param argc number of command line parameters
 * @param argv list of command line parameters
 */
int main(int argc, char **argv)
{
	uint32_t soc_die_num;
	oob_status_t ret;
	uint8_t check = 0;
	int i;

	if (getuid() != 0)
		rerun_sudo(argc, argv);

	for (i = 0; i < argc; i++) {
		if (!strcmp("--readtsiraw", *(argv + i)))
			check = 1;
	}
	if (!check)
		show_smi_message();

	/* Parse command arguments */
	ret = parseesb_args(argc, argv);
	if (ret)
		return ret;

	if (!check)
		show_smi_end_message();

	return ret;
}
