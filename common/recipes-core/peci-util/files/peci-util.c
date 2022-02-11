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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <openbmc/pal.h>

/***********************************************************************/
struct timing_negotiation {
  uint8_t msg_timing;
  uint8_t addr_timing;
};

struct xfer_msg {
  uint8_t client_addr;
  uint8_t tx_len;
  uint8_t rx_len;
  uint8_t tx_fcs;
  uint8_t rx_fcs;
  uint8_t fcs_en;
  uint8_t sw_fcs;
  uint8_t *tx_buf;
  uint8_t *rx_buf;
  uint32_t sts;
};

#define PECI_DEVICE "/dev/ast-peci"

//IOCTL ..
#define PECIIOC_BASE 'P'

#define AST_PECI_IOCRTIMING _IOR(PECIIOC_BASE, 0, struct timing_negotiation*)
#define AST_PECI_IOCWTIMING _IOW(PECIIOC_BASE, 1, struct timing_negotiation*)
#define AST_PECI_IOCXFER _IOWR(PECIIOC_BASE, 2, struct xfer_msg*)

#define PECI_INT_TIMEOUT     (0x1 << 4)
#define PECI_INT_CONNECT     (0x1 << 3)
#define PECI_INT_W_FCS_BAD   (0x1 << 2)
#define PECI_INT_W_FCS_ABORT (0x1 << 1)
#define PECI_INT_CMD_DONE    (0x1)

/***********************************************************************/

#if defined(__GNUC__)
int pal_before_peci(void) __attribute__((weak));
int pal_after_peci(void) __attribute__((weak));
#endif
static int process_file (int peci_fd, char* file_path);
static int process_command (int peci_fd, int argc, char **argv);

static uint8_t awfcs_en = 0;
static uint8_t retry_times = 3;
static uint8_t retry_interval = 250;
static uint8_t verbose = 0;

static void
print_usage_help(void) {
  printf("Usage: peci-util <client addr> <Tx length> <Rx length> <[0..n]data_bytes_to_send>\n");
  printf("       peci-util -f <peci command file>\n");
  printf("  client addr: 0x30 CPU0, 0x31 CPU1\n");
  printf("  --awfcs, -a <1|0>\n");
  printf("    Enable/Disable AW FCS, default is 0\n");
  printf("  --retry, -r <retry times>\n");
  printf("    Retry times if CC is 0x80 or 0x81. Default is 3\n");
  printf("  --interval, -i <interval>\n");
  printf("    Interval between retry, unit is ms. Default is 250\n");
  printf("  --verbose, -v\n");
  printf("    Display verbose information\n");
  printf("  --help, -h\n");
  printf("    Display this help messages\n");
  printf("  --file, -f <file>\n");
  printf("    Read commands from <file>\n");
}

#define MAX_ARG_NUM 64
static int
process_file (int peci_fd, char* path) {
  FILE *fp;
  int argc, final_ret=0, ret;
  char buf[1024];
  char *str, *next, *del=" \n";
  char *argv[MAX_ARG_NUM];

  if (!(fp = fopen(path, "r"))) {
    syslog(LOG_WARNING, "Failed to open %s", path);
    return -1;
  }

  argv[0] = path;
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    // getopt parse arguments from argv[1], we have to fill arg from 1.
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

    ret = process_command(peci_fd, argc, argv);
    // return failure if any command failed
    if (ret)
      final_ret = ret;
  }
  fclose(fp);

  return final_ret;
}

static int
process_command (int peci_fd, int argc, char **argv) {
  struct xfer_msg msg;
  uint8_t tbuf[32] = {0x00};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  int i, opt, retry;
  char file_path[256];
  int optind_long = 0;

  static const char* optstring = "a:r:i:f:vhe";
  static const struct option long_options[] = {
    {"awfcs", required_argument, 0, 'a'},
    {"retry", required_argument, 0, 'r'},
    {"interval", required_argument, 0, 'i'},
    {"file", required_argument, 0, 'f'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {"echo", no_argument, 0, 'e'},
    {0, 0, 0, 0}
  };

  msg.tx_buf = tbuf;
  msg.rx_buf = rbuf;

  optind = 0; // Reset getopt function
  while ((opt = getopt_long(argc, argv, optstring, long_options, &optind_long)) != -1) {
    switch (opt) {
    case 'a':
      awfcs_en = strtoul(optarg, NULL, 0) & 0x1;
      break;
    case 'r':
      retry_times = strtoul(optarg, NULL, 0);
      break;
    case 'i':
      retry_interval = strtoul(optarg, NULL, 0);
      break;
    case 'f':
      strncpy(file_path, optarg, 256);
      return process_file(peci_fd, file_path);
    case 'v':
      verbose = 1;
      break;
    case 'h':
      print_usage_help();
      return 0;
    case 'e':
      for (i=optind; i<argc; i++)
        printf("%s ", argv[i]);
      printf("\n");
      return 0;
    default:
      printf("Unknown option.\n");
      goto err_exit;
    }
  }

  // fill xfer_msg
  if (argc - optind < 3) {
    goto err_exit;
  }
  i = optind;
  msg.client_addr = (uint8_t)strtoul(argv[i++], NULL, 0);
  msg.tx_len = (uint8_t)strtoul(argv[i++], NULL, 0);
  msg.rx_len = (uint8_t)strtoul(argv[i++], NULL, 0);
  msg.fcs_en = awfcs_en;
  if (argc - i != msg.tx_len || msg.tx_len > 32 || msg.rx_len > 32) {
    goto err_exit;
  }
  while (i < argc) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i++], NULL, 0);
  }

  if (verbose) {
    printf("awfcs:%d,addr:%d,", msg.fcs_en, msg.client_addr);
    printf("tx(%d):", msg.tx_len);
    for (i=0; i<msg.tx_len; i++)
      printf("%02X ", tbuf[i]);
    printf("rx(%d)", msg.rx_len);
    printf("\n");
  }

  retry = 0;
  do {
    ioctl(peci_fd, AST_PECI_IOCXFER, &msg);

    switch (msg.sts) {
      case PECI_INT_CMD_DONE:
        break;
      default:
        fprintf(stderr, "PECI failed. sts:%08Xh\n", msg.sts);
        return -1;
    }

    if (retry_times) {
      switch (msg.rx_buf[0]) {
        case 0x80: // Response timeout, Data not ready
        case 0x81: // Response timeout, not able to allocate resource
          if (verbose) {
            printf("CC: %02Xh, retry...\n", msg.rx_buf[0]);
          }
          if (retry < retry_times) {
            retry++;
            usleep(retry_interval*1000);
            continue;
          }
          break;
        case 0x40: // Command Passed
        case 0x82: // Low power state
        case 0x90: // Unknown/Invalid/Illegal Request
        case 0x91: // Error
        default:
          break;
      }
    }
    break;
  } while (retry < retry_times);

  if (verbose && retry_times) {
    if (retry == retry_times)
      printf("retry count is full.\n");
  }

  // Print the response
  for (i = 0; i < msg.rx_len; i++) {
    printf("%02X ", msg.rx_buf[i]);
  }
  printf("\n");

  return 0;
err_exit:
  printf("wrong arg:");
  for (i=0; i<argc; i++)
    printf("%s ", argv[i]);
  printf("\n");
  print_usage_help();
  return -1;
}

int
main(int argc, char **argv) {
  int peci_fd = 0;
  int timeout = 5;
  int ret = -1;

  if (pal_before_peci != NULL)
    pal_before_peci();

  while ((peci_fd = open(PECI_DEVICE, O_RDWR)) < 0) {
    if (timeout-- < 0) {
      fprintf(stderr, "Failed to open %s\n", PECI_DEVICE);
      return -1;
    }
    usleep(10*1000);
  }

  ret = process_command(peci_fd, argc, argv);

  if (peci_fd)
    close(peci_fd);
  if (pal_after_peci != NULL)
    pal_after_peci();
  return ret;
}
