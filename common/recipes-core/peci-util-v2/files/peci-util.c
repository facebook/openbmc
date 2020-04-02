/*
 * peci-util
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openbmc/peci.h>

#define MAX_ARG_NUM 64

int __attribute__((weak)) pal_before_peci(void) { return 0; }
int __attribute__((weak)) pal_after_peci(void) { return 0; }

static int process_file(int fd, char *file_path);

static uint8_t retry_times = 0;
static uint32_t retry_interval = 250;
static uint8_t verbose = 0;

static void
print_usage_help(void) {
  printf("Usage: peci-util <client addr> <Tx length> <Rx length> <[0..n]data_bytes_to_send>\n");
  printf("       peci-util -f <peci command file>\n\n");
  printf("       client addr: 0x30 CPU0, 0x31 CPU1\n");
  printf("       --retry, -r <retry times>\n");
  printf("         Retry when CC is 0x80 or 0x81. Default is 0\n");
  printf("       --interval, -i <interval>\n");
  printf("         Interval between retry, unit is ms. Default is 250\n");
  printf("       --file, -f <file>\n");
  printf("         Read commands from <file>\n");
  printf("       --verbose, -v\n");
  printf("         Display verbose information\n");
  printf("       --help, -h\n");
  printf("         Display this help messages\n");
}

static int
process_command(int fd, int argc, char **argv) {
  struct peci_xfer_msg msg;
  uint8_t tlen = 0;
  char file_path[256];
  int i, opt, retry;
  int optind_long = 0;
  static const char* optstring = "r:i:f:vh";
  static const struct option long_options[] = {
    {"retry", required_argument, 0, 'r'},
    {"interval", required_argument, 0, 'i'},
    {"file", required_argument, 0, 'f'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };

  optind = 0;  // reset getopt function
  while ((opt = getopt_long(argc, argv, optstring, long_options, &optind_long)) != -1) {
    switch (opt) {
    case 'r':
      retry_times = strtoul(optarg, NULL, 0);
      break;
    case 'i':
      retry_interval = strtoul(optarg, NULL, 0);
      break;
    case 'f':
      strncpy(file_path, optarg, sizeof(file_path) - 1);
      return process_file(fd, file_path);
    case 'v':
      verbose = 1;
      break;
    case 'h':
      print_usage_help();
      return 0;
    default:
      printf("Unknown option\n");
      goto err_exit;
    }
  }

  // fill peci_xfer_msg
  if (argc - optind < 3) {
    goto err_exit;
  }
  i = optind;
  msg.addr = (uint8_t)strtoul(argv[i++], NULL, 0);
  msg.tx_len = (uint8_t)strtoul(argv[i++], NULL, 0);
  msg.rx_len = (uint8_t)strtoul(argv[i++], NULL, 0);
  if ((argc - i) != msg.tx_len || (msg.tx_len > PECI_BUFFER_SIZE) || (msg.rx_len > PECI_BUFFER_SIZE)) {
    goto err_exit;
  }
  while (i < argc) {
    msg.tx_buf[tlen++] = (uint8_t)strtoul(argv[i++], NULL, 0);
  }

  if (verbose) {
    printf("addr:%d,", msg.addr);
    printf("tx(%d):", msg.tx_len);
    for (i = 0; i < msg.tx_len; i++) {
      printf("%02X ", msg.tx_buf[i]);
    }
    printf("rx(%d)", msg.rx_len);
    printf("\n");
  }

  retry = 0;
  do {
    if (peci_cmd_xfer_fd(fd, &msg) < 0) {
      perror("peci_cmd_xfer_fd");
      return -1;
    }

    if (retry_times) {
      switch (msg.rx_buf[0]) {
        case 0x80:  // response timeout, Data not ready
        case 0x81:  // response timeout, not able to allocate resource
          if (retry < retry_times) {
            if (verbose) {
              printf("CC: %02Xh, retry...\n", msg.rx_buf[0]);
            }
            retry++;
            usleep(retry_interval*1000);
            continue;
          }
          break;
        case 0x40:  // command passed
        case 0x82:  // low power state
        case 0x90:  // unknown/invalid/illegal request
        case 0x91:  // error
        default:
          break;
      }
    }
    break;
  } while (retry <= retry_times);

  for (i = 0; i < msg.rx_len; i++) {
    printf("%02X ", msg.rx_buf[i]);
  }
  printf("\n");

  return 0;

err_exit:
  printf("wrong arg: ");
  for (i = 0; i < argc; i++) {
    printf("%s ", argv[i]);
  }
  printf("\n");

  print_usage_help();
  return -1;
}

static int
process_file(int fd, char *path) {
  FILE *fp;
  int argc, ret, final_ret = 0;
  char buf[1024];
  char *str, *next, *del=" \n";
  char *argv[MAX_ARG_NUM];

  if (!(fp = fopen(path, "r"))) {
    fprintf(stderr, "Failed to open %s\n", path);
    return -1;
  }

  argv[0] = path;
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    // getopt parse arguments from argv[1], we have to fill arg from 1
    str = strtok_r(buf, del, &next);
    for (argc = 1; argc < MAX_ARG_NUM && str; argc++, str = strtok_r(NULL, del, &next)) {
      if (str[0] == '#')
        break;

      if ((argc == 1) && !strcmp(str, "echo")) {
        printf("%s", (*next) ? next : "\n");
        break;
      }
      argv[argc] = str;
    }
    if (argc == 1)
      continue;

    ret = process_command(fd, argc, argv);
    if (ret) {  // return failure if any command failed
      final_ret = ret;
    }
  }
  fclose(fp);

  return final_ret;
}

int
main(int argc, char **argv) {
  int fd, ret;

  pal_before_peci();

  if ((fd = open(PECI_DEVICE, O_RDWR)) < 0) {
    fprintf(stderr, "Failed to open %s\n", PECI_DEVICE);
    return -1;
  }
  ret = process_command(fd, argc, argv);
  close(fd);

  pal_after_peci();

  return ret;
}
