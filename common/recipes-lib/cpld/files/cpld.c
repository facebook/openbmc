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
#include <string.h>
#include "cpld.h"
#include "lattice.h"
#include "altera.h"

struct cpld_dev_info *cur_dev = NULL;
struct cpld_dev_info **cpld_dev_list;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

const char *cpld_list[] = {
  "LCMXO2-2000HC",
  "LCMXO2-4000HC",
  "LCMXO2-7000HC",
  "LCMXO3-2100C",
  "LCMXO3-9400C",
  "LFMNX-50",
  "MAX10-10M16",
  "MAX10-10M25",
  "MAX10-10M04",
};

static int cpld_probe(void *attr)
{
  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_open) {
    printf("Open not supported\n");
    return -1;
  }

  return cur_dev->cpld_open(attr);
}

static int cpld_remove()
{
  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_close) {
    printf("Close not supported\n");
    return -1;
  }

  return cur_dev->cpld_close();
}

static int cpld_malloc_list()
{
  int i, j = 0;
  int dev_cnts = 0;
  int lcnts = 0;
  int acnts = 0;
  struct cpld_dev_info* lattice_list;

  lcnts = get_lattice_dev_list(&lattice_list);
  acnts += ARRAY_SIZE(altera_dev_list);

  dev_cnts = lcnts + acnts;

  cpld_dev_list = (struct cpld_dev_info **)malloc(sizeof(struct cpld_dev_info *) * dev_cnts);

  for (i = 0; i < lcnts; i++, j++)
    cpld_dev_list[j] = &lattice_list[i];
  for (i = 0; i < acnts; i++, j++)
    cpld_dev_list[j] = &altera_dev_list[i];

  return dev_cnts;
}

int cpld_intf_open(uint8_t cpld_index, cpld_intf_t intf, void *attr)
{
  int i;
  int dev_cnts;

  if (cur_dev != NULL)
    return 0;

  if (cpld_index >= UNKNOWN_DEV) {
    printf("Unknown CPLD index = %d\n", cpld_index);
    return -1;
  }

  dev_cnts = cpld_malloc_list();
  for (i = 0; i < dev_cnts; i++) {
    if ( !strcmp(cpld_list[cpld_index], cpld_dev_list[i]->name) &&
         (intf == cpld_dev_list[i]->intf) ) {
      cur_dev = cpld_dev_list[i];
      break;
    }
  }
  if (i == dev_cnts) {
    //No CPLD device match
    printf("Unknown CPLD name = %s\n", cpld_list[cpld_index]);
    free(cpld_dev_list);
    return -1;
  }

  return cpld_probe(attr);
}

int cpld_intf_close()
{
  int ret;

  ret = cpld_remove();
  free(cpld_dev_list);
  cur_dev = NULL;

  return ret;
}

int cpld_get_ver(uint32_t *ver)
{
  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_ver) {
    printf("Get version not supported\n");
    return -1;
  }

  return cur_dev->cpld_ver(ver);
}

int cpld_get_device_id(uint32_t *dev_id)
{
  if (cur_dev == NULL)
    return -1;

  if (!cur_dev->cpld_dev_id) {
    printf("Get device id not supported\n");
    return -1;
  }

  return cur_dev->cpld_dev_id(dev_id);
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

int cpld_program(char *file, char *key, char is_signed)
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

  ret = cur_dev->cpld_program(fp_in, key, is_signed);
  fclose(fp_in);

  return ret;
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

  ret = cur_dev->cpld_verify(fp_in);
  fclose(fp_in);

  return ret;
}
