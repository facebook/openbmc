/*
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
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>
#include "mpsse.h"

// support up to 65536 bytes
#define MAX_PKT_SIZE 0x10000
#define MAX_RETRY 10

static int
mpsse_read(struct ftdi_context *ftdi, int num_of_bytes, uint8_t *out) {
  int i = MAX_RETRY;
  int ret = 0;

  do {
    ret = ftdi_read_data(ftdi, out, num_of_bytes);
    if ( ret == num_of_bytes ) break;
  } while( i-- > 0 );

  // ret is a read count, it can't be 0.
  if ( i == 0 || ret == 0 ) {
    printf("%s() Failed to read data! read count=%d\n", __func__, ret);
    ret = -1;
  }

  return ret;
}

static int
mpsse_write(struct ftdi_context *ftdi, int num_of_bytes, uint8_t *in) {
  if ( ftdi_write_data(ftdi, in, num_of_bytes) != num_of_bytes ) {
    printf("%s() Failed to write data! reason: %s\n", __func__, ftdi_get_error_string(ftdi));
    return -1;
  }
  return 0;
}

// Jtag start
//Send TMS and TDI and then read TDO
int
tap_tms_with_read(struct ftdi_context *ftdi, int tms, uint8_t bit7) {
  uint8_t buf[4] = {0};
  int size = 0;
  buf[size++] = MPSSE_WRITE_TMS|MPSSE_DO_READ|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;//0x6B;
  buf[size++] = 0; /* 1 clk */
  buf[size++] = (tms ? 0x01 : 0x00) | ((bit7 & 0x01) << 7);
  buf[size++] = SEND_IMMEDIATE;
  return mpsse_write(ftdi, size, buf);
}

//Send TMS and TDI only
int 
tap_tms(struct ftdi_context *ftdi, int tms, uint8_t bit7) {
  uint8_t buf[3] = {0};
  int size = 0;
  buf[size++] = MPSSE_WRITE_TMS|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;//0x4B;
  buf[size++] = 0; /* 1 clk */
  buf[size++] = (tms ? 0x01 : 0x00) | ((bit7 & 0x01) << 7);
  return mpsse_write(ftdi, size, buf);
}

//Send TMS and TDI in byte mode
int
tap_tms_byte(struct ftdi_context *ftdi, int clk, int tms_seq, uint8_t bit7) {
  uint8_t buf[3] = {0};
  int size = 0;
  buf[size++] = MPSSE_WRITE_TMS|MPSSE_LSB|MPSSE_WRITE_NEG|MPSSE_BITMODE;//0x4B
  buf[size++] = clk - 1; // 0 means 1 clk
  buf[size++] = tms_seq | ((bit7 & 0x01) << 7);
  return mpsse_write(ftdi, size, buf);
}

//Reset TAP state and then go to RTI
int
tap_reset_to_rti(struct ftdi_context *ftdi) {
  // 0x1f is passed in LSB first
  // TMS = 1111 10b
  return tap_tms_byte(ftdi, 6, 0x1f, 0); /* Goto TLR first and then go to RTI*/
}

//Write JTAG data
int
mpsse_jtag_write(struct ftdi_context *ftdi, int num_of_bits, uint8_t *in, uint8_t last_trans) {
  uint8_t buf[MAX_PKT_SIZE] = {0x0};
  int size = 0;
  int ret = -1;
  int num_of_write_bytes = num_of_bits / 8;
  int last_write_bits = num_of_bits % 8;

  if ( last_trans == 1 ) {
    if ( last_write_bits == 1 ) {
      return tap_tms(ftdi, 0x1, in[num_of_write_bytes]); //no need to move on
    }

    if ( last_write_bits == 0 ) {
      num_of_write_bytes--; // the last byte is passed in bit mode
      last_write_bits = 8;
    }
  }

  ret = -1;
  do {
    //write data in byte mode
    if ( num_of_write_bytes > 0 ) {
      size = num_of_write_bytes + 3;// cmd + L_byte + H_byte + data ...
      buf[0] = MPSSE_DO_WRITE|MPSSE_LSB|MPSSE_WRITE_NEG; // 0x19
      buf[1] = (num_of_write_bytes - 1) & 0xff;
      buf[2] = ((num_of_write_bytes - 1) >> 8) & 0xff;
      memcpy(&buf[3], in, num_of_write_bytes);
      if ( mpsse_write(ftdi, size, buf) < 0 ) return -1;
    }

    //write data in bit mode
    if ( last_write_bits > 0 ) {
      if ( last_trans == 1 ) last_write_bits--;
      buf[0] = MPSSE_DO_WRITE|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;//0x1b;
      buf[1] = last_write_bits - 1;
      buf[2] = in[num_of_write_bytes];
      mpsse_write(ftdi, 3, buf);
      if ( last_trans == 1 ) tap_tms(ftdi, 0x1, (in[num_of_write_bytes] >> last_write_bits));
    }
    ret = 0;
  } while(0);
 
  return ret;
}

int
mpsse_jtag_read(struct ftdi_context *ftdi, int num_of_bits, uint8_t *out, uint8_t last_trans) {
  uint8_t buf[MAX_PKT_SIZE] = {0x0};
  int ret = -1;
  int num_of_read_bytes = num_of_bits / 8;
  int last_read_bits = num_of_bits % 8;
  uint8_t tmp_val = 0;

  if ( last_trans == 1 ) {
    if ( last_read_bits == 1 ) {
      ret = tap_tms_with_read(ftdi, 0x1, 0x0);
      if ( ret < 0 ) return ret;

      ret = mpsse_read(ftdi, 1, &tmp_val);
      if ( ret < 0 ) return ret;

      out[num_of_read_bytes] = tmp_val >> 7;
      return ret; // no need to move on
    }

    if ( last_read_bits == 0 ) {
      num_of_read_bytes--; // the last byte is passed in bit mode
      last_read_bits = 8;
    }
  }

  ret = -1;
  do {
    if ( num_of_read_bytes > 0 ) {
      buf[0] = MPSSE_DO_READ|MPSSE_LSB; //0x28;
      buf[1] = (num_of_read_bytes - 1) & 0xff;
      buf[2] = ((num_of_read_bytes - 1) >> 8) & 0xff;
      buf[3] = SEND_IMMEDIATE;
      if ( mpsse_write(ftdi, 4, buf) < 0 ) break;
      if ( mpsse_read(ftdi, num_of_read_bytes, out) < 0 ) break;
    }

    if ( last_read_bits > 0 ) {
      if ( last_trans == 1 ) last_read_bits--;
      buf[0] = MPSSE_DO_READ|MPSSE_LSB|MPSSE_BITMODE; //0x2A;
      buf[1] = (last_read_bits - 1) & 0xff;
      buf[2] = SEND_IMMEDIATE;
      if ( mpsse_write(ftdi, 3, buf) < 0 ) break;
      if ( mpsse_read(ftdi, 1, &tmp_val) < 0 ) break;
      out[num_of_read_bytes] = tmp_val >> (8-last_read_bits);

      if ( last_trans == 1 ) {
        if ( tap_tms_with_read(ftdi, 0x1, 0x0) < 0 ) break;
        if ( mpsse_read(ftdi, 1, &tmp_val) < 0 ) break;
        out[num_of_read_bytes] |= tmp_val & 0x80;
      }
    }
    ret = 0;
  } while(0);

  return ret;
}

// byte mode
int
tap_readwrite(struct ftdi_context *ftdi, int write_bits, uint8_t *in, int read_bits, uint8_t *out, uint8_t last_trans) {
  uint8_t buf[MAX_PKT_SIZE] = {0x0};
  int size = 0;
  int ret = -1;
  int num_of_write_bytes = write_bits / 8;
  int last_write_bits = write_bits % 8;
  //int num_of_read_bytes = read_bits / 8;
  //int last_read_bits = read_bits % 8;
  //int max_bits = (write_bits > read_bits)?write_bits:read_bits;
  uint8_t tmp_val = 0;

  if ( last_trans == 1 ) {
    if ( last_write_bits == 1 ) {
      ret = tap_tms_with_read(ftdi, 0x1, in[num_of_write_bytes]);
      if ( ret < 0 ) return ret;

      ret = mpsse_read(ftdi, 1, &tmp_val);
      if ( ret < 0 ) return ret;

      out[num_of_write_bytes] = tmp_val >> 7;
      return ret; // no need to move on
    }

    if ( last_write_bits == 0 ) {
      num_of_write_bytes--; // the last byte is passed in bit mode
      last_write_bits = 8;
    }
  }

  ret = -1;
  do {
    if ( num_of_write_bytes > 0 ) {
      buf[0] = MPSSE_DO_READ|MPSSE_DO_WRITE|MPSSE_LSB|MPSSE_WRITE_NEG; //0x39;
      buf[1] = (num_of_write_bytes - 1) & 0xff;
      buf[2] = ((num_of_write_bytes - 1) >> 8) & 0xff;
      memcpy(&buf[3], in, num_of_write_bytes);
      size = 3 + num_of_write_bytes;
      buf[size++] = SEND_IMMEDIATE;
      if ( mpsse_write(ftdi, size, buf) < 0 ) break;
      if ( mpsse_read(ftdi, num_of_write_bytes, out) < 0 ) break;
    }

    if ( last_write_bits > 0 ) {
      if ( last_trans == 1 ) last_write_bits--;

      buf[0] = MPSSE_DO_READ|MPSSE_DO_WRITE|MPSSE_LSB|MPSSE_BITMODE|MPSSE_WRITE_NEG;//0x3b;
      buf[1] = (last_write_bits - 1);
      buf[2] = in[num_of_write_bytes];
      buf[3] = SEND_IMMEDIATE;
      if ( mpsse_write(ftdi, 4, buf) < 0 ) break;
      if ( mpsse_read(ftdi, 1, &tmp_val) < 0 ) break;
      out[num_of_write_bytes] = tmp_val >> (8-last_write_bits);
 
      if ( last_trans == 1 ) {
        if ( tap_tms_with_read(ftdi, 0x1, in[num_of_write_bytes] >> (last_write_bits)) < 0 ) break;
        if ( mpsse_read(ftdi, 1, &tmp_val) < 0 ) break;
        out[num_of_write_bytes] |= tmp_val & 0x80;
      }
    }
    ret = 0;
  } while(0);

  return ret;
}
// Jtag end

int
find_devlist(struct ftdi_context *ftdi) {
  int ret = 0;
  int i = 0;
  struct ftdi_device_list *devlist = NULL;
  struct ftdi_device_list *curr = NULL;
  char manufacturer[128] = {0x0}, description[128] = {0x0};

  printf("Checking for FTDI devices...");
  ret = ftdi_usb_find_all(ftdi, &devlist, 0, 0);
  if ( ret < 0 ) {
    fprintf(stderr, "ftdi_usb_find_all failed: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
    return ret;
  }

  printf("Number of FTDI devices found: %d\n", ret);
  i = 0;
  for (curr = devlist; curr != NULL; i++) {
    printf("Checking device: %d\n", i);
    ret = ftdi_usb_get_strings(ftdi, curr->dev, manufacturer, 128, description, 128, NULL, 0);
    if ( ret < 0) {
      fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
      goto do_delist;
    }
    printf("Manufacturer: %s, Description: %s\n\n", manufacturer, description);
    curr = curr->next;
  }

do_delist:
  ftdi_list_free(&devlist);

  return ret;
}

// clear buffer correctly
#define SIO_TCIFLUSH 2
#define SIO_TCOFLUSH 1
static int ftdi_tciflush(struct ftdi_context *ftdi)
{
    if (ftdi == NULL || ftdi->usb_dev == NULL)
        printf("%s() USB device unavailable\n", __func__);

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_RESET_REQUEST, SIO_TCIFLUSH,
                                ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        printf("%s() FTDI purge of RX buffer failed\n", __func__);

    // Invalidate data in the readbuffer
    ftdi->readbuffer_offset = 0;
    ftdi->readbuffer_remaining = 0;
    return 0;
}

static int ftdi_tcoflush(struct ftdi_context *ftdi)
{
    if (ftdi == NULL || ftdi->usb_dev == NULL)
        printf("%s() USB device unavailable\n", __func__);

    if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
                                SIO_RESET_REQUEST, SIO_TCOFLUSH,
                                ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
        printf("%s() FTDI purge of TX buffer failed\n", __func__);

    return 0;
}

int ftdi_tcioflush(struct ftdi_context *ftdi) {
  int ret = -1;

  do {
    if (ftdi == NULL || ftdi->usb_dev == NULL) {
      printf("%s() USB device unavailable\n", __func__);
      break;
    }

    if ( ftdi_tcoflush(ftdi) < 0) {
      printf("%s() Failed to do ftdi_tcoflush\n", __func__);
      break;
    }

    if ( ftdi_tciflush(ftdi) < 0) {
      printf("%s() Failed to do ftdi_tciflush\n", __func__);
      break;
    }
    ret = 0;
  } while(0);

  return ret;
}

int
init_dev(struct ftdi_context *ftdi) {
  int ret = -1;

  do {
    //open dev with PID,VID
    ret= ftdi_usb_open(ftdi, 0x0403, 0x6014);
    if ( ret < 0 && ret != -5 ) {
      printf("%s() Unable to open ftdi device: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }

    printf("Configuring port for MPSSE use...\n");

    ret = ftdi_usb_reset(ftdi);
    if ( ret < 0 ) {
      printf("%s() Unable to reset ftdi device: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }

    ret = ftdi_set_event_char(ftdi, 0, 0);
    if ( ret < 0 ) {
      printf("%s() Unable to disable event char: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }

    ret = ftdi_set_error_char(ftdi, 0, 0);
    if ( ret < 0 ) {
      printf("%s() Unable to disable error char: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }

    ret = ftdi_set_latency_timer(ftdi, 255); //255ms, 0~255
    if ( ret < 0 ) {
      printf("%s() Unable to set latency timer: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }

    ret = ftdi_tcioflush(ftdi);
    if ( ret < 0 ) {
      printf("%s() Unable to purge buffers: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }

    ret = ftdi_set_bitmode(ftdi, 0x00, BITMODE_RESET);
    if ( ret < 0 ) {
      printf("%s() Unable to reset device: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }

    ret = ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);
    if ( ret < 0 ) {
      printf("%s() Unable to set BITMODE_MPSSE: %d (%s)\n", __func__, ret, ftdi_get_error_string(ftdi));
      break;
    }
    ret = 0;
  } while(0);

  return ret;
}

int
mpsse_trigger_trst(struct ftdi_context *ftdi) {
  uint8_t ftdi_init[] = { SET_BITS_LOW, 0x18, 0x1B, /*val, dir*/
                          GET_BITS_LOW,
                         
                          SET_BITS_LOW, 0x08, 0x1B, /*val, dir*/
                          GET_BITS_LOW,

                          SET_BITS_LOW, 0x18, 0x1B, /*val, dir*/
                          GET_BITS_LOW};
 
  if ( mpsse_write(ftdi, sizeof(ftdi_init), ftdi_init) < 0 ) {
    return -1;
  }

  uint8_t out[3] = {0};
  mpsse_read(ftdi, 3, out);
  printf("%02X, %02X, %02X\n", out[0], out[1], out[2]);
  return 0;
}

int
mpsse_init_conf(struct ftdi_context *ftdi, uint16_t tck) {
  int ret = 0;
  //  ADBUS0~3 are used only
  // Pin    Name   Direction Bit   Value  Bit
  // ADBUS0 TCK/SK output    1     Low    0
  // ADBUS1 TDI/DO output    1     Low    0
  // ADBUS2 TDO/DI input     0       0    0
  // ADBUS3 TMS/CS output    1     High   1
  // ADBUS4 GPIOL0 output    1     High   1
  // ADBUS5 GPIOL1 input     0       x    x
  // ADBUS6 GPIOL2 input     0       x    x
  // ADBUS7 GPIOL3 input     0       x    x
  uint8_t ftdi_init[] = {TCK_DIVISOR,   0x00, 0x00, /*12M divisior*/

                         SET_BITS_LOW,  0x08, 0x0B, /*val, dir*/
                         GET_BITS_LOW,

                         SET_BITS_HIGH, 0x00, 0x00, /*val, dir*/
                         GET_BITS_HIGH,
                                  0x8b,
                                  0x86, (tck & 0xff), (tck >> 8) & 0xff,
                         /*LOOPBACK_START, BAD_COMMAND,*/
                         LOOPBACK_END};

  printf("TCK: %.2fK\n", (12582912/((tck+1)*2))/1000.0);
  if ( mpsse_write(ftdi, sizeof(ftdi_init), ftdi_init) < 0 ) {
    return -1;
  }
    
  uint8_t data[2] = {0x0};
  ret = mpsse_read(ftdi, 2, data); //only read GPIO val
  printf("Read bits_low_val: 0x%02X, bits_high_val: 0x%02X\n", data[0], data[1]);

  return ret;
}
