/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>

#include "pal.h"
#include "pal-pim.h"

#define GET_LOW_16B(val)            (val & 0xffff)
#define GET_HIGH_16B(val)           ((val & 0xffff0000) >> 16)
#define BUILD_U32(lsb, msb)         ((msb << 16) | lsb)

#define BCM87326_PHY_ID_16NM        0x1356
#define BCM87326_PHY_ID_7NM         0x7326
#define BCM87326_PHY_ID_REG         0x5201d000

#define BCM87326_ADDR_L             0x0000
#define BCM87326_ADDR_H             0x0001
#define BCM87326_DATA_L             0x0002
#define BCM87326_DATA_H             0x0003
#define BCM87326_CTRL               0x0004

#define DOM_FPGA_MDIO_BASE          0x200
#define DOM_FPGA_MDIO_OFFSET        0x40
#define DOM_FPGA_MDIO_COMMAND       0x04
#define   MDIO_CMD_REG(reg)         (reg << 16)
#define   MDIO_CMD_PHY_ID(phy)      ((phy * 0x4) << 8)
#define   MDIO_CMD_READ             (1 << 7)
#define   MDIO_CMD_WRITE            (0 << 7)
#define   MDIO_CMD_DIRECT_RW        0x1e
#define   MDIO_CMD_INDIRECT_RW      0x1f
#define DOM_FPGA_MDIO_WRITE         0x08
#define DOM_FPGA_MDIO_READ          0x0c
#define DOM_FPGA_MDIO_STATUS        0x10
#define   MDIO_TRANS_DONE           (1 << 0)    // write 1 to clear
#define   MDIO_TRANS_ERROR          (1 << 1)    // write 1 to clear

#define MDIO_BASE(phy)              (DOM_FPGA_MDIO_BASE + DOM_FPGA_MDIO_OFFSET * phy)

/* This is fixed value */
enum {
    PHY1 = 0,
    PHY2 = 1,
    PHY3 = 2,
    PHY4 = 3,
    PHY_NUM,
};

static int pim_dom_fpga_indirect_read(int fru, u_int32_t reg, int *value)
{
    char path[LARGEST_DEVICE_NAME];
    char buff[11]; //32 bits hex value 0x00000000
    uint8_t bus = ((fru - FRU_PIM1) * PIM_I2C_OFFSET) + PIM_I2C_BASE;

    snprintf(path, sizeof(path), I2C_SYSFS_DEVICES"/%d-0060/indirect_addr", bus);
    snprintf(buff, sizeof(buff), "0x%x", reg);

    if (device_write_buff(path, buff))
        return -1;

    snprintf(path, sizeof(path), I2C_SYSFS_DEVICES"/%d-0060/indirect_read", bus);

    if (device_read(path, value))
        return -1;

    return 0;
}

static int pim_dom_fpga_indirect_write(int fru, u_int32_t reg, int value)
{
    char path[LARGEST_DEVICE_NAME];
    char buff[11]; //32 bits hex value 0x00000000
    uint8_t bus = ((fru - FRU_PIM1) * PIM_I2C_OFFSET) + PIM_I2C_BASE;

    snprintf(path, sizeof(path), I2C_SYSFS_DEVICES"/%d-0060/indirect_addr", bus);
    snprintf(buff, sizeof(buff), "0x%x", reg);

    if (device_write_buff(path, buff))
        return -1;

    snprintf(path, sizeof(path), I2C_SYSFS_DEVICES"/%d-0060/indirect_write", bus);
    snprintf(buff, sizeof(buff), "0x%x", value);

    if (device_write_buff(path, buff))
        return -1;

    return 0;
}

static int pim_dom_fpga_mdio_done(int fru, int phy) {
    int retry = 0;
    int value = 0;
    int mdio_status_addr = MDIO_BASE(phy) + DOM_FPGA_MDIO_STATUS;
    do {
        pim_dom_fpga_indirect_read(fru, mdio_status_addr, &value);
        if (value & 0x1)
            return 0;
        usleep(5000);
    } while (retry++ < 10);

    return -1;
}

static int pim_get_7nm_phy_id(int fru, int phy, u_int32_t *chip_id)
{
    int lsb, msb;
    int mdio_read_addr   = MDIO_BASE(phy) + DOM_FPGA_MDIO_READ;
    int mdio_write_addr  = MDIO_BASE(phy) + DOM_FPGA_MDIO_WRITE;
    int mdio_status_addr = MDIO_BASE(phy) + DOM_FPGA_MDIO_STATUS;
    int mdio_cmd_addr    = MDIO_BASE(phy) + DOM_FPGA_MDIO_COMMAND;

    /* step 1: set PHY ID reg low 16 bits */
    pim_dom_fpga_indirect_write(fru, mdio_write_addr, GET_LOW_16B(BCM87326_PHY_ID_REG));
    pim_dom_fpga_indirect_write(fru, mdio_status_addr, MDIO_TRANS_DONE | MDIO_TRANS_ERROR);
    pim_dom_fpga_indirect_write(fru, mdio_cmd_addr, MDIO_CMD_REG(BCM87326_ADDR_L) | MDIO_CMD_WRITE |
                                                    MDIO_CMD_PHY_ID(phy) | MDIO_CMD_INDIRECT_RW);
    if (pim_dom_fpga_mdio_done(fru, phy))
        return -1;

    /* step 2: set PHY ID reg high 16 bits */
    pim_dom_fpga_indirect_write(fru, mdio_write_addr, GET_HIGH_16B(BCM87326_PHY_ID_REG));
    pim_dom_fpga_indirect_write(fru, mdio_status_addr, MDIO_TRANS_DONE | MDIO_TRANS_ERROR);
    pim_dom_fpga_indirect_write(fru, mdio_cmd_addr, MDIO_CMD_REG(BCM87326_ADDR_H) | MDIO_CMD_WRITE |
                                                    MDIO_CMD_PHY_ID(phy) | MDIO_CMD_INDIRECT_RW);
    if (pim_dom_fpga_mdio_done(fru, phy))
        return -1;

    /* step 3: read PHY ID reg low 16 bits */
    pim_dom_fpga_indirect_write(fru, mdio_status_addr, MDIO_TRANS_DONE | MDIO_TRANS_ERROR);
    pim_dom_fpga_indirect_write(fru, mdio_cmd_addr, MDIO_CMD_REG(BCM87326_DATA_L) | MDIO_CMD_READ |
                                                    MDIO_CMD_PHY_ID(phy) | MDIO_CMD_INDIRECT_RW);
    if (pim_dom_fpga_mdio_done(fru, phy))
        return -1;
    pim_dom_fpga_indirect_read(fru, mdio_read_addr, &lsb);

    /* step 4: read PHY ID reg high 16 bits */
    pim_dom_fpga_indirect_write(fru, mdio_status_addr, MDIO_TRANS_DONE | MDIO_TRANS_ERROR);
    pim_dom_fpga_indirect_write(fru, mdio_cmd_addr, MDIO_CMD_REG(BCM87326_DATA_H) | MDIO_CMD_READ |
                                                    MDIO_CMD_PHY_ID(phy) | MDIO_CMD_INDIRECT_RW);
    if (pim_dom_fpga_mdio_done(fru, phy))
        return -1;
    pim_dom_fpga_indirect_read(fru, mdio_read_addr, &msb);

    *chip_id = BUILD_U32(lsb, msb);
    return 0;
}

/*
 * check PIM is 7nm or 16nm
 * 7nm: return 1
 * 16nm: return 0
 */
int pal_pim_is_7nm(int fru)
{
    u_int32_t chip_id = 0;

    /* If find any one of the 4 PHY is BCM87326, the pim should be 7nm.
     * We check all the phy in case of mdio access failure. */
    for (int phy = 0; phy < PHY_NUM; phy++) {
        if(pim_get_7nm_phy_id(fru, phy, &chip_id))
            continue;
        if (chip_id == BCM87326_PHY_ID_7NM) {
            return 1;
        }
    }

    /* If none the 4 PHY is BCM87326, the pim should be 16nm */
    return 0;
}
