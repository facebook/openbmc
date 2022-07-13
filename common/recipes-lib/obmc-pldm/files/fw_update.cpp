/*
 * Copyright 2020-present Meta Platforms, Inc. All Rights Reserved.
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

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "fw_update.h"

int obmc_pldm_fw_update (uint8_t bus, uint8_t dst_eid, char *path)
{
  pldm_cmd_req  pldmReq = {0};
  pldm_response pldmRes = {0};
  pldm_fw_pkg_hdr_t *pkgHdr;
  size_t reqSize, respSize;
  int sockfd, ret, pldmCmdStatus = -1;;

  auto pldmHdr = (struct pldm_hdr*)pldmReq.common;
  auto reqBuf  = (uint8_t *)pldmReq.common;
  auto respBuf = (uint8_t *)pldmRes.common;


  // init connection with pldm daemon.
  sockfd = oem_pldm_init_fwupdate_fd(bus);
  if (sockfd < 0) {
    fprintf(stderr, "Failed to connect pldm daemon.");
    return -1;
  }

  // init firmware package
  pkgHdr = pldm_parse_fw_pkg(path);
  if (!pkgHdr) {
    ret = -1;
    goto free_exit;
  }

  // send 01 packet
  pldmCreateReqUpdateCmd(pkgHdr, &pldmReq, 1024);
  printf("\n01 PldmRequestUpdateOp: payload_size=%d\n", pldmReq.payload_size);
  reqSize = pldmReq.payload_size;
  ret = oem_pldm_send_recv_w_fd(dst_eid, sockfd, reqBuf, reqSize, &respBuf, &respSize);
  if (ret != 0) {
    goto free_exit;
  }

  // send 02 packet
  for (int i = 0; i < pkgHdr->componentImageCnt; ++i) {
    pldmCreatePassComponentTblCmd(pkgHdr, i, &pldmReq);
    printf("\n02 PldmPassComponentTableOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    reqSize = pldmReq.payload_size;
    ret = oem_pldm_send_recv_w_fd(dst_eid, sockfd, reqBuf, reqSize, &respBuf, &respSize);
    if (ret != 0) {
      goto exit;
    }
  }

  // send 03 packet
  for (int i = 0; i < pkgHdr->componentImageCnt; ++i) {
    pldmCreateUpdateComponentCmd(pkgHdr, i, &pldmReq);
    printf("\n03 PldmUpdateComponentOp[%d]: payload_size=%d\n", i,
            pldmReq.payload_size);
    reqSize = pldmReq.payload_size;
    ret = oem_pldm_send_recv_w_fd(dst_eid, sockfd, reqBuf, reqSize, &respBuf, &respSize);
    if (ret != 0) {
      goto exit;
    }
  }

  // FW data transfer
  while (1) {
    if (oem_pldm_recv(dst_eid, sockfd, &reqBuf, &reqSize) != CC_SUCCESS) 
      break;

    pldmHdr = (struct pldm_hdr*) reqBuf;
    if ( (pldmHdr->Command_Code == CMD_REQUEST_FIRMWARE_DATA) ||
         (pldmHdr->Command_Code == CMD_TRANSFER_COMPLETE) ||
         (pldmHdr->Command_Code == CMD_VERIFY_COMPLETE) ||
         (pldmHdr->Command_Code == CMD_APPLY_COMPLETE)) {

      pldmReq.payload_size = reqSize;
      memcpy(pldmReq.common, reqBuf, reqSize);
      pldmCmdStatus = pldmFwUpdateCmdHandler(pkgHdr, &pldmReq, &pldmRes);
      respBuf = (uint8_t *)&pldmRes;
      respSize = pldmRes.resp_size;
      if (oem_pldm_send(dst_eid, sockfd, respBuf+2, respSize) < 0)
        break;

      if ((pldmHdr->Command_Code == CMD_APPLY_COMPLETE) || (pldmCmdStatus == -1))
        break;

    } else {
      printf("unknown PLDM cmd 0x%02X\n", pldmHdr->Command_Code);
      break;
    }
  }

exit:
  // only activate FW if update loop exists with good status
  if (!pldmCmdStatus && (pldmHdr->Command_Code == CMD_APPLY_COMPLETE)) {
    // update successful,  activate FW
    pldmCreateActivateFirmwareCmd(&pldmReq);
    printf("\n05 PldmActivateFirmwareOp\n");
    reqBuf = (uint8_t *)&pldmReq;
    reqSize = pldmReq.payload_size;
    ret = oem_pldm_send_recv_w_fd(dst_eid, sockfd, reqBuf+2, reqSize, &respBuf, &respSize);
    if (ret != 0) {
      goto free_exit;
    }

  } else {
    printf("PLDM cmd (0x%02X) failed (status %d), abort update\n",
      pldmHdr->Command_Code, pldmCmdStatus);

    // send abort update req
    pldmCreateCancelUpdateCmd(&pldmReq);
    reqBuf = (uint8_t *)&pldmReq;
    reqSize = pldmReq.payload_size;
    ret = oem_pldm_send_recv_w_fd(dst_eid, sockfd, reqBuf+2, reqSize, &respBuf, &respSize);
    if (ret != 0) {
      goto free_exit;
    }
    ret = -1;
  }

free_exit:
  if (pkgHdr)
    free_pldm_pkg_data(&pkgHdr);
  oem_pldm_close(sockfd);
  return ret;
}
