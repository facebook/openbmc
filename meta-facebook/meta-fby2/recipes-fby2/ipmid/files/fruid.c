/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file provides platform specific implementation of FRUID information
 *
 * FRUID specification can be found at
 * www.intel.com/content/dam/www/public/us/en/documents/product-briefs/platform-management-fru-document-rev-1-2-feb-2013.pdf
 *
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
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <arpa/inet.h>
#include "fruid.h"
#include <facebook/fby2_common.h>
#include <facebook/fby2_fruid.h>
#include <openbmc/pal.h>
#include <openbmc/ncsi.h>

#define EEPROM_DC       "/sys/class/i2c-adapter/i2c-%d/%d-0051/eeprom"
#define EEPROM_SPB      "/sys/class/i2c-adapter/i2c-8/8-0051/eeprom"

#define BIN_SPB         "/tmp/fruid_spb.bin"
#define BIN_NIC         "/tmp/fruid_nic.bin"
#define BIN_SLOT        "/tmp/fruid_slot%d.bin"

#define FRUID_SIZE        256


/*
 * copy_eeprom_to_bin - copy the eeprom to binary file im /tmp directory
 *
 * @eeprom_file   : path for the eeprom of the device
 * @bin_file      : path for the binary file
 *
 * returns 0 on successful copy
 * returns non-zero on file operation errors
 */
static int copy_eeprom_to_bin(const char * eeprom_file, const char * bin_file) {

  int eeprom;
  int bin;
  uint64_t tmp[FRUID_SIZE];
  ssize_t bytes_rd, bytes_wr;

  errno = 0;

  if (access(eeprom_file, F_OK) != -1) {

    eeprom = open(eeprom_file, O_RDONLY);
    if (eeprom == -1) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: unable to open the %s file: %s",
          eeprom_file, strerror(errno));
      return errno;
    }

    bin = open(bin_file, O_WRONLY | O_CREAT, 0644);
    if (bin == -1) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: unable to create %s file: %s",
          bin_file, strerror(errno));
      return errno;
    }

    bytes_rd = read(eeprom, tmp, FRUID_SIZE);
    if (bytes_rd != FRUID_SIZE) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: write to %s file failed: %s",
          eeprom_file, strerror(errno));
      return errno;
    }

    bytes_wr = write(bin, tmp, bytes_rd);
    if (bytes_wr != bytes_rd) {
      syslog(LOG_ERR, "copy_eeprom_to_bin: write to %s file failed: %s",
          bin_file, strerror(errno));
      return errno;
    }

    close(bin);
    close(eeprom);
  }

  return 0;
}

static int get_ncsi_vid(void) {
  int sock_fd, ret = 0;
  int req_msg_size = offsetof(NCSI_NL_MSG_T, msg_payload);
  int msg_size = sizeof(NCSI_NL_RSP_T);
  struct sockaddr_nl src_addr, dest_addr;
  struct nlmsghdr *nlh = NULL;
  struct iovec iov;
  struct msghdr msg;
  NCSI_NL_MSG_T *nl_msg;
  NCSI_NL_RSP_T *rcv_buf;
  NCSI_Response_Packet *resp;

  /* open NETLINK socket to send message to kernel */
  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if (sock_fd < 0)  {
    syslog(LOG_ERR, "get_ncsi_vid: failed to allocate socket");
    return -1;
  }

  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid();
  if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) == -1)
  {
    syslog(LOG_ERR, "get_ncsi_vid: failed to bind socket, errno=0x%x", errno);
    ret = -1;
    goto close_and_exit;
  }

  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; /* For Linux Kernel */
  dest_addr.nl_groups = 0; /* unicast */

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msg_size));
  if (!nlh) {
    syslog(LOG_ERR, "get_ncsi_vid: failed to allocate message buffer");
    ret = -1;
    goto close_and_exit;
  }
  memset(nlh, 0, NLMSG_SPACE(msg_size));
  nlh->nlmsg_len = NLMSG_SPACE(req_msg_size);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  nl_msg = (NCSI_NL_MSG_T *)NLMSG_DATA(nlh);
  sprintf(nl_msg->dev_name, "eth0");
  nl_msg->channel_id = 0;
  nl_msg->cmd = NCSI_GET_VERSION_ID;
  nl_msg->payload_length = 0;

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;

  memset(&msg, 0, sizeof(msg));
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  ret = sendmsg(sock_fd, &msg, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "get_ncsi_vid: failed to send msg, errno=%d", errno);
    goto close_and_exit;
  }

  /* Read message from kernel */
  iov.iov_len = NLMSG_SPACE(msg_size);
  ret = recvmsg(sock_fd, &msg, 0);
  if (ret < 0) {
    syslog(LOG_ERR, "get_ncsi_vid: failed to receive msg, errno=%d", errno);
    goto close_and_exit;
  }
  rcv_buf = (NCSI_NL_RSP_T *)NLMSG_DATA(nlh);
  resp = (NCSI_Response_Packet *)rcv_buf->msg_payload;
  if (ntohs(resp->Response_Code) == RESP_COMMAND_COMPLETED) {
    if (access("/tmp/cache_store", F_OK) == -1) {
      mkdir("/tmp/cache_store", 0777);
    }
    handle_get_version_id(resp);
  }

close_and_exit:
  if (nlh) {
    free(nlh);
  }
  close(sock_fd);

  return ret;
}

int plat_fruid_init(void) {

  int ret = -1;
  int fru=0;
  char path[128] = {0};
  char fpath[64] = {0};

  for (fru = 1; fru <= MAX_NUM_FRUS; fru++) {

    switch(fru) {
      case FRU_SLOT1:
      case FRU_SLOT2:
      case FRU_SLOT3:
      case FRU_SLOT4:
        switch(fby2_get_slot_type(fru))
        {
           case SLOT_TYPE_SERVER:
           case SLOT_TYPE_GPV2:
             // Do not access EEPROM
             break;
           case SLOT_TYPE_CF:
           case SLOT_TYPE_GP:
             sprintf(path, EEPROM_DC, plat_get_ipmb_bus_id(fru), plat_get_ipmb_bus_id(fru));
             sprintf(fpath, BIN_SLOT, fru);
             ret = copy_eeprom_to_bin(path, fpath);
             break;
           case SLOT_TYPE_NULL:
             // Do not access EEPROM
             break;
        }
        break;
      case FRU_SPB:
        ret = copy_eeprom_to_bin(EEPROM_SPB, BIN_SPB);
        break;
      case FRU_NIC:
        get_ncsi_vid();
        ret = pal_read_nic_fruid(BIN_NIC, 256);
        break;
      default:
        break;
    }
  }

  return ret;
}

int plat_fruid_size(unsigned char payload_id) {
  char fpath[64] = {0};
  struct stat buf;
  int ret;

  // Fill the file path for a given slot
  sprintf(fpath, BIN_SLOT, payload_id);

  // check the size of the file and return size
  ret = stat(fpath, &buf);
  if (ret) {
    return 0;
  }

  return buf.st_size;
}

int plat_fruid_data(unsigned char payload_id, int offset, int count, unsigned char *data) {
  char fpath[64] = {0};
  int fd;
  int ret;

  // Fill the file path for a given slot
  sprintf(fpath, BIN_SLOT, payload_id);

  // open file for read purpose
  fd = open(fpath, O_RDONLY);
  if (fd < 0) {
    return fd;
  }

  // seek position based on given offset
  ret = lseek(fd, offset, SEEK_SET);
  if (ret < 0) {
    close(fd);
    return ret;
  }

  // read the file content
  ret = read(fd, data, count);
  if (ret != count) {
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}
