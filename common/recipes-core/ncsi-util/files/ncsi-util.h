/*
 * ncsi-util.h
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#ifndef _NCSI_UTIL_H_
#define _NCSI_UTIL_H_

#include <openbmc/ncsi.h>

#define noDEBUG   /* debug printfs */

/*
 * ncsi-util common command arguments
 */
typedef struct {
  char  *dev_name;            /* If not null, user specified netdev */
  int   channel_id;           /* NC-SI channel */
} ncsi_util_args_t;

#define NCSI_UTIL_DEFAULT_DEV_NAME    "eth0"

/*
 * Command vendor
 */
enum {
  NCSI_UTIL_VENDOR_DMTF = 0,  /* DMTF defined command */
  NCSI_UTIL_VENDOR_BRCM,      /* Broadcom OEM command */
  NCSI_UTIL_VENDOR_MAX,
};

/*
 * Vendor specific data and functions
 */
typedef struct {
  const char *name;     /* Vendor name used in command line -o option */

  /* Util command handler function */
  int (*handler)(int argc, char **argv, ncsi_util_args_t *util_args);

} ncsi_util_vendor_t;


// Common command line options
#define NCSI_UTIL_COMMON_OPT_STR    "n:c:l:m:"

// Common getopt characters for switch-case
#define NCSI_UTIL_GETOPT_COMMON_OPT_CHARS \
       'n': \
  case 'c': \
  case 'l': \
  case 'm'


/* NC-SI netlink send function */
extern NCSI_NL_RSP_T * (*send_nl_msg)(NCSI_NL_MSG_T *nl_msg);

void ncsi_util_common_usage(void);
void ncsi_util_set_pkt_dev(NCSI_NL_MSG_T *nl_msg,
                           const ncsi_util_args_t *util_args,
                           const char *dflt_dev_name);
void ncsi_util_init_pkt_by_args(NCSI_NL_MSG_T *nl_msg,
                               const ncsi_util_args_t *util_args);
unsigned long ncsi_util_print_progress(unsigned long cur, unsigned long total,
                                       unsigned long last_percent,
                                       const char *prefix_str);
#endif /* _NCSI_UTIL_H_ */
