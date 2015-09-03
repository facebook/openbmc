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

#include "intf.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/fib_rules.h>

#include "openbmc/log.h"
#include "libnetlink.h"
#include "ll_map.h"

struct oob_intf_t {
  char oi_name[32];
  int oi_fd;
  int oi_ifidx;
  uint8_t oi_mac[6];
  struct rtnl_handle oi_rth;
};

#define TUN_DEVICE "/dev/net/tun"

static int oob_intf_set_mac(oob_intf *intf, const uint8_t mac[6]) {
  int rc;
  struct {
    struct nlmsghdr n;
    struct ifinfomsg ifi;
    char buf[256];
  } req;

  memset(&req, 0, sizeof(req));
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_type = RTM_NEWLINK;
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.ifi.ifi_family = AF_UNSPEC;
  req.ifi.ifi_index = intf->oi_ifidx;
  memcpy(intf->oi_mac, mac, sizeof(intf->oi_mac));
  addattr_l(&req.n, sizeof(req), IFLA_ADDRESS,
            intf->oi_mac, sizeof(intf->oi_mac));
  rc = rtnl_talk(&intf->oi_rth, &req.n, 0, 0, NULL);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to set mac to interface %s @ index %d",
            intf->oi_name, intf->oi_ifidx);
    return -rc;
  }

  LOG_INFO("Set interface %s @ index %d mac to %x:%x:%x:%x:%x:%x",
           intf->oi_name, intf->oi_ifidx,
           intf->oi_mac[0], intf->oi_mac[1], intf->oi_mac[2],
           intf->oi_mac[3], intf->oi_mac[4], intf->oi_mac[5]);

  return 0;
}

static int oob_intf_bring_up(oob_intf *intf) {
  int rc;
  struct {
    struct nlmsghdr n;
    struct ifinfomsg ifi;
    char buf[256];
  } req;

  memset(&req, 0, sizeof(req));
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_type = RTM_NEWLINK;
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.ifi.ifi_family = AF_UNSPEC;
  req.ifi.ifi_change |= IFF_UP;
  req.ifi.ifi_flags |= IFF_UP;
  req.ifi.ifi_index = intf->oi_ifidx;
  rc = rtnl_talk(&intf->oi_rth, &req.n, 0, 0, NULL);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to bring up interface %s @ index %d",
            intf->oi_name, intf->oi_ifidx);
    return -rc;
  }

  LOG_INFO("Brought up interface %s @ index %d", intf->oi_name, intf->oi_ifidx);

  return 0;
}

oob_intf* oob_intf_create(const char *name, const uint8_t mac[6]) {

  int rc;
  int flags;
  struct ifreq ifr;
  oob_intf *intf = NULL;

#define _CHECK_RC(fmt, ...) do {                \
  if (rc < 0) {                                 \
    rc = errno;                                 \
    LOG_ERR(rc, fmt, ##__VA_ARGS__);            \
    goto err_out;                               \
  }                                             \
} while(0)

  intf = malloc(sizeof(*intf));
  if (!intf) {
    rc = ENOMEM;
    LOG_ERR(rc, "Failed to allocate memory for interface");
    goto err_out;
  }
  memset(intf, 0, sizeof(*intf));
  strncpy(intf->oi_name, name, sizeof(intf->oi_name));
  intf->oi_name[sizeof(intf->oi_name) - 1] = '\0';
  intf->oi_fd = -1;

  rc = rtnl_open(&intf->oi_rth, 0);
  _CHECK_RC("Failed to open rth_handler");

  rc = open(TUN_DEVICE, O_RDWR);
  _CHECK_RC("Failed to open %s", TUN_DEVICE);
  intf->oi_fd = rc;

  memset(&ifr, 0, sizeof(ifr));
  /*
   * IFF_TAP: TAP interface
   * IFF_NO_PI: Do not provide pracket information
   */
  ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
  strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
  ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

  rc = ioctl(intf->oi_fd, TUNSETIFF, (void *) &ifr);
  _CHECK_RC("Failed to create tap interface %s", ifr.ifr_name);

  /* make fd non-blocking */
  rc = fcntl(intf->oi_fd, F_GETFL);
  _CHECK_RC("Failed to get flags from fd ", intf->oi_fd);
  flags = rc | O_NONBLOCK;
  rc = fcntl(intf->oi_fd, F_SETFL, rc);
  _CHECK_RC("Failed to set non-blocking flags ", flags,
            " to fd ", intf->oi_fd);

  /* set CLOEXEC */
  rc = fcntl(intf->oi_fd, F_GETFD);
  _CHECK_RC("Failed to get flags from fd ", intf->oi_fd);
  flags = rc | FD_CLOEXEC;
  rc = fcntl(intf->oi_fd, F_SETFD, flags);
  _CHECK_RC("Failed to set close-on-exec flags ", flags,
            " to fd ", intf->oi_fd);

  // TODO: if needed, we can adjust send buffer size, TUNSETSNDBUF
  intf->oi_ifidx = ll_name_to_index(intf->oi_name);

  /* now set the mac address */
  oob_intf_set_mac(intf, mac);

#if 0
  /* make it persistent */
  rc = ioctl(intf->oi_fd, TUNSETPERSIST, 0);
  _CHECK_RC("Failed to make the tap interface %s persistent", intf->oi_name);
#endif

  LOG_INFO("Create/attach to tap interface %s @ fd %d, index %d",
           intf->oi_name, intf->oi_fd, intf->oi_ifidx);

  //oob_intf_bring_up(intf);

  return intf;

 err_out:
  if (intf) {
    rtnl_close(&intf->oi_rth);
    if (intf->oi_fd != -1) {
      close(intf->oi_fd);
    }
    free(intf);
  }

  return NULL;
}

int oob_intf_get_fd(const oob_intf *intf) {
  return intf->oi_fd;
}

int oob_intf_receive(const oob_intf *intf, char *buf, int len) {
  int rc;
  do {
    rc = read(intf->oi_fd, buf, len);
  } while (rc == -1 && errno == EINTR);
  if (rc < 0) {
    rc = errno;
    if (rc != EAGAIN) {
      LOG_ERR(rc, "Failed to read on interface fd %d", intf->oi_fd);
      return -rc;
    } else {
      /* nothing is available */
      return 0;
    }
  } else if (rc == 0) {
    // Nothing to read. It shall not happen as the fd is non-blocking.
    // Just add this case to be safe.
    return 0;
  } else if (rc > len) {
    // The pkt is larger than the buffer. We don't have complete packet.
    // It shall not happen unless the MTU is mis-match. Drop the packet.
    LOG_ERR(ENOSPC, "Received a too large packet (%d bytes > %d) from the "
            "tap interface. Drop it...", rc, len);
    return -ENOSPC;
  } else {
    LOG_VER("Recv a packet of %d bytes from %s", rc, intf->oi_name);
    return rc;
  }
}

int oob_intf_send(const oob_intf *intf, const char *buf, int len) {
  int rc;
  do {
    rc = write(intf->oi_fd, buf, len);
  } while (rc == -1 && errno == EINTR);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to send on interface fd %d", intf->oi_fd);
    return -rc;
  } else if (rc < len) {
    LOG_ERR(EIO, "Failed to send the full packet (%d bytes > %d) for fd %d",
            len, rc, intf->oi_fd);
    return -EIO;
  } else {
    LOG_VER("Sent a packet of %d bytes to %s", rc, intf->oi_name);
    return rc;
  }
}
