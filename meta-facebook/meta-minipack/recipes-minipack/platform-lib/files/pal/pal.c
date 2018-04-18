/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
  * This file contains functions and logics that depends on Wedge100 specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include "pal.h"
#include <facebook/bic.h>
#include <openbmc/edb.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensor.h>

#define LAST_KEY "last_key"

char * key_list[] = {
"sysfw_ver_slot1",
/* Add more Keys here */
LAST_KEY /* This is the last key of the list */
};

char * def_val_list[] = {
  "0", /* sysfw_ver_slot1 */
/* Add more Keys here */
  LAST_KEY /* Same as last entry of the key_list */
};

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
	*status = 1;
	return 0;
}

void
pal_inform_bic_mode(uint8_t fru, uint8_t mode) {
  switch(mode) {
  case BIC_MODE_NORMAL:
    // Bridge IC entered normal mode
    // Inform BIOS that BMC is ready
    bic_set_gpio(fru, BMC_READY_N, 0);
    break;
  case BIC_MODE_UPDATE:
    // Bridge IC entered update mode
    // TODO: Might need to handle in future
    break;
  default:
    break;
  }
}

//For OEM command "CMD_OEM_GET_PLAT_INFO" 0x7e
int
pal_get_plat_sku_id(void){
  return 0x06; // Minipack
}


//Use part of the function for OEM Command "CMD_OEM_GET_POSS_PCIE_CONFIG" 0xF4
int
pal_get_poss_pcie_config(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len) {

  uint8_t completion_code = CC_SUCCESS;
  uint8_t pcie_conf = 0x02;//Minipack
  uint8_t *data = res_data;

  *data++ = pcie_conf;
  *res_len = data - res_data;
  return completion_code;
}
