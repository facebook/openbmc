/*
 * libusb example program to list devices on the bus
 * Copyright © 2007 Daniel Drake <dsd@gentoo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <openssl/sha.h>

#include "bic_fwupdate.h"
#include "bic_bios_fwupdate.h"

#define USB_PKT_SIZE 0x200
#define USB_DAT_SIZE (USB_PKT_SIZE - USB_PKT_HDR_SIZE)
#define USB_PKT_SIZE_BIG 0x1000
#define USB_DAT_SIZE_BIG (USB_PKT_SIZE_BIG - USB_PKT_HDR_SIZE)
#define SIZE_IANA_ID 3
#define PESW_UPDATE_BLK_SIZE (64*1024)
#define SIMPLE_DIGEST_LENGTH 4
#define STRONG_DIGEST_LENGTH SHA256_DIGEST_LENGTH

#define MAX_FW_PCIE_SWITCH_BLOCK_SIZE 1008
#define PCIE_FW_IDX 0x05

/*******************************************************************************************************/

int
bic_update_fw_usb_brcm(uint8_t slot_id __attribute__((unused)), uint8_t comp __attribute__((unused)), int fd, usb_dev* udev)
{
  int ret = -1;
  uint8_t *buf = NULL;
  uint8_t usb_package_buf[USB_PKT_SIZE_BIG] = {0};
  unsigned int write_offset = 0;

  buf = malloc(USB_PKT_HDR_SIZE + PESW_UPDATE_BLK_SIZE);
  if (buf == NULL) {
    fprintf(stderr, "failed to allocate memory\n");
    goto out;
  }

  int num_blocks_written = 0, num_blocks_skipped = 0;
  int cs_len = STRONG_DIGEST_LENGTH;
  int attempts = NUM_ATTEMPTS;
  size_t file_buf_num_bytes = 0;
  while (attempts > 0) {
    uint8_t *file_buf = buf + USB_PKT_HDR_SIZE;
    size_t file_buf_pos = 0;
    bool send_packet_fail = false;
    fprintf(stderr, "\r%d blocks ...",
        num_blocks_written + num_blocks_skipped);
    fflush(stderr);
    // Read a block of data from file.
    while (file_buf_num_bytes < PESW_UPDATE_BLK_SIZE) {
      size_t num_to_read = PESW_UPDATE_BLK_SIZE - file_buf_num_bytes;
      ssize_t num_read = read(fd, file_buf, num_to_read);
      if (num_read < 0) {
        if (errno == EINTR) {
          continue;
        }
        fprintf(stderr, "read error: %d\n", errno);
        goto out;
      }
      if (num_read == 0) {
        break;
      }
      file_buf_num_bytes += num_read;
    }
    // Finished.
    if (file_buf_num_bytes == 0) {
      break;
    }
    // Pad to 64K with 0xff, if needed.
    for (size_t i = file_buf_num_bytes; i < PESW_UPDATE_BLK_SIZE; i++, file_buf_num_bytes++) {
      file_buf[i] = '\xff';
    }
    while (file_buf_pos < file_buf_num_bytes) {
      unsigned int count = file_buf_num_bytes - file_buf_pos;
      // 4K USB packets and SHA256 checksums were added together,
      // so if we have SHA256 checksum, we can use big packets as well.
      size_t limit = (cs_len == STRONG_DIGEST_LENGTH ? USB_DAT_SIZE_BIG : USB_DAT_SIZE);
      if (count > limit) count = limit;
      bic_usb_packet *pkt = (bic_usb_packet *) usb_package_buf;
      pkt->dummy = PCIE_FW_IDX;
      pkt->offset = write_offset + file_buf_pos;
      pkt->length = count;
      memcpy(&(pkt->data), file_buf + file_buf_pos, count);
      int rc = send_bic_usb_packet(udev, pkt, false);
      if (rc < 0) {
        fprintf(stderr, "failed to write %u bytes @ %u: %d\n", count, write_offset, rc);
        send_packet_fail = true;
        break;  //prevent the endless while loop.
      }
      file_buf_pos += count;
    }

    if (send_packet_fail) {
      attempts--;
      continue;
    }

    write_offset += PESW_UPDATE_BLK_SIZE;
    file_buf_num_bytes = 0;
    num_blocks_written++;
    attempts = NUM_ATTEMPTS;
  }
  if (attempts == 0) {
    fprintf(stderr, "failed.\n");
    goto out;
  }

  fprintf(stderr, "finished.\n");

  ret = 0;

out:
  free(buf);
  return ret;
}

int
update_bic_brcm(uint8_t slot_id, uint8_t comp, int fd, uint8_t intf)
{
  struct timeval start, end;
  char key[64];
  int ret = -1;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  udev->ci = 1;
  udev->epaddr = 0x1;

  remote_bic_set_gpio(slot_id, BRCM_GPIO_SPI_LS_EN_N, VALUE_LOW, intf);
  remote_bic_set_gpio(slot_id, BRCM_GPIO_SPI_MUX_SEL_N, VALUE_LOW, intf);
  bic_set_gpio(slot_id, GPIO_RST_USB_HUB, VALUE_HIGH);
  sleep(60);

  // init usb device
  ret = bic_init_usb_dev(slot_id, udev, EXP2_TI_PRODUCT_ID, EXP2_TI_VENDOR_ID);
  if (ret < 0) {
    goto error_exit;
  }
  sleep(10);

  gettimeofday(&start, NULL);

  // sending file
  ret = bic_update_fw_usb_brcm(slot_id, comp, fd, udev);
  if (ret < 0)
    goto error_exit;

  gettimeofday(&end, NULL);
  if (comp == FW_BIOS) {
    fprintf(stderr, "Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  }

  ret = 0;
error_exit:
  sprintf(key, "fru%u_fwupd", slot_id);
  remove(key);

  // close usb device
  bic_close_usb_dev(udev);
  return ret;
}
