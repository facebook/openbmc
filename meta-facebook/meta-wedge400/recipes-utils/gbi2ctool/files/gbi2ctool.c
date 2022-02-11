/*
 * gbi2ctool
 *
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <openbmc/obmc-i2c.h>

void print_i2c_data(const unsigned char *data, size_t len)
{
    int i = 0;
    for (i = 0; i < len; i++) {
        fprintf(stdout, "%02x ", data[i]);
        if (i % 16 == 15) {
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, "\n");
}

static int
i2c_open(uint8_t bus_num, uint8_t addr, uint8_t force) {
  int fd = -1, rc = -1;
  char fn[32];

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    fprintf(stderr, "Failed to open i2c device %s\n", fn);
    return -1;
  }

  rc = ioctl(fd, force ? I2C_SLAVE_FORCE : I2C_SLAVE, addr);
  if (rc < 0) {
    fprintf(stderr, "Failed to open slave @ address 0x%x\n", addr);
    close(fd);
    return -1;
  }
  return fd;
}

void usage(char *app_name){
    fprintf(stdout, "\nUsage:%s [-h] [-f] <bus_num> <dev_addr> <reg_addr> [data_dword]\n"
                "  option:   -f          - Add force flag when open the i2c\n"
                "            -h          - show this usage\n"
                "  argument: <bus_num>   - bus of i2c (start with 0)\n"
                "            <dev_addr>  - i2c device address\n"
                "            <reg_addr>  - register address (4 byte address)\n"
                "            [data_dword]- data for writing (4 byte data)\n"
                "Example as:\n"
                "  [reading] - %s 3 0x2a 0x00000004\n"
                "  [writing] - %s 3 0x2a 0x00000004 0x04030201\n", app_name,app_name,app_name);
}

int main(int argc, char **argv)
{
    int opt;
    unsigned char buf[8],force = 0;
    unsigned int addr = 0, reg_address = 0, data_bytes = 0, bus_num = -1;

    while((opt = getopt(argc,argv,"fh")) != -1){
        switch(opt)
        {
            case 'f':
                force = 1;
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
                break;
            default:
                fprintf(stderr,"unknown option: %c\n",opt);
                usage(argv[0]);
                exit(-1);
        }
    }

    if (argc - optind < 3) { // at least 3 argument (bus,dev_addr,reg_addr)
        usage(argv[0]);
        exit(-1);
    }

    /* Get i2c bus number */
    if (sscanf(argv[optind], "%u", &bus_num) != 1) {
        fprintf(stderr, "Can't parse i2c 'bus_num' [%s]\n", argv[1]);
        exit(-1);
    }

    /* Get i2c device address */
    if (sscanf(argv[optind+1], "0x%x", &addr) != 1) {
        fprintf(stderr, "Can't parse i2c 'dev_addr' [%s]\n", argv[2]);
        exit(-1);
    }

    /* Get i2c internal address bytes */
    if (sscanf(argv[optind+2], "0x%x", &reg_address) != 1) {
        fprintf(stderr, "Can't parse i2c 'reg_address' [%s]\n", argv[3]);
        exit(-2);
    }
    buf[0] = (reg_address >> 24) & 0xFF;
    buf[1] = (reg_address >> 16) & 0xFF;
    buf[2] = (reg_address >> 8) & 0xFF;
    buf[3] = (reg_address) & 0xFF;

    /* Get i2c writing address */
    if( argc - optind > 3) {
        if (sscanf(argv[optind+3], "0x%x", &data_bytes) != 1) {
            fprintf(stderr, "Can't parse i2c 'data_bytes' [%s]\n", argv[4]);
            exit(-2);
        }
        buf[4] = (data_bytes >> 24) & 0xFF;
        buf[5] = (data_bytes >> 16) & 0xFF;
        buf[6] = (data_bytes >> 8) & 0xFF;
        buf[7] = (data_bytes) & 0xFF;
    }

    /* Open i2c bus */
    int fd;
    if ((fd = i2c_open(bus_num,addr,force)) == -1) {
        fprintf(stderr, "Open i2c bus:/dev/i2c-%d error!\n", bus_num);
        exit(-3);
    }

    if( argc - optind > 3 ){ // write
      /* Print before write */
      fprintf(stdout, "Reg Addr : ");
      print_i2c_data(&buf[0], 4);
      fprintf(stdout, "Write data: ");
      print_i2c_data(&buf[4], 4);

      if(i2c_rdwr_msg_transfer(fd,addr<<1,buf,8,NULL,0)!=0){
          fprintf(stderr, "/dev/i2c-%d transfer error!\n",bus_num);
          close(fd);
          exit(-3);
      }

    } else { //read
      fprintf(stdout, "Reg Addr : ");
      print_i2c_data(&buf[0], 4);

      if(i2c_rdwr_msg_transfer(fd,addr<<1,buf,4,&buf[4],4)!=0){
          fprintf(stderr, "/dev/i2c-%d transfer error!\n",bus_num);
          close(fd);
          exit(-3);
      }
      /* Print read result */
      fprintf(stdout, "Read data: ");
      print_i2c_data(&buf[4], 4);
    }

    close(fd);
    return 0;
}