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
#include "nic.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <openbmc/obmc-i2c.h>
#include "openbmc/log.h"

#define ETHERTYPE_LLDP 0x88cc

/* Addendum support for large data blocks */
#define I2C_SMBUS_BLOCK_LARGE_MAX       240 /* Large data transfer */
/* SMBus transaction types (size parameter in the above function) addendum
 *  * support for large data (XXX Unused?) */
#define I2C_SMBUS_BLOCK_LARGE_DATA      9

#define _I2C_MIN(a, b) (((a) <= (b)) ? (a) : (b))

/*
* Data for Large SMBus Messages
*/
union i2c_smbus_large_data {
 union i2c_smbus_data data;
 __u8 block[I2C_SMBUS_BLOCK_LARGE_MAX + 2]; /* block[0] is used for length */
                                            /* and one more for PEC */
};

struct oob_nic_t {
  int on_bus;
  uint8_t on_addr;
  int on_file;                  /* the file descriptor */
  uint8_t on_mac[6];            /* the mac address assigned to this NIC */
};

static inline __s32 i2c_smbus_read_block_large_data(int file, __u8 command,
                                                    __u8 *values)
{
  union i2c_smbus_large_data data;
  if (i2c_smbus_access(file, I2C_SMBUS_READ, command,
                       I2C_SMBUS_BLOCK_LARGE_DATA,
                       (union i2c_smbus_data *)&data)) {
    return -1;
  } else {
    /* the first byte is the length which is not copied */
    memcpy(values, &data.block[1], _I2C_MIN(data.block[0], I2C_SMBUS_BLOCK_LARGE_MAX));
    return data.block[0];
  }
}

static inline __s32 i2c_smbus_write_block_large_data(int file, __u8 command,
                                                     __u8 length,
                                                     const __u8 *values)
{
  union i2c_smbus_large_data data;
  if (length > I2C_SMBUS_BLOCK_LARGE_MAX) {
    length = I2C_SMBUS_BLOCK_LARGE_MAX;
  }
  data.block[0] = length;
  memcpy(&data.block[1], values, length);
  return i2c_smbus_access(file, I2C_SMBUS_WRITE, command,
                          I2C_SMBUS_BLOCK_LARGE_DATA,
                          (union i2c_smbus_data *)&data);
}

oob_nic* oob_nic_open(int bus, uint8_t addr) {
  oob_nic *dev = NULL;
  char fn[32];
  int rc;

  /* address must be 7 bits maximum */
  if ((addr & 0x80)) {
    LOG_ERR(EINVAL, "Address 0x%x has the 8th bit", addr);
    return NULL;
  }

  dev = calloc(1, sizeof(*dev));
  if (!dev) {
    return NULL;
  }
  dev->on_bus = bus;
  dev->on_addr = addr;

  /* construct the device file name */
  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  dev->on_file = open(fn, O_RDWR);
  if (dev->on_file == -1) {
    LOG_ERR(errno, "Failed to open i2c device %s", fn);
    goto err_out;
  }

  /* assign the device address */
  rc = ioctl(dev->on_file, I2C_SLAVE, dev->on_addr);
  if (rc < 0) {
    LOG_ERR(errno, "Failed to open slave @ address 0x%x", dev->on_addr);
    goto err_out;
  }

  return dev;

 err_out:
  oob_nic_close(dev);
  return NULL;
}

void oob_nic_close(oob_nic *dev) {
  if (!dev) {
    return;
  }
  if (dev->on_file != -1) {
    close(dev->on_file);
  }
  free(dev);
}

int oob_nic_get_mac(oob_nic *dev, uint8_t mac[6]) {
  int rc;
  uint8_t buf[64];

  rc = i2c_smbus_read_block_data(dev->on_file, NIC_READ_MAC_CMD, buf);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to get MAC on %d-%x",
            dev->on_bus, dev->on_addr);
    return -rc;
  }

  if (rc != NIC_READ_MAC_RES_LEN) {
    LOG_ERR(EFAULT, "Unexpected response len (%d) for get MAC on %d-%x",
            rc, dev->on_bus, dev->on_addr);
    return -EFAULT;
  }

  if (buf[0] != NIC_READ_MAC_RES_OPT) {
    LOG_ERR(EFAULT, "Unexpected response opt code (0x%x) get MAC on %d-%x",
            buf[0], dev->on_bus, dev->on_addr);
    return -EFAULT;
  }

  memcpy(mac, &buf[1], 6);

  LOG_DBG("Get MAC on %d-%x: %x:%x:%x:%x:%x:%x", dev->on_bus, dev->on_addr,
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return 0;
}

int oob_nic_get_status(oob_nic *dev, oob_nic_status *status) {
  int rc;
  uint8_t buf[64];

  rc = i2c_smbus_read_block_data(dev->on_file, NIC_READ_STATUS_CMD, buf);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to get status on %d-%x",
            dev->on_bus, dev->on_addr);
    return -rc;
  }

  if (rc != NIC_READ_STATUS_RES_LEN) {
    LOG_ERR(EFAULT, "Unexpected response len (%d) for get status on %d-%x",
            rc, dev->on_bus, dev->on_addr);
    return -EFAULT;
  }

  if (buf[0] != NIC_READ_STATUS_RES_OPT) {
    LOG_ERR(EFAULT, "Unexpected response opt code (0x%x) get status on %d-%x",
            buf[0], dev->on_bus, dev->on_addr);
    return -EFAULT;
  }

  memset(status, 0, sizeof(*status));
  status->ons_byte1 = buf[1];
  status->ons_byte2 = buf[2];

  LOG_VER("Get status on %d-%x: byte1:0x%x byte2:0x%x",
          dev->on_bus, dev->on_addr,
          status->ons_byte1, status->ons_byte2);
  return 0;
}

int oob_nic_receive(oob_nic *dev, uint8_t *buf, int len) {

  int rc = 0;
  uint8_t pkt[I2C_SMBUS_BLOCK_LARGE_MAX];
  uint8_t opt;
  int copied = 0;
  int to_copy;
  int expect_first = 1;
  int n_frags = 0;

#define _COPY_DATA(n, data) do {                    \
  int to_copy;                                      \
  if (copied >= len) {                              \
    break;                                          \
  }                                                 \
  to_copy = (n < len - copied) ? n : len - copied;  \
  if (to_copy) {                                    \
    memcpy(buf + copied, data, to_copy);            \
  }                                                 \
  copied += to_copy;                                \
} while(0)

  do {
    rc = i2c_smbus_read_block_large_data(dev->on_file, NIC_READ_PKT_CMD, pkt);
    if (rc < 0) {
      rc = errno;
      LOG_ERR(rc, "Failed to get packet on %d-%x",
              dev->on_bus, dev->on_addr);
      goto err_out;
    }
    if (rc > I2C_SMBUS_BLOCK_LARGE_MAX) {
      LOG_ERR(EFAULT, "Too large i2c block (%d) received on %d-%x",
              rc, dev->on_bus, dev->on_addr);
      rc = EFAULT;
      goto err_out;
    }
    opt = pkt[0];
    switch (opt) {
    case NIC_READ_PKT_RES_FIRST_OPT:
      if (!expect_first) {
        rc = EFAULT;
        LOG_ERR(rc, "Received more than one buffer with FIRST set");
        goto err_out;
      }
      expect_first = 0;
      n_frags++;
      _COPY_DATA(rc - 1, &pkt[1]);
      break;
    case NIC_READ_PKT_RES_MIDDLE_OPT:
      if (expect_first) {
        rc = EFAULT;
        LOG_ERR(rc, "Received MIDDLE before getting FIRST");
        goto err_out;
      }
      _COPY_DATA(rc - 1, &pkt[1]);
      n_frags++;
      break;
    case NIC_READ_PKT_RES_LAST_OPT:
      if (expect_first) {
        rc = EFAULT;
        LOG_ERR(rc, "Received LAST before getting FIRST");
        goto err_out;
      }
      if (rc != NIC_READ_PKT_RES_LAST_LEN) {
        LOG_ERR(EFAULT, "Expect %d bytes (got %d) for LAST segement",
                NIC_READ_PKT_RES_LAST_LEN, rc);
        rc = EFAULT;
        goto err_out;
      }
      /* TODO: pkt status???? */
      break;
    case NIC_READ_STATUS_RES_OPT:
      /* that means no pkt available */
      if (!expect_first) {
        rc = EFAULT;
        LOG_ERR(rc, "Received STATUS in the middle of packet");
        goto err_out;
      }
      //LOG_VER("Received STATUS when receiving the packet");
      return 0;
    default:
      rc = EFAULT;
      LOG_ERR(rc, "Unexpected opt code 0x%x", opt);
      goto err_out;
    }
  } while (opt != NIC_READ_PKT_RES_LAST_OPT);

  LOG_VER("Received a packet with %d bytes in %d fragments", copied, n_frags);
  return copied;

 err_out:
  return -rc;
#undef _COPY_DATA
}

int oob_nic_send(oob_nic *dev, const uint8_t *data, int len) {

  int rc;
  uint8_t to_send;
  int has_sent = 0;
  int is_first = 1;
  uint8_t cmd;
  int n_frags = 0;

  if (len <= 0 || len > NIC_PKT_SIZE_MAX) {
    rc = EINVAL;
    LOG_ERR(rc, "Invalid packet length %d", len);
    return -rc;
  }

  while (len) {
    to_send = (len < OOB_NIC_PKT_FRAGMENT_SIZE)
      ? len : OOB_NIC_PKT_FRAGMENT_SIZE;

    if (is_first) {
      if (to_send >= len) {
        /* this is the last pkt also */
        cmd = NIC_WRITE_PKT_SINGLE_CMD;
      } else {
        cmd = NIC_WRITE_PKT_FIRST_CMD;
      }
      is_first = 0;
    } else {
      if (to_send >= len) {
        /* this is the last pkt */
        cmd = NIC_WRITE_PKT_LAST_CMD;
      } else {
        cmd = NIC_WRITE_PKT_MIDDLE_CMD;
      }
    }

    rc = i2c_smbus_write_block_large_data(dev->on_file, cmd,
                                          to_send, data + has_sent);
    if (rc < 0) {
      rc = errno;
      LOG_ERR(rc, "Failed to sent packet with cmd 0x%x, has_sent=%d "
              "to_send=%d", cmd, has_sent, to_send);
      return -rc;
    }

    has_sent += to_send;
    len -= to_send;
    n_frags++;
  }

  LOG_VER("Sent a packet with %d bytes in %d fragments", has_sent, n_frags);

  return has_sent;
}

static int oob_nic_set_mng_ctrl(oob_nic *dev, const uint8_t *data, int len) {
  int rc;

  if (len <= 0) {
    rc = EINVAL;
    LOG_ERR(rc, "Invalid data length: %d", len);
    return -rc;
  }

  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_MNG_CTRL_CMD,
                                  len, data);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to send management control command for parameter # %d",
            data[0]);
    return -rc;
  }

  return 0;
}

static int oob_nic_set_force_up(oob_nic *dev, int enable) {
  uint8_t cmd[2];

  cmd[0] = NIC_MNG_CTRL_KEEP_LINK_UP_NUM;
  cmd[1] = enable
    ? NIC_MNG_CTRL_KEEP_LINK_UP_ENABLE : NIC_MNG_CTRL_KEEP_LINK_UP_DISABLE;

  LOG_DBG("Turn %s link force up", enable ? "on" : "off");
  return oob_nic_set_mng_ctrl(dev, cmd, sizeof(cmd));
}

static int oob_nic_setup_filters(oob_nic *dev, const uint8_t mac[6]) {
  int rc;
  uint32_t cmd32;
  uint8_t buf[32];
  uint8_t *cmd;

  /*
   * There are 8 filters in total (MDEF0-MDEF7). Any filter that has a
   * configuration will be applied. Any packet that matches any filter will
   * be passed to OOB by the main NIC.
   *
   * Each filter has two sets of bits, MDEF and MDEF_EXT. Each bit in the
   * filter represents a filter with its logical operation. For example,
   * NIC_FILTER_MDEF_MAC_AND_OFFSET(0) represent MAC filter 0 using AND
   * operation. So, in order to receive packets matching a specific MAC, MAC0
   * filter (NIC_FILTER_MAC_NUM:NIC_FILTER_MAC_PAIR0) must be programmed
   * with the specific MAC. Then set NIC_FILTER_MDEF_MAC_AND_OFFSET (for
   * AND) or NIC_FILTER_MDEF_MAC_OR_OFFSET (for OR) in one of the filters.
   */

  /*
   * Command to set MAC filter
   * Seven bytes are required to load the MAC address filters.
   * Data 2—MAC address filters pair number (3:0).
   * Data 3—MSB of MAC address.
   * ...
   * Data 8: LSB of MAC address.
   */
  /* set MAC filter to pair 0 */
  cmd = buf;
  *cmd++ = NIC_FILTER_MAC_NUM;
  *cmd++ = NIC_FILTER_MAC_PAIR0; /* pair 0 */
  memcpy(cmd, mac, 6);
  cmd += 6;
  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_FILTER_CMD,
                                  cmd - buf, buf);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to set MAC filter");
    return -rc;
  }

  /*
   * Command to enable filter
   *
   * 9 bytes to load the extended decision filters (MDEF_EXT & MDEF)
   * Data 2—MDEF filter index (valid values are 0...6)
   * Data 3—MSB of MDEF_EXT (DecisionFilter0)
   * ....
   * Data 6—LSB of MDEF_EXT (DecisionFilter0)
   * Data 7—MSB of MDEF (DecisionFilter0)
   * ....
   * Data 10—LSB of MDEF (DecisionFilter0)
   */

  /* enable MAC filter pair 0 on filter 0 */
  cmd = buf;
  *cmd++ = NIC_FILTER_DECISION_EXT_NUM;
  *cmd++ = NIC_FILTER_MDEF0;
  /* enable filter for traffic from network and host */
  cmd32 = htonl(NIC_FILTER_MDEF_BIT(NIC_FILTER_MDEF_EXT_NET_EN_OFFSET)
                | NIC_FILTER_MDEF_BIT(NIC_FILTER_MDEF_EXT_HOST_EN_OFFSET));
  memcpy(cmd, &cmd32, sizeof(cmd32));
  cmd += sizeof(cmd32);
  /* enable mac pair 0 */
  cmd32 = htonl(NIC_FILTER_MDEF_BIT_VAL(NIC_FILTER_MDEF_MAC_AND_OFFSET,
                                        NIC_FILTER_MAC_PAIR0));
  memcpy(cmd, &cmd32, sizeof(cmd32));
  cmd += sizeof(cmd32);
  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_FILTER_CMD,
                                  cmd - buf, buf);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to set MAC filter to MDEF 0");
    return -rc;
  }

  /* Program EtherType0 to match LLDP */
  cmd = buf;
  *cmd++ = NIC_FILTER_ETHERTYPE_NUM;
  *cmd++ = NIC_FILTER_ETHERTYPE0;
  cmd32 = htonl(ETHERTYPE_LLDP);
  memcpy(cmd, &cmd32, sizeof(cmd32));
  cmd += sizeof(cmd32);
  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_FILTER_CMD,
                                  cmd - buf, buf);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to program EtherType0 to match LLDP");
    return -rc;
  }

  /* enable ARP, ND, and EtheryType0 (OR) on filter 1 */
  cmd = buf;
  *cmd++ = NIC_FILTER_DECISION_EXT_NUM;
  *cmd++ = NIC_FILTER_MDEF1;
  /* enable filter for traffic from network and host, matching ethertype0 */
  cmd32 = htonl(NIC_FILTER_MDEF_BIT(NIC_FILTER_MDEF_EXT_NET_EN_OFFSET)
                | NIC_FILTER_MDEF_BIT(NIC_FILTER_MDEF_EXT_HOST_EN_OFFSET)
                | NIC_FILTER_MDEF_BIT_VAL(
                    NIC_FILTER_MDEF_EXT_ETHTYPE_OR_OFFSET,
                    NIC_FILTER_ETHERTYPE0));
  memcpy(cmd, &cmd32, sizeof(cmd32));
  cmd += sizeof(cmd32);

  /* enable ARP and ND */
  cmd32 = htonl(NIC_FILTER_MDEF_BIT(NIC_FILTER_MDEF_ARP_REQ_OR_OFFSET)
                | NIC_FILTER_MDEF_BIT(NIC_FILTER_MDEF_ARP_RES_OR_OFFSET)
                | NIC_FILTER_MDEF_BIT(NIC_FILTER_MDEF_NBG_OR_OFFSET));
  memcpy(cmd, &cmd32, sizeof(cmd32));
  cmd += sizeof(cmd32);
  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_FILTER_CMD,
                                  cmd - buf, buf);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to set ARP and ND filter to MDEF 1");
    return -rc;
  }

  /* make filter 0, matching MAC, to be mng only */
  cmd = buf;
  *cmd++ = NIC_FILTER_MNG_ONLY_NUM;
  cmd32 = htonl(NIC_FILTER_MNG_ONLY_FILTER0);
  memcpy(cmd, &cmd32, sizeof(cmd32));
  cmd += sizeof(cmd32);
  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_FILTER_CMD,
                                  cmd - buf, buf);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to enabled management only filter");
    return -rc;
  }

  return 0;
}

int oob_nic_start(oob_nic *dev, const uint8_t mac[6]) {
  int rc;
  uint8_t cmd;

  /* force the link up, no matter what the status of the main link */
  rc = oob_nic_set_force_up(dev, 1);
  if (rc != 0) {
    return rc;
  }

  oob_nic_setup_filters(dev, mac);

  /* first byte is the control */
  cmd = NIC_WRITE_RECV_ENABLE_EN
    | NIC_WRITE_RECV_ENABLE_STA
    | NIC_WRITE_RECV_ENABLE_NM_UNSUPP /* TODO, to support ALERT */
    | NIC_WRITE_RECV_ENABLE_RESERVED;

  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_RECV_ENABLE_CMD,
                                  1, &cmd);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to start receive function");
    return -rc;
  }
  LOG_DBG("Started receive function");
  return 0;
}

int oob_nic_stop(oob_nic *dev) {
  int rc;
  uint8_t ctrl;
  /* don't set any enable bits, which turns off the receive func */
  ctrl = NIC_WRITE_RECV_ENABLE_RESERVED;
  rc = i2c_smbus_write_block_data(dev->on_file, NIC_WRITE_RECV_ENABLE_CMD,
                                  1, &ctrl);
  if (rc < 0) {
    rc = errno;
    LOG_ERR(rc, "Failed to stop receive function");
    return -rc;
  }
  LOG_DBG("Stopped receive function");
  return 0;
}
