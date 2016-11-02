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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

static void
print_usage_help(void) {
  printf("Usage: fpc-util <slot1> --ext1 <warning/off>\n");
  printf("Usage: fpc-util <slot1> --ext2 <warning/off>\n");
  printf("       fpc-util <sled> --fault <on/off/auto>\n");
  printf("       fpc-util <slot1> --identify <on/off>\n");
}

int
main(int argc, char **argv) {

  uint8_t slot_id;
  char tstr[64] = {0};

  if (argc < 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "sled")) {
    slot_id = 0;
  } else {
    goto err_exit;
  }
  
  if (!strcmp(argv[2], "--ext2")) {
	  int LED = 0;
    printf("fpc-util: Enable/Disable miniSAS2 Error LED\n");
    if(!strcmp(argv[3], "warning")){
		LED = 1;
		return pal_minisas_led(1,LED);
	}else if(!strcmp(argv[3], "off")){
		LED = 0;
	    return pal_minisas_led(1,LED);
	}
    return -1;// to do pal_minisas_led()
  }else  if (!strcmp(argv[2], "--ext1")) {
	  int LED = 0;
    printf("fpc-util: Enable/Disable miniSAS1 Error LED\n");
    if(!strcmp(argv[3], "warning")){
		LED = 1;
		return pal_minisas_led(0,LED);
	}else if(!strcmp(argv[3], "off")){
		LED = 0;
	    return pal_minisas_led(0,LED);
	}
    return -1;// to do pal_minisas_led()
  } else if (!strcmp(argv[2], "--fault")) {
	  printf("fpc-util: Enable/Disable fault LED manually\n");
		int mode  = 0;
		if(!strcmp(argv[3], "on")){  
			mode = 1;
			return pal_fault_led(1,mode);
	    }else if(!strcmp(argv[3], "off")){
			mode = 1;
			return pal_fault_led(0,mode);
		}else if(!strcmp(argv[3], "auto")){
			mode = 0;
			return pal_fault_led(0,mode);
		}
        return -1;
  } else if (!strcmp(argv[2], "--identify")) {
    if (argc != 4) {
      goto err_exit;
  }
    printf("fpc-util: identification for %s is %s\n", argv[1], argv[3]);
    if (slot_id == 1) {
      sprintf(tstr, "identify_slot1");
    } 

    return pal_set_key_value(tstr, argv[3]);
  } else {
    goto err_exit;
  }

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
