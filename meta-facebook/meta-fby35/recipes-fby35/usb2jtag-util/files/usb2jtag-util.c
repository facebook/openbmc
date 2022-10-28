/*
 * usb2jtag-util
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
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <libftdi1/ftdi.h>
#include <openbmc/obmc-i2c.h>
#include <facebook/mpsse.h>

static void
print_usage_help(void) {
  printf("Usage: usb2jtag-util slot[1|3] --dev [0~11]\n");
  printf("Usage: usb2jtag-util slot[1|3] --trst\n");
}

int init_mpsse(struct ftdi_context *ftdi, uint16_t tck) {
  // find dev
  if ( find_devlist(ftdi) < 0 ) return -1;
  if ( init_dev(ftdi) < 0 ) return -1;
  if ( mpsse_init_conf(ftdi, tck) < 0 ) return -1;
  return 0;
}

void trigger_trst(struct ftdi_context *ftdi) {
  //init mpsse
  if ( init_mpsse(ftdi, 0) < 0 ) {
    printf("Couldn't init the IC\n");
    return;
  }
  printf("Trying to do rst\n");
  mpsse_trigger_trst(ftdi);
}

int
switch_cpld_mux(uint8_t slot_id, uint8_t dev) {
  const uint8_t cpld_addr = 0x5C; //7-bit addr
  uint8_t bus = slot_id + 3;
  int ret = 0;
  int  i2cfd = i2c_cdev_slave_open(bus, cpld_addr, I2C_SLAVE_FORCE_CLAIM);
  if ( i2cfd < 0 ) {
    printf("Failed to open i2c bus\n");
    return -1;
  }

  uint8_t tbuf[2] = {0x01, dev};
  ret = i2c_rdwr_msg_transfer(i2cfd, (cpld_addr << 1), tbuf, sizeof(tbuf), NULL, 0);
  if ( ret < 0 ) {
    printf("Failed to switch mux to M2_%c\n", 65+dev);
  } else {
    printf("Switch mux to M2_%c\n", 65+dev);
  }

  if (i2cfd > 0) close(i2cfd);
  return ret;
}

void scan_dev_byte_mode(struct ftdi_context *ftdi, uint8_t dev, uint16_t tck) {
  uint8_t JTAG_IDCODE = 0x06;

  //init mpsse
  if ( init_mpsse(ftdi, tck) < 0 ) {
    printf("Couldn't init the IC\n");
    return;
  }

  struct  timeval start, end;
  gettimeofday(&start,NULL);

  //reset JTAG TAP and then go to RTI
  tap_reset_to_rti(ftdi);

  tap_tms(ftdi, 1, 0);  /* from RTI to Shift-IR, TMS=1100b*/
  tap_tms(ftdi, 1, 0);
  tap_tms(ftdi, 0, 0);
  tap_tms(ftdi, 0, 0);

  mpsse_jtag_write(ftdi, 10, &JTAG_IDCODE, 1);
  tap_tms(ftdi, 1, 0); /* from Exit1-IR to Shift-DR 11100b */
  tap_tms(ftdi, 1, 0);
  tap_tms(ftdi, 0, 0);
  tap_tms(ftdi, 0, 0);

  //go to shift-dr and then go back to RTI
  uint32_t out = 0;
  mpsse_jtag_read(ftdi, 32, (uint8_t *)&out, 1);
  gettimeofday(&end,NULL);
  printf("ByteMode: Dev%d IDCODE: %08X, time: %lu.%lus\n", dev, out, end.tv_sec - start.tv_sec, end.tv_usec-start.tv_usec);
}

int
main(int argc, char **argv) {
  struct ftdi_context *ftdi = NULL;
  uint8_t slot_id = 0;
  uint8_t dev = 0;

  if ( argc < 2 ) {
    print_usage_help();
    return EXIT_FAILURE;
  }

  if ( strncmp(argv[1], "slot1", 5) == 0 ) slot_id = 1;
  else if ( strncmp(argv[1], "slot3", 5) == 0 ) slot_id = 3;
  else {
    printf("%s is not supported!\n", argv[1]);
    return EXIT_FAILURE;
  }

  if ((ftdi = ftdi_new()) == 0) {
    fprintf(stderr, "ftdi_new failed\n");
    return EXIT_FAILURE;
  }

  if ( (argc == 4 || argc == 5) && (strncmp(argv[2], "--dev", 5) == 0) ) {
    dev = atoi(argv[3]);
    uint16_t tck = (argc == 5)?atoi(argv[4]):0x3;
    if ( dev >= 12 ) {
      printf("only support dev0~11\n");
    } else {
      if ( switch_cpld_mux(slot_id, dev) < 0 ) {}
      else {
        scan_dev_byte_mode(ftdi, dev, tck);
      }
    }
  } else if ( argc == 3 && (strncmp(argv[2], "--trst", 6) == 0) ) {
    trigger_trst(ftdi);
  } else {
    print_usage_help();
  }

  ftdi_usb_reset(ftdi);
  ftdi_usb_close(ftdi);
  ftdi_free(ftdi);
  return 0;
}
