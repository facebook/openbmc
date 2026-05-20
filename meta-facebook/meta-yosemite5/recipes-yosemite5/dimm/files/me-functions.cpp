/*
 * Copyright 2026-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include <esmi_mailbox.h>
}
#include "dimm-util-plat.h"
#include "dimm.h"

#ifdef DEBUG_DIMM_UTIL
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

static uint8_t yv5_dimm_addr[MAX_DIMM_PER_CPU_YV5] = {
    DIMM_A0_ADDR, DIMM_A1_ADDR, DIMM_A2_ADDR, DIMM_A3_ADDR,
    DIMM_A4_ADDR, DIMM_A5_ADDR, DIMM_A6_ADDR, DIMM_A7_ADDR,
    DIMM_A8_ADDR, DIMM_A9_ADDR, DIMM_A10_ADDR, DIMM_A11_ADDR,
    DIMM_A12_ADDR, DIMM_A13_ADDR, DIMM_A14_ADDR, DIMM_A15_ADDR,
};

// dimm location constant strings, matching silk screen
static const char* yv5_dimm_label[NUM_CPU_YV5][MAX_DIMM_PER_CPU_YV5] = {
    {"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10", "A11",
     "A12", "A13", "A14", "A15"},
};

static const char* fru_name_yv5[NUM_FRU_YV5] = {
    "mb",
};

static uint8_t* dimm_addr = yv5_dimm_addr;
static const char* (*dimm_label)[MAX_DIMM_PER_CPU_YV5] = yv5_dimm_label;

int plat_init(void)
{
    num_frus = NUM_FRU_YV5;
    fru_name = fru_name_yv5;
    fru_id_min = FRU_ID_MIN_YV5;
    fru_id_max = FRU_ID_MAX_YV5;
    fru_id_all = FRU_ID_ALL_YV5;

    num_dimms_per_cpu = MAX_DIMM_PER_CPU_YV5;
    num_cpus = NUM_CPU_YV5;
    total_dimms = num_dimms_per_cpu * num_cpus;

    read_block = true;
    return 0;
}

const char* get_dimm_label(uint8_t cpu, uint8_t dimm)
{
    if ((cpu >= num_cpus) || (dimm >= num_dimms_per_cpu))
    {
        return "N/A";
    }
    return dimm_label[cpu][dimm];
}

int util_read_spd(uint8_t /*fru_id*/, uint8_t cpu, uint8_t dimm,
                  uint16_t offset, uint8_t len, uint8_t* rxbuf)
{
    struct dimm_spd_d_in spd_in = {0};
    oob_status_t ret;
    uint32_t spd_data = 0;
    uint8_t read_bytes = 0;

    if (rxbuf == NULL || dimm >= num_dimms_per_cpu)
    {
        return -1;
    }

    spd_in.dimm_addr = dimm_addr[dimm];
    spd_in.lid = 0xa;     // SPD
    spd_in.reg_space = 1; // NVM
    spd_in.rsvd = 0;

    while (read_bytes < len)
    {
        spd_in.reg_offset = offset + read_bytes;
        ret = read_dimm_spd_register(cpu, spd_in, &spd_data);
        if (ret != OOB_SUCCESS)
        {
            DBG_PRINT("APML read SPD failed at offset 0x%x: %d\n",
                      spd_in.reg_offset, ret);
            return (read_bytes > 0) ? read_bytes : -1;
        }

        uint8_t chunk = (len - read_bytes > 4) ? 4 : (len - read_bytes);
        memcpy(rxbuf + read_bytes, &spd_data, chunk);
        read_bytes += chunk;
    }

    return read_bytes;
}

int util_set_EE_page(uint8_t /*fru_id*/, uint8_t /*cpu*/, uint8_t /*dimm*/,
                     uint8_t /*page_num*/)
{
    return 0;
}

int util_check_me_status(uint8_t /*fru_id*/)
{
    return 0;
}

bool is_pmic_supported(void)
{
    return true;
}

int util_read_pmic(uint8_t /*fru_id*/, uint8_t cpu, uint8_t dimm,
                   uint8_t offset, uint8_t len, uint8_t* rxbuf)
{
    struct dimm_sb_reg_d_in sb_in = {0};
    oob_status_t ret;
    uint32_t reg_data = 0;
    uint8_t read_bytes = 0;

    if (rxbuf == NULL || dimm >= num_dimms_per_cpu)
    {
        return -1;
    }

    sb_in.dimm_addr = dimm_addr[dimm];
    sb_in.lid = 0x9; // PMIC
    sb_in.reg_space = 0;
    sb_in.rsvd = 0;

    while (read_bytes < len)
    {
        sb_in.reg_offset = offset + read_bytes;
        ret = get_dimm_sb_register(cpu, sb_in, &reg_data);
        if (ret != OOB_SUCCESS)
        {
            DBG_PRINT("APML read PMIC failed at offset 0x%x: %d\n",
                      sb_in.reg_offset, ret);
            return (read_bytes > 0) ? read_bytes : -1;
        }

        uint8_t chunk = (len - read_bytes > 4) ? 4 : (len - read_bytes);
        memcpy(rxbuf + read_bytes, &reg_data, chunk);
        read_bytes += chunk;
    }

    return read_bytes;
}

int util_write_pmic(uint8_t /*fru_id*/, uint8_t cpu, uint8_t dimm,
                    uint8_t offset, uint8_t len, uint8_t* txbuf)
{
    struct dimm_sb_reg_write sb_write = {0};
    oob_status_t ret;
    uint8_t write_bytes = 0;

    if (txbuf == NULL || dimm >= num_dimms_per_cpu)
    {
        return -1;
    }

    sb_write.dimm_addr = dimm_addr[dimm];
    sb_write.lid = 0x9; // PMIC
    sb_write.reg_space = 0;

    while (write_bytes < len)
    {
        sb_write.reg_offset = offset + write_bytes;
        sb_write.w_data = txbuf[write_bytes];
        ret = set_dimm_sb_register_data(cpu, sb_write);
        if (ret != OOB_SUCCESS)
        {
            DBG_PRINT("APML write PMIC failed at offset 0x%x: %d\n",
                      sb_write.reg_offset, ret);
            return (write_bytes > 0) ? write_bytes : -1;
        }
        write_bytes++;
    }

    return write_bytes;
}
