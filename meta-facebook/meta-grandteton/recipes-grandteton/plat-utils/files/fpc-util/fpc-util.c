/*
 * fpc-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include <libpldm/pldm.h>
#include <libpldm/platform.h>
#include <libpldm-oem/pldm.h>

#define SWB_E1S_AMMBER_LED_ON  0x01
#define SWB_E1S_AMMBER_LED_OFF 0x02


enum plat_pldm_effecter_id
{
  PLAT_EFFECTER_ID_POWER_LED = 0x00,
  PLAT_EFFECTER_ID_FAULT_LED = 0x01,
  PLAT_EFFECTER_ID_LED_E1S_0 = 0x10,
  PLAT_EFFECTER_ID_LED_E1S_1 = 0x11,
  PLAT_EFFECTER_ID_LED_E1S_2 = 0x12,
  PLAT_EFFECTER_ID_LED_E1S_3 = 0x13,
  PLAT_EFFECTER_ID_LED_E1S_4 = 0x14,
  PLAT_EFFECTER_ID_LED_E1S_5 = 0x15,
  PLAT_EFFECTER_ID_LED_E1S_6 = 0x16,
  PLAT_EFFECTER_ID_LED_E1S_7 = 0x17,
  PLAT_EFFECTER_ID_LED_E1S_8 = 0x18,
  PLAT_EFFECTER_ID_LED_E1S_9 = 0x19,
  PLAT_EFFECTER_ID_LED_E1S_10 = 0x1A,
  PLAT_EFFECTER_ID_LED_E1S_11 = 0x1B,
  PLAT_EFFECTER_ID_LED_E1S_12 = 0x1C,
  PLAT_EFFECTER_ID_LED_E1S_13 = 0x1D,
  PLAT_EFFECTER_ID_LED_E1S_14 = 0x1E,
  PLAT_EFFECTER_ID_LED_E1S_15 = 0x1F,
};


static int
set_swb_amber_led(uint8_t e1s_id, uint8_t status)
{
  uint8_t tbuf[255] = {0};
  uint8_t* rbuf = (uint8_t *) NULL;//, tmp;
  uint8_t tlen = 0;
  size_t  rlen = 0;
  int rc;

//  printf("id=%x, status=%d\n", e1s_id, status);
  struct pldm_msg* pldmbuf = (struct pldm_msg *)tbuf;
  pldmbuf->hdr.request = 1;
  pldmbuf->hdr.type    = PLDM_PLATFORM;
  pldmbuf->hdr.command = PLDM_SET_STATE_EFFECTER_STATES;
  tlen = PLDM_HEADER_SIZE;
  tbuf[tlen++] = e1s_id + PLAT_EFFECTER_ID_LED_E1S_0;
  tbuf[tlen++] = 0xE0;
  tbuf[tlen++] = 0x01;
  tbuf[tlen++] = PLDM_REQUEST_SET;
  tbuf[tlen++] = status;

  rc = oem_pldm_send_recv(SWB_BUS_ID, SWB_BIC_EID, tbuf, tlen, &rbuf, &rlen);
  return rc;
}


static void
print_usage_help(void) {
  printf("Usage: fpc-util <sled> --identify <on/off>\n");
  printf("Usage: fpc-util <e1s> <0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15> <on/off>\n");
}

int
main(int argc, char **argv) {
  char tstr[64] = {0};
  uint8_t status=0;

  if (argc < 4 || argc > 5) {
    goto err_exit;
  }

  if (strcmp(argv[1], "sled") == 0) {
    if ( strcmp(argv[2], "--identify") || (strcmp(argv[3], "on") != 0 && strcmp(argv[3], "off") != 0) ) {
      goto err_exit;
    }

    printf("fpc-util: identification for %s is %s\n", argv[1], argv[3]);
    sprintf(tstr, "identify_sled");
    return pal_set_key_value(tstr, argv[3]);
  } else if((strcmp(argv[1], "e1s") == 0)) {

    if(atoi(argv[2]) < 0 || atoi(argv[2]) > 15) {
      goto err_exit;
    }

    printf("fpc-util: %s %s LED is %s\n", argv[1], argv[2], argv[3]);
    if (strcmp(argv[3], "on") == 0)
      status = SWB_E1S_AMMBER_LED_ON;
    else if(strcmp(argv[3], "off") == 0)
      status = SWB_E1S_AMMBER_LED_OFF;
    else
      goto err_exit;

    return set_swb_amber_led(atoi(argv[2]), status);
  } else {
    goto err_exit;
  }

err_exit:
  print_usage_help();
  return -1;
}
