/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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

#include <errno.h>
#include <stdio.h>

#include <facebook/wedge_eeprom.h>
#include <openbmc/log.h>

/*
 * Following fruid info translates to thrift structures in Fboss
 * and has a dependency in FBNet
 */

int main(int argc, const char *argv[])
{
  const char *fn;
  struct wedge_eeprom_st eeprom;
  int rc;

  if (argc >= 2) {
    fn = argv[1];
  } else {
    fn = NULL;
  }

  rc = wedge_eeprom_parse(fn, &eeprom);
  if (rc) {
    fprintf(stderr, "Failed to parse %s EEPROM\n", fn ? fn : "default");
    return -1;
  }

  printf("Wedge EEPROM %s:\n", fn ? fn : "");
  printf("Version: %d\n", eeprom.fbw_version);
  printf("Product Name: %s\n", eeprom.fbw_product_name);
  printf("Product Part Number: %s\n", eeprom.fbw_product_number);
  printf("System Assembly Part Number: %s\n", eeprom.fbw_assembly_number);
  printf("Facebook PCBA Part Number: %s\n", eeprom.fbw_facebook_pcba_number);
  printf("Facebook PCB Part Number: %s\n", eeprom.fbw_facebook_pcb_number);
  printf("ODM PCBA Part Number: %s\n", eeprom.fbw_odm_pcba_number);
  printf("ODM PCBA Serial Number: %s\n", eeprom.fbw_odm_pcba_serial);
  printf("Product Production State: %d\n", eeprom.fbw_production_state);
  printf("Product Version: %d\n", eeprom.fbw_product_version);
  printf("Product Sub-Version: %d\n", eeprom.fbw_product_subversion);
  printf("Product Serial Number: %s\n", eeprom.fbw_product_serial);
  printf("Product Asset Tag: %s\n", eeprom.fbw_product_asset);
  printf("System Manufacturer: %s\n", eeprom.fbw_system_manufacturer);
  printf("System Manufacturing Date: %s\n",
         eeprom.fbw_system_manufacturing_date);
  printf("PCB Manufacturer: %s\n", eeprom.fbw_pcb_manufacturer);
  printf("Assembled At: %s\n", eeprom.fbw_assembled);
  printf("Local MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
         eeprom.fbw_local_mac[0], eeprom.fbw_local_mac[1],
         eeprom.fbw_local_mac[2], eeprom.fbw_local_mac[3],
         eeprom.fbw_local_mac[4], eeprom.fbw_local_mac[5]);
  printf("Extended MAC Base: %02X:%02X:%02X:%02X:%02X:%02X\n",
         eeprom.fbw_mac_base[0], eeprom.fbw_mac_base[1],
         eeprom.fbw_mac_base[2], eeprom.fbw_mac_base[3],
         eeprom.fbw_mac_base[4], eeprom.fbw_mac_base[5]);
  printf("Extended MAC Address Size: %d\n", eeprom.fbw_mac_size);
  printf("Location on Fabric: %s\n", eeprom.fbw_location);
  printf("CRC8: 0x%x\n", eeprom.fbw_crc8);

  return 0;
}
