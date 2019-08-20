/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
/*
Please get the JEDEC file format before you read the code
*/

#include <stdio.h>
#include <stdlib.h>
#include "cpld.h"

#ifdef LATTICE_SUPPORT
#include "lattice.h"
#endif

struct cpld_dev_info *cur_dev = NULL;
struct cpld_dev_info **cpld_device_list;


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int cpld_device_init()
{
  int i, j = 0;
  int dev_cnts = 0;

#ifdef LATTICE_SUPPORT
  dev_cnts += ARRAY_SIZE(lattice_device_list);
#endif

  cpld_device_list = (struct cpld_dev_info **)malloc(sizeof(struct cpld_dev_info *) * dev_cnts);

#ifdef LATTICE_SUPPORT
  for (i = 0; i < ARRAY_SIZE(lattice_device_list); i++, j++)
    cpld_device_list[j] = &lattice_device_list[i];
#endif

  return dev_cnts;
}

int cpld_intf_open(void)
{
  int i;
  int dev_cnts;
  unsigned int dev_id;

  if (cur_dev != NULL)
    return 0;

  if (cpld_device_open() < 0)
    return -1;

  dev_cnts = cpld_device_init();
  for (i = 0; i < dev_cnts; i++) {
    cur_dev = cpld_device_list[i];
    if (!cpld_get_device_id(&dev_id) && dev_id == cpld_device_list[i]->dev_id)
      break;
  }

  if (i == dev_cnts) {
    //No CPLD device match
    printf("Unknown CPLD dev_id = 0x%x\n", dev_id);
    cpld_intf_close();
    return -1;
  }

  return 0;
}

int cpld_intf_close(void)
{
  cur_dev = NULL;
  free(cpld_device_list);
  cpld_device_close();

  return 0;
}

int cpld_get_ver(unsigned int *ver)
{
  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_ver) {
    printf("Get version not supported\n");
    return -1;
  }

  return cur_dev->cpld_ver(ver);
}

int cpld_get_device_id(unsigned int *dev_id)
{
  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_dev_id) {
    printf("Get device id not supported\n");
    return -1;
  }

  return cur_dev->cpld_dev_id(dev_id);
}

int cpld_verify(char *file)
{
  int ret;
  FILE *fp_in = NULL;

  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_verify) {
    printf("Verify CPLD not supported\n");
    return -1;
  }

  fp_in = fopen(file, "r");
  if (NULL == fp_in) {
    printf("[%s] Cannot Open File %s!\n", __func__, file);
    return -1;
  }

  cur_dev->cpld_verify(fp_in);
  fclose(fp_in);

  return ret;
}

int cpld_program(char *file)
{
  int ret;
  FILE *fp_in = NULL;

  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_program) {
    printf("Program CPLD not supported\n");
    return -1;
  }

  fp_in = fopen(file, "r");
  if (NULL == fp_in) {
    printf("[%s] Cannot Open File %s!\n", __func__, file);
    return -1;
  }

  ret = cur_dev->cpld_program(fp_in);
  fclose(fp_in);

  return ret;
}

int cpld_erase(void)
{
  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_erase) {
    printf("Erase CPLD not supported\n");
    return -1;
  }

  return cur_dev->cpld_erase();
}
