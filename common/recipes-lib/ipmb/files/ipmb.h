/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#ifndef __IPMB_H__
#define __IPMB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SOCK_PATH_IPMB "ipmb_socket"

#define BMC_SLAVE_ADDR 0x10
#define BRIDGE_SLAVE_ADDR 0x20
#define ZERO_CKSUM_CONST 0x100

// rqSA, rsSA, rqSeq, hdrCksum, dataCksum
#define IPMB_HDR_SIZE 5

// rqSA, NetFn, hdrCksum
#define IPMB_DATA_OFFSET 3

// Slot#0 is on I2C Bus1
#define IPMB_BUS_SLOT0 1

// TODO: Some IPMB responses take about 5-6 seconds
// Need to add a timeout parameter to IPMB request
// For now changing global timeout to 8 seconds
#if !defined(TIMEOUT_IPMB)
  #define TIMEOUT_IPMB 8
#endif
#define MIN_IPMB_REQ_LEN 7
#define MAX_IPMB_RES_LEN 300
#define MIN_IPMB_RES_LEN 8

typedef struct _ipmb_req_t {
  uint8_t res_slave_addr;
  uint8_t netfn_lun;
  uint8_t hdr_cksum;
  uint8_t req_slave_addr;
  uint8_t seq_lun;
  uint8_t cmd;
  uint8_t data[];
} ipmb_req_t;

typedef struct _ipmb_res_t {
  uint8_t req_slave_addr;
  uint8_t netfn_lun;
  uint8_t hdr_cksum;
  uint8_t res_slave_addr;
  uint8_t seq_lun;
  uint8_t cmd;
  uint8_t cc;
  uint8_t data[];
} ipmb_res_t;

int lib_ipmb_handle(unsigned char bus_id,
                    unsigned char *request, unsigned short req_len,
                    unsigned char *response, unsigned char *res_len);
int
lib_ipmb_send_request(uint8_t ipmi_cmd, uint8_t netfn,
              uint8_t *txbuf, uint8_t txlen, 
              uint8_t *rxbuf, uint8_t *rxlen,
              uint8_t bus_num, uint8_t dev_addr, uint8_t bmc_addr);


/*
 * ipmb_send():
 *   Send IPMB command without prepare tx data.
 *   Return length of 'Data' on Success
 *   Return -1 on failure
 * ipmb_send_buf():
 *   Prepare tx data in buffer from ipmb_txb() before call this function,
 *   only Target_Addr, NetFn_LUN, CMD and Data are required, other fields
 *   should be handled by ipmbd, tlen is total tx length include header,
 *   data and checksum.
 *   Return length of effective 'rx buff' on Success
 *   Return -1 on failure
 * ipmb_txb():
 *   Return thread specific tx buffer
 * ipmb_rxb():
 *   Return thread specific rx buffer
 */
#define OBMC_API_IPMB_CMD_END 0x193BCDED
int ipmb_send_internal (unsigned char bus_id,
  unsigned char addr,
  unsigned char netfn,
  unsigned char cmd, ...);
#define ipmb_send(...) ipmb_send_internal(__VA_ARGS__, OBMC_API_IPMB_CMD_END)
int ipmb_send_buf (unsigned char bus_id, unsigned char tlen);
ipmb_res_t* ipmb_rxb();
ipmb_req_t* ipmb_txb();

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __IPMB_H__ */
