/*
 * force_mtd_parts.c - Recover spooked openbmc images which have lost access to
 *                     the BMC (flash1) SPI flash image.
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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

//#define DEBUG

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/partitions.h>

static struct mtd_partition ast_data_partitions[] = {
	{
		.name       = "romx",              /* (unused) */
		.offset     = 0,                   /* From 0 */
		.size       = 0x60000,             /* Size 384K */
		.mask_flags = MTD_WRITEABLE,
	}, {
		.name       = "env",               /* U-Boot NVRAM */
		.offset     = MTDPART_OFS_APPEND,  /* From 384K */
		.size       = 0x20000,             /* Size 128K, two sectors */
	}, {
		.name       = "u-boot",            /* Signed: U-boot, intermediate keys */
		.offset     = MTDPART_OFS_APPEND,  /* From 512K */
		.size       = 0x60000,             /* Size 384K */
	}, {
		.name       = "fit",               /* Signed: kernel, rootfs */
		.offset     = MTDPART_OFS_APPEND,  /* From 896K */
		.size       = 0x1B20000,           /* Size 27.125M */
	}, {
		.name       = "data0",
		.offset     = MTDPART_OFS_APPEND,
		.size       = MTDPART_SIZ_FULL,
	}, {
		.name       = "flash1",
		.offset     = 0,                   /* From 0 */
		.size       = MTDPART_SIZ_FULL,    /* full size */
	}, {
		.name       = "flash1rw",          /* Writable flash1 region */
		.offset     = 0x10000,
		.size       = MTDPART_SIZ_FULL,
	},
};

/* The data is FMC.1 (CS1) */
static struct flash_platform_data ast_data_platform_data = {
	.type       = "mx25l25635e",
	.nr_parts   = ARRAY_SIZE(ast_data_partitions),
	.parts      = ast_data_partitions,
};


static struct spi_board_info bmc_spi[] = {
	{
		.modalias    		= "m25p80",
		.platform_data  = &ast_data_platform_data,
		.chip_select    = 1,
		.max_speed_hz   = 50 * 1000 * 1000,
		.bus_num    		= 0,
		.mode 			    = SPI_MODE_0,
	}
};

static struct spi_device *my_device = NULL;

static int __init force_mtd_parts_init(void)
{
  u16 busno = 0;
  struct spi_master *master;

  printk(KERN_DEBUG "force_mtd_parts initializing");

  master = spi_busnum_to_master(busno);
  if (!master) {
    printk(KERN_ERR "Cannot get master for bus:0");
    return -1;
  }
  my_device = spi_new_device(master, bmc_spi);
  if (!my_device) {
    printk(KERN_ERR "Error adding new device!");
    return -1;
  }
  return 0;
}

static void __exit force_mtd_parts_exit(void)
{
  printk(KERN_DEBUG "force_mtd_parts exiting");
  if (my_device) {
    spi_unregister_device(my_device);
    my_device = NULL;
  }
}

MODULE_AUTHOR("Amithash Prasad <amithash@fb.com>");
MODULE_DESCRIPTION("Force flash1 partitions to be visible to userspace");
MODULE_LICENSE("GPL");

module_init(force_mtd_parts_init);
module_exit(force_mtd_parts_exit);
