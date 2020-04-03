/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#ifndef _PLDM_H_
#define _PLDM_H_

#include "pldm_base.h"
#include "pldm_fw_update.h"

// PLDM types, defined in DMTF DSP0245 v1.2.0
typedef enum pldm_type {
  PLDM_TYPE_MSG_CTRL_AND_DISCOVERY       = 0,
  PLDM_TYPE_SMBIOS                       = 1,
  PLDM_TYPE_PLATFORM_MONITORING_AND_CTRL = 2,
  PLDM_TYPE_BIOS_CTRL_AND_CFG            = 3,
  PLDM_TYPE_FRU_DATA                     = 4,
  PLDM_TYPE_FIRMWARE_UPDATE              = 5,
} PldmType;


#define MAX_PLDM_MSG_SIZE  (NCSI_MAX_PAYLOAD)

typedef struct {
  uint16_t payload_size;
  unsigned char  common[PLDM_COMMON_REQ_LEN];
  unsigned char  payload[MAX_PLDM_MSG_SIZE - PLDM_COMMON_REQ_LEN];
} __attribute__((packed)) pldm_cmd_req;

typedef struct {
  uint16_t resp_size;
  unsigned char  common[PLDM_COMMON_RES_LEN];
  unsigned char  response[MAX_PLDM_MSG_SIZE - PLDM_COMMON_RES_LEN];
} __attribute__((packed)) pldm_response;



pldm_fw_pkg_hdr_t *pldm_parse_fw_pkg(char *path);
const char *pldm_fw_cmd_cc_to_name(uint8_t comp_code);

void genReqCommonFields(PldmType type, PldmFWCmds cmd, uint8_t *common);
void dbgPrintCdb(pldm_cmd_req *cdb);

void free_pldm_pkg_data           (pldm_fw_pkg_hdr_t **pFwPkgHdr);
void pldmCreateReqUpdateCmd       (pldm_fw_pkg_hdr_t *pFwPkgHdr,
                                   pldm_cmd_req *pPldmCdb,
                                   int pldm_bufsize);
void pldmCreatePassComponentTblCmd(pldm_fw_pkg_hdr_t *pFwPkgHdr, uint8_t compIdx,
                                   pldm_cmd_req *pPldmCdb);
void pldmCreateUpdateComponentCmd (pldm_fw_pkg_hdr_t *pFwPkgHdr, uint8_t compIdx,
                                   pldm_cmd_req *pPldmCdb);
void pldmCreateActivateFirmwareCmd(pldm_cmd_req *pPldmCdb);
void pldmCreateCancelUpdateCmd(pldm_cmd_req *pPldmCdb);

void pldmCreateGetVersionCmd(uint8_t pldmType, pldm_cmd_req *pPldmCdb);
void pldmHandleGetVersionResp(PLDM_GetPldmVersion_Response_t *pPldmResp,
                              pldm_ver_t *version);

void pldmCreateGetPldmTypesCmd(pldm_cmd_req *pPldmCdb);
uint64_t pldmHandleGetPldmTypesResp(PLDM_GetPldmTypes_Response_t *pPldmResp);

void setPldmTimeout(int pldmCmd, int *timeout_sec);
int pldmFwUpdateCmdHandler(pldm_fw_pkg_hdr_t *pkgHdr, pldm_cmd_req *pCmd,
                           pldm_response *pldmRes);
int pldmRespHandler(pldm_response *pResp);
int ncsiGetPldmCmd(NCSI_NL_RSP_T *nl_resp,  pldm_cmd_req *pCmd);
int ncsiDecodePldmCompCode(NCSI_NL_RSP_T *nl_resp);
int ncsiDecodePldmIID(NCSI_Response_Packet *ncsi_resp);
int ncsiDecodePldmType(NCSI_Response_Packet *ncsi_resp);
int ncsiDecodePldmCmd(NCSI_Response_Packet *ncsi_resp);
unsigned char *get_pldm_response_payload(unsigned char *buf);

#endif
