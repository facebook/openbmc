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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openbmc/ncsi.h>
#include "pldm_base.h"
#include "pldm_fw_update.h"
#include "pldm.h"


#define DEBUG_PLDM

#ifdef DEBUG_PLDM
  #define DBG_PRINT(fmt, args...) printf(fmt, ##args)
#else
  #define DBG_PRINT(fmt, args...)
#endif

// global PLDM control & configuration variables
static uint8_t  gPldm_iid = 0; // technically only 5 bits as per DSP0240 v1.0.0
static uint32_t gPldm_transfer_size = PLDM_MAX_XFER_SIZE;
static uint8_t  gPldm_max_transfer_cnt = PLDM_MAX_XFER_CNT;

/* PLDM Firmware Update command name*/
const char *pldm_fw_cmd_string[NUM_PLDM_UPDATE_CDMS] = {
  "CMD_REQUEST_UPDATE",
  "CMD_GET_PACKAGE_DATA",
  "CMD_GET_DEVICE_METADATA",
  "CMD_PASS_COMPONENT_TABLE",
  "CMD_UPDATE_COMPONENT",
  "CMD_REQUEST_FIRMWARE_DATA",
  "CMD_TRANSFER_COMPLETE",
  "CMD_VERIFY_COMPLETE",
  "CMD_APPLY_COMPLETE",
  "CMD_GET_META_DATA",
  "CMD_ACTIVATE_FIRMWARE",
  "CMD_GET_STATUS",
  "CMD_CANCEL_UPDATE_COMPONENT",
  "CMD_CANCEL_UPDATE",
};

/* PLDM base completion code */
const char *pldm_base_cc_string[6] = {
    "SUCCESS",
    "ERROR",
    "ERROR_INVALID_DATA",
    "ERROR_INVALID_LENGTH",
    "ERROR_NOT_READY",
    "ERROR_UNSUPPORTED_PLDM_CMD",
};


/* PLDM Firmware completion code*/
const char *pldm_update_cmd_cc_string[NUM_FW_UPDATE_CC] = {
  "NOT_IN_UPDATE_MODE",
  "ALREADY_IN_UPDATE_MODE",
  "DATA_OUT_OF_RANGE",
  "INVALID_TRANSFER_LENTH",
  "INVALID_STATE_FOR_CMD",
  "INCOMPLETE_UPDATE",
  "BUSY_IN_BACKGROUND",
  "CANCEL_PENDING",
  "COMMAND_NOT_EXPECTED",
  "RETRY_REQUEST_FW_DATA",
  "UNABLE_TO_INITIATE_UPDATE",
  "ACTIVATION_NOT_REQUIRED",
  "SELF_CONTAINED_ACTIVATION_NOT_PEMITTED",
  "NO_DEVICE_METADATA",
  "RETRY_REQUEST_UPDATE",
  "NO_PACKAGE_DATA",
  "INVALID_DATA_TRANSFER_HANDLE",
  "INVALID_TRANSFER_OPERATION_FLAG",
};

/* PLDM component class name*/
const char *pldm_comp_class_name[NUM_COMP_CLASS] = {
  "C_CLASS_UNKNOWN",
  "C_CLASS_OTHER",
  "C_CLASS_DRIVER",
  "C_CLASS_CONFIG_SW",
  "C_CLASS_APPLICATION_SW",
  "C_CLASS_INSTRUMENTATION",
  "C_CLASS_FIRMWARE_BIOS",
  "C_CLASS_DIAG_SW",
  "C_CLASS_OS",
  "C_CLASS_MIDDLEWARE",
  "C_CLASS_FIRMWARE",
  "C_CLASS_BIOS_FCODE",
  "C_CLASS_SUPPORT_SERVICE_PACK",
  "C_CLASS_SOFTWARE_BUNDLE",
};


/* PLDM firmware string type*/
const char *pldm_str_type_name[NUM_PLDM_FW_STR_TYPES] = {
  "UNKNOWN",
  "ASCII",
  "UTF8",
  "UTF16",
  "UTF16LE",
  "UTF16BE",
};

const char *
pldm_str_type_to_name(fwStrType strType)
{
  if (strType < 0 || strType >= NUM_PLDM_FW_STR_TYPES  ||
         pldm_str_type_name[strType] == NULL) {
      return "unknown_str_type";
  }
  return pldm_str_type_name[strType];
}


const char *
pldm_fw_cmd_to_name(PldmFWCmds cmd)
{
  if (cmd < CMD_REQUEST_UPDATE ||
      cmd > CMD_CANCEL_UPDATE  ||
      pldm_fw_cmd_string[cmd - CMD_REQUEST_UPDATE] == NULL) {
      return "unknown_pldm_fw_cmd";
  }
  return pldm_fw_cmd_string[cmd - CMD_REQUEST_UPDATE];
}


const char *
pldm_fw_cmd_cc_to_name(uint8_t comp_code)
{
  if ((comp_code <= CC_ERR_UNSUPPORTED_PLDM_CMD) &&
      pldm_base_cc_string[comp_code] != NULL) {
      return pldm_base_cc_string[comp_code];
  } else if (comp_code == CC_INVALID_PLDM_TYPE) {
      return "ERROR_INVALID_PLDM_TYPE";
  } else if (comp_code < CC_FW_UPDATE_BASE ||
      comp_code > CC_INVALID_TRANSFER_OPERATION_FLAG  ||
      pldm_update_cmd_cc_string[comp_code - CC_FW_UPDATE_BASE] == NULL) {
      return "unknown_completion code";
  }
  return pldm_update_cmd_cc_string[comp_code - CC_FW_UPDATE_BASE];
}

const char *
pldm_comp_class_to_name(PldmComponentClass compClass)
{
  if (compClass < 0 || compClass >= NUM_COMP_CLASS  ||
         pldm_comp_class_name[compClass] == NULL) {
      return "unknown_classification";
  }
  return pldm_comp_class_name[compClass];
}

void dbgPrintCdb(pldm_cmd_req *cdb)
{
  int i = 0;
  DBG_PRINT("%s\n   common:  ", __FUNCTION__);
  for (i=0; i<3; ++i)
    DBG_PRINT("%02x", cdb->common[i]);
  DBG_PRINT("\n   payload: ");
  for (i=0; i<cdb->payload_size; ++i) {
    DBG_PRINT("%02x", cdb->payload[i]);
    if ((i%4 == 3) && (i%15 != 0))
      DBG_PRINT(" ");
    if (i%16 == 15)
      DBG_PRINT("\n            ");
  }
  DBG_PRINT("\n");

  DBG_PRINT("\n   iid=%d, cmd=0x%x (%s)\n\n",
             (cdb->common[0]) & 0x1f,
             cdb->common[2],
             pldm_fw_cmd_to_name(cdb->common[2]));
}


void genReqCommonFields(PldmFWCmds cmd, uint8_t *common)
{
  uint8_t iid = 0;

  gPldm_iid = (gPldm_iid + 1) & 0x1f;;
  iid = gPldm_iid;

  common[0] = 0x80 | (iid & 0x1f);
  common[1] = PLDM_TYPE_FIRMWARE_UPDATE;
  common[2] = cmd;
}



// Given a PLDM package pointed by pFwPkgHdr
// Generate command cdb for PLDM  RequestUpdate command (Type 0x05, cmd 0x10)
void pldmCreateReqUpdateCmd(pldm_fw_pkg_hdr_t *pFwPkgHdr, pldm_cmd_req *pPldmCdb)
{
  PLDM_RequestUpdate_t *pCmdPayload = (PLDM_RequestUpdate_t *)&(pPldmCdb->payload);

  printf("\n\nCMD_REQUEST_UPDATE\n");

  genReqCommonFields(CMD_REQUEST_UPDATE, &(pPldmCdb->common[0]));


  pCmdPayload->maxTransferSize = gPldm_transfer_size;
  pCmdPayload->numComponents = pFwPkgHdr->componentImageCnt;
  pCmdPayload->maxOutstandingTransferRequests = gPldm_max_transfer_cnt;
  pCmdPayload->packageDataLength =
     pFwPkgHdr->pDevIdRecs[0]->pRecords->fwDevPkgDataLength;

  pCmdPayload->componentImageSetVersionStringType =
     pFwPkgHdr->pDevIdRecs[0]->pRecords->compImgSetVersionStringType;

  pCmdPayload->componentImageSetVersionStringLength =
     pFwPkgHdr->pDevIdRecs[0]->pRecords->compImgSetVersionStringLength;

  if (pCmdPayload->componentImageSetVersionStringLength > MAX_VERSION_STRING_LEN)
  {
    pCmdPayload->componentImageSetVersionStringLength = MAX_VERSION_STRING_LEN;
    syslog(LOG_ERR, "%s version length(%d) exceeds max (%d)",
        __FUNCTION__, pCmdPayload->componentImageSetVersionStringLength,
        MAX_VERSION_STRING_LEN);
  }

  memcpy(pCmdPayload->componentImageSetVersionString,
         pFwPkgHdr->pDevIdRecs[0]->versionString,
         pCmdPayload->componentImageSetVersionStringLength);

  pPldmCdb->payload_size = PLDM_COMMON_REQ_LEN +
      offsetof(PLDM_RequestUpdate_t, componentImageSetVersionString) +
      pCmdPayload->componentImageSetVersionStringLength;

  dbgPrintCdb(pPldmCdb);
  return;
}


// Given:
//    1. a PLDM package pointed by pFwPkgHdr
//    2. component index within PLDM package
// Generate command cdb for PLDM  Pass Component Table command (Type 0x05, cmd 0x11)
void pldmCreatePassComponentTblCmd(pldm_fw_pkg_hdr_t *pFwPkgHdr, uint8_t compIdx,
                            pldm_cmd_req *pPldmCdb)
{
  PLDM_PassComponentTable_t *pCmdPayload =
      (PLDM_PassComponentTable_t *)&(pPldmCdb->payload);
  pldm_component_img_info_t *pCompImgInfo = pFwPkgHdr->pCompImgInfo[compIdx];

  printf("\n\nCMD_PASS_COMPONENT_TABLE\n");

  genReqCommonFields(CMD_PASS_COMPONENT_TABLE, &(pPldmCdb->common[0]));

  pCmdPayload->transferFlag = TFLAG_STATRT_END;  // only support 1 pass for now
  pCmdPayload->class = pCompImgInfo->class;
  pCmdPayload->id = pCompImgInfo->id;
  pCmdPayload->classIndex = 0;  // TBD - needs to query for this from NIC
  pCmdPayload->compStamp = pCompImgInfo->compStamp;
  pCmdPayload->versionStringType = pCompImgInfo->versionStringType;
  pCmdPayload->versionStringLength = pCompImgInfo->versionStringLength;
  if (pCmdPayload->versionStringLength > MAX_VERSION_STRING_LEN) {
    pCmdPayload->versionStringLength = MAX_VERSION_STRING_LEN;
    syslog(LOG_ERR, "%s version length(%d) exceeds max (%d)",
        __FUNCTION__, pCmdPayload->versionStringLength, MAX_VERSION_STRING_LEN);
  }
  memcpy(pCmdPayload->versionString, pCompImgInfo->versionString,
         pCmdPayload->versionStringLength);

  pPldmCdb->payload_size = PLDM_COMMON_REQ_LEN +
        offsetof(PLDM_PassComponentTable_t, versionString) +
        pCmdPayload->versionStringLength;

  dbgPrintCdb(pPldmCdb);
}



void pldmCreateUpdateComponentCmd(pldm_fw_pkg_hdr_t *pFwPkgHdr, uint8_t compIdx,
                                  pldm_cmd_req *pPldmCdb)
{
  PLDM_UpdateComponent_t *pCmdPayload =
      (PLDM_UpdateComponent_t *)&(pPldmCdb->payload);
  pldm_component_img_info_t *pCompImgInfo = pFwPkgHdr->pCompImgInfo[compIdx];

  printf("\n\nCMD_UPDATE_COMPONENT\n");

  genReqCommonFields(CMD_UPDATE_COMPONENT, &(pPldmCdb->common[0]));

  pCmdPayload->class = pCompImgInfo->class;
  pCmdPayload->id = pCompImgInfo->id;
  pCmdPayload->classIndex = 0;  // TBD - needs to query for this from NIC
  pCmdPayload->compStamp = pCompImgInfo->compStamp;
  pCmdPayload->compSize = pCompImgInfo->size;
  pCmdPayload->updateOptions = pCompImgInfo->options;
  pCmdPayload->versionStringType = pCompImgInfo->versionStringType;
  pCmdPayload->versionStringLength  = pCompImgInfo->versionStringLength;

  if (pCmdPayload->versionStringLength > MAX_VERSION_STRING_LEN) {
    pCmdPayload->versionStringLength = MAX_VERSION_STRING_LEN;
    syslog(LOG_ERR, "%s version length(%d) exceeds max (%d)",
        __FUNCTION__, pCmdPayload->versionStringLength, MAX_VERSION_STRING_LEN);
  }
  memcpy(pCmdPayload->versionString, pCompImgInfo->versionString,
         pCmdPayload->versionStringLength);

  pPldmCdb->payload_size = PLDM_COMMON_REQ_LEN +
        offsetof(PLDM_UpdateComponent_t, versionString) +
        pCmdPayload->versionStringLength;

  dbgPrintCdb(pPldmCdb);
}


void pldmCreateActivateFirmwareCmd(pldm_cmd_req *pPldmCdb)
{
  PLDM_ActivateFirmware_t *pCmdPayload =
      (PLDM_ActivateFirmware_t *)&(pPldmCdb->payload);

  printf("\n\nCMD_ACTIVATE_FIRMWARE\n");

  genReqCommonFields(CMD_ACTIVATE_FIRMWARE, &(pPldmCdb->common[0]));
  pCmdPayload->selfContainedActivationRequest = 0; // no self activation
  pPldmCdb->payload_size = PLDM_COMMON_REQ_LEN +
        sizeof(PLDM_ActivateFirmware_t);
}


// helper function to check if a given UUID is a PLDM FW Package
int isPldmFwUuid(char *uuid)
{
  return strncmp(uuid, PLDM_FW_UUID, PLDM_FW_UUID_LEN);
}


void printHdrInfo(pldm_fw_pkg_hdr_info_t *hdr_info, int printVersion)
{
  int i;
  printf("\n\nPLDM Firmware Package Header Info\n");
  printf("  UUID: ");
  for (i=0; i<16; ++i)
    printf("%x", hdr_info->uuid[i]);
  printf("    PLDM UUID Y/N? (%C)\n", isPldmFwUuid((char *)(hdr_info->uuid))? 'N':'Y');
  printf("  headerRevision: 0x%x\n", hdr_info->headerRevision);
  printf("  headerSize: 0x%x\n", hdr_info->headerSize);
  printf("  componentBitmapBitLength: 0x%x\n", hdr_info->componentBitmapBitLength);
  printf("  versionStringType: 0x%x (%s)\n", hdr_info->versionStringType,
          pldm_str_type_to_name(hdr_info->versionStringType));
  printf("  versionStringLength: 0x%x\n", hdr_info->versionStringLength);

  if (printVersion) {
    printf("  version string: ");
    for (i=0; i<hdr_info->versionStringLength; ++i)
      printf("%c", hdr_info->versionString[i]);
    printf("  (");
    for (i=0; i<hdr_info->versionStringLength; ++i)
      printf("0x%x ", hdr_info->versionString[i]);
    printf(")\n");
  }
}


void printDevIdRec(pldm_fw_pkg_hdr_t *pFwPkgHdr, int index)
{
  int i;
  uint8_t compBitFieldLen = pFwPkgHdr->phdrInfo->componentBitmapBitLength/8;
  pldm_fw_dev_id_records_t *pDevRc = pFwPkgHdr->pDevIdRecs[index];
  uint8_t versionLen = pDevRc->pRecords->compImgSetVersionStringLength;

  printf("\n\nDevice ID[%d] Record Info\n", index);
  printf("  devRec[%d].RecordLength: %d\n", index,
          pDevRc->pRecords->recordLength);
  printf("  devRec[%d].descriptorCnt: 0x%x\n", index,
          pDevRc->pRecords->descriptorCnt);
  printf("  devRec[%d].deviceUpdateOptionFlags: 0x%x\n", index,
          pDevRc->pRecords->devUpdateOptFlags);
  printf("  devRec[%d].compImgSetVersionStringType: 0x%x (%s)\n", index,
          pDevRc->pRecords->compImgSetVersionStringType,
          pldm_str_type_to_name(pDevRc->pRecords->compImgSetVersionStringType));
  printf("  devRec[%d].compImgSetVersionStringLength: 0x%x\n", index,
          pDevRc->pRecords->compImgSetVersionStringLength);
  printf("  devRec[%d].fwDevPkgDataLength: 0x%x\n", index,
          pDevRc->pRecords->fwDevPkgDataLength);

  printf("  devRec[%d].applicableComponents: ", index);
  for (i=0; i<compBitFieldLen; ++i)
    printf("%d\n", pDevRc->pApplicableComponents[i]);

  printf("  devRec[%d].versionString: ", index);
  for (i=0; i<versionLen; ++i)
    printf("%c", pDevRc->versionString[i]);
  printf("  (");
  for (i=0; i<versionLen; ++i)
    printf("0x%x ", pDevRc->versionString[i]);
  printf(")\n");

  // print each record descriptors
  printf("  Record Descriptors\n");
  for (i=0; i<pDevRc->pRecords->descriptorCnt; ++i) {
    printf("    Record Descriptor[%d]\n", i);
    printf("      Type=0x%x", pDevRc->pRecordDes[i]->type);
    printf("      length=0x%x", pDevRc->pRecordDes[i]->length);
    printf("      data=[");
    for (int j=0; j<pDevRc->pRecordDes[i]->length; ++j) {
      printf("%x",pDevRc->pRecordDes[i]->data[j]);
    }
    printf("]\n");
  }
}



void printComponentImgInfo(pldm_fw_pkg_hdr_t *pFwPkgHdr, int index)
{
  int i;
  pldm_component_img_info_t *pComp = pFwPkgHdr->pCompImgInfo[index];

  printf("\n\nComponent[%d] Info\n", index);
  printf("  Component[%d].classification: %d (%s)\n", index,
          pComp->class, pldm_comp_class_to_name(pComp->class));
  printf("  Component[%d].identifier: 0x%x\n", index,
          pComp->id);
  printf("  Component[%d].comparison stamp: 0x%x\n", index,
          pComp->compStamp);
  printf("  Component[%d].component options: 0x%x\n", index,
          pComp->options);
  printf("  Component[%d].request update activation method: 0x%x\n", index,
          pComp->requestedActivationMethod);
  printf("  Component[%d].location offset: 0x%x\n", index,
          pComp->locationOffset);
  printf("  Component[%d].component size: 0x%x\n", index,
          pComp->size);
  printf("  Component[%d].versionStringType: 0x%x (%s)\n", index,
          pComp->versionStringType,
          pldm_str_type_to_name(pComp->versionStringType));
  printf("  Component[%d].versionStringLength: 0x%x\n", index,
          pComp->versionStringLength);

  printf("  Component[%d].versionString: ", index);
  for (i=0; i<pComp->versionStringLength; ++i)
    printf("%c", pComp->versionString[i]);
  printf("  (");
  for (i=0; i<pComp->versionStringLength; ++i)
    printf("0x%x ", pComp->versionString[i]);
  printf(")\n");
}


// Given a PLDM Firmware package, this function will
//  1. allocate a pldm_fw_pkg_hdr_t structure representing this package,
//  2. read PLDM firmware package to RAM
//  3. initialize header info area of pldm_fw_pkg_hdr_t
//  4. returns
//       1. pointer to the struct,
//       2. number of bytes consumed in the header info area
//
int
init_pkg_hdr_info(char *path, pldm_fw_pkg_hdr_t** pFwPkgHdr, int *pOffset)
{
  FILE *fp;
  int rcnt, size;
  struct stat buf;

  // Open the file exclusively for read
  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path :%s!\n", path);
    return -1;
  }

  stat(path, &buf);
  size = buf.st_size;
  printf("size of file is %d bytes\n", size);

  // allocate pointer structure to access fw pkg header
  *pFwPkgHdr = (pldm_fw_pkg_hdr_t *)calloc(1, sizeof(pldm_fw_pkg_hdr_t));
  if (!(*pFwPkgHdr)) {
    printf("ERROR: pFwPkgHdr malloc failed, size %d\n", sizeof(pldm_fw_pkg_hdr_t));
    fclose(fp);
    return -1;
  }

  // allocate actual buffer to store fw package header
  (*pFwPkgHdr)->rawHdrBuf = (unsigned char *)calloc(1, size);
  if (!(*pFwPkgHdr)->rawHdrBuf) {
    printf("ERROR: rawHdrBuf malloc failed, size %d\n", size);
    fclose(fp);
    return -1;
  }

  // Move the file pointer to the start and read the entire package header
  fseek(fp, 0, SEEK_SET);
  rcnt = fread((*pFwPkgHdr)->rawHdrBuf, 1, size, fp);
  if (rcnt < size) {
    printf("ERROR: rawHdrBuf read, rcnt(%d) < %d", rcnt,
           size);
    fclose(fp);
    return -1;
  }
  fclose(fp);


  (*pFwPkgHdr)->phdrInfo = (pldm_fw_pkg_hdr_info_t *)(*pFwPkgHdr)->rawHdrBuf;
  printHdrInfo((*pFwPkgHdr)->phdrInfo, 1);
  *pOffset += offsetof(pldm_fw_pkg_hdr_info_t, versionString) +
             (*pFwPkgHdr)->phdrInfo->versionStringLength;
  return 0;
}


// initialize "firmware device area" of pldm_fw_pkg_hdr_t structure,
//  i.e.  devIdRecordCnt
//        allocate array of device records and initialize corresponding pointers
//
// input: pFwPkgHdr with header area preinitialized
//        pOffset : pointer to starting offset of "firmware device area"
//                  of the package
// output: pFwPkgHdr with firmware device area initialized
//         pOffset: updated to pointing to starting offset of Component area
//
int
init_device_id_records(pldm_fw_pkg_hdr_t* pFwPkgHdr, int *pOffset)
{
  int i;

  pFwPkgHdr->devIdRecordCnt = pFwPkgHdr->rawHdrBuf[*pOffset];
  *pOffset += sizeof(pFwPkgHdr->devIdRecordCnt);
  DBG_PRINT("\n\n Number of Device ID Record in package (devIdRecordCnt) =%d\n",
            pFwPkgHdr->devIdRecordCnt);

  // allocate a look up table of pointers to each devIdRecord
  pFwPkgHdr->pDevIdRecs =
      (pldm_fw_dev_id_records_t **) calloc(pFwPkgHdr->devIdRecordCnt,
                                        sizeof(pldm_fw_dev_id_records_t *));
  if (!pFwPkgHdr->pDevIdRecs)
  {
    printf("ERROR: pFwPkgHdr->pDevIdRecs malloc failed, size %d\n",
       sizeof(pldm_fw_dev_id_records_t *) * pFwPkgHdr->devIdRecordCnt);
    return -1;
  }

  for (i=0; i<pFwPkgHdr->devIdRecordCnt; ++i)
  {
    // pointer to current DeviceRecord we're working on
    pFwPkgHdr->pDevIdRecs[i] = calloc(1, sizeof(pldm_fw_dev_id_records_t));
    if (!(pFwPkgHdr->pDevIdRecs[i])) {
      printf("ERROR: pFwPkgHdr->pDevIdRecs[%d] malloc failed, size %d\n", i,
         sizeof(pldm_fw_dev_id_records_t));
      return -1;
    }
    pldm_fw_dev_id_records_t *pDevIdRec = pFwPkgHdr->pDevIdRecs[i];

    // "offset" will be updated to track sizeof(current deviceRecord)
    // "suboffset" is used to initialize subfields within current deviceRecord
    //    that are variable length
    int  subOffset = *pOffset;

    pDevIdRec->pRecords = (pldm_fw_dev_id_records_fixed_len_t *)(pFwPkgHdr->rawHdrBuf + subOffset);
    subOffset += sizeof(pldm_fw_dev_id_records_fixed_len_t);
    pDevIdRec->pApplicableComponents = (uint8_t *)(pFwPkgHdr->rawHdrBuf + subOffset);
    // length of pApplicableComponents field is defined in hdr info,
    subOffset += (pFwPkgHdr->phdrInfo->componentBitmapBitLength/8);
    pDevIdRec->versionString = (uint8_t *)(pFwPkgHdr->rawHdrBuf + subOffset);
    subOffset += pDevIdRec->pRecords->compImgSetVersionStringLength;

    // allocate a look up table of pointers to each Record Descriptor
    pDevIdRec->pRecordDes =
           (record_descriptors_t **) calloc(pDevIdRec->pRecords->descriptorCnt,
                                    sizeof(record_descriptors_t *));
    if (!pDevIdRec->pRecordDes)
    {
      printf("ERROR: pDevIdRec->pRecordDes malloc failed, size %d\n",
         sizeof(record_descriptors_t *) * pDevIdRec->pRecords->descriptorCnt);
      return -1;
    }
    for (int j=0; j<pDevIdRec->pRecords->descriptorCnt; ++j)
    {
      pDevIdRec->pRecordDes[j] = (record_descriptors_t *)(pFwPkgHdr->rawHdrBuf + subOffset);
      subOffset += (offsetof(record_descriptors_t, data) + pDevIdRec->pRecordDes[j]->length);
    }


    DBG_PRINT("DevRec[%d].length=%d, processed=%d\n", i,
                 pDevIdRec->pRecords->recordLength,
                 subOffset-*pOffset);
    printDevIdRec(pFwPkgHdr, i);

    // update offset for next deviceRecord
    *pOffset += pDevIdRec->pRecords->recordLength;
  }

  return 0;
}

// initialize "Component Image area" of pldm_fw_pkg_hdr_t structure,
//  i.e.  componentImageCnt
//        allocate array of pointers to each Component Image
//
// input: pFwPkgHdr with header and firmware device ID area preinitialized
//        pOffset : pointer to starting offset of "Component Image area"
//                  of the package
// output: pFwPkgHdr with component image area initialized
//         pOffset: updated to pointing to starting offset of header checksum
//
int
init_component_img_info(pldm_fw_pkg_hdr_t* pFwPkgHdr, int *pOffset)
{
  int i;

  pFwPkgHdr->componentImageCnt = pFwPkgHdr->rawHdrBuf[*pOffset];
  *pOffset += sizeof(pFwPkgHdr->componentImageCnt);
  DBG_PRINT("\n\n Number of Component in package (componentImageCnt) =%d\n",
              pFwPkgHdr->componentImageCnt);
  // allocate a look up table of pointers to each component image
  pFwPkgHdr->pCompImgInfo =
     (pldm_component_img_info_t **) calloc(pFwPkgHdr->componentImageCnt,
                                     sizeof(pldm_component_img_info_t *));
  if (!pFwPkgHdr->pCompImgInfo)
  {
    printf("ERROR: pFwPkgHdr->pCompImgInfo malloc failed, size %d\n",
       sizeof(pldm_component_img_info_t **) * pFwPkgHdr->componentImageCnt);
    return -1;
  }
  for (i=0; i<pFwPkgHdr->componentImageCnt; ++i)
  {
    pFwPkgHdr->pCompImgInfo[i] = (pldm_component_img_info_t *)(pFwPkgHdr->rawHdrBuf + *pOffset);
    printComponentImgInfo(pFwPkgHdr, i);

    // move pointer to next Component image, taking int account of variable
    //  version size
    *pOffset += offsetof(pldm_component_img_info_t, versionString) +
             pFwPkgHdr->pCompImgInfo[i]->versionStringLength;
  }

  return 0;
}


// free up all pointers allocated in pldm_fw_pkg_hdr_t *pFwPkgHdr
void
free_pldm_pkg_data(pldm_fw_pkg_hdr_t **pFwPkgHdr)
{
  if ((!pFwPkgHdr) || (!(*pFwPkgHdr)))
    return;

  if ((*pFwPkgHdr)->pDevIdRecs) {
    for (int i=0; i<(*pFwPkgHdr)->devIdRecordCnt; ++i) {
      if ( ((*pFwPkgHdr)->pDevIdRecs[i]) &&
           ((*pFwPkgHdr)->pDevIdRecs[i]->pRecordDes)) {
        free((*pFwPkgHdr)->pDevIdRecs[i]->pRecordDes);
      }

      if ((*pFwPkgHdr)->pDevIdRecs[i]) {
        free((*pFwPkgHdr)->pDevIdRecs[i]);
      }
    }
    free((*pFwPkgHdr)->pDevIdRecs);
  }

  if ((*pFwPkgHdr)->pCompImgInfo) {
    free((*pFwPkgHdr)->pCompImgInfo);
  }
  if ((*pFwPkgHdr)->rawHdrBuf) {
    free((*pFwPkgHdr)->rawHdrBuf);
  }
  free(*pFwPkgHdr);
  return;
}



pldm_fw_pkg_hdr_t *
pldm_parse_fw_pkg(char *path) {
  int ret = 0;
  int offset = 0;

  // firmware package header
  pldm_fw_pkg_hdr_t *pFwPkgHdr;

  // initialize pFwPkgHdr access pointer as fw package header contains
  // multiple variable size fields

  // header info area
  offset = 0;
  ret = init_pkg_hdr_info(path, &pFwPkgHdr, &offset);
  if (ret < 0) {
    goto error_exit;
  }

  // firmware device id area
  ret = init_device_id_records(pFwPkgHdr, &offset);
  if (ret < 0) {
    goto error_exit;
  }

  // component image info area
  ret = init_component_img_info(pFwPkgHdr, &offset);
  if (ret < 0) {
    goto error_exit;
  }

  // pkg header checksum
  pFwPkgHdr->pkgHdrChksum = *(uint32_t *)(pFwPkgHdr->rawHdrBuf+ offset);
  offset += sizeof(uint32_t);
  printf("\n\nPDLM Firmware Package Checksum=0x%x\n", pFwPkgHdr->pkgHdrChksum);

  if (pFwPkgHdr->phdrInfo->headerSize != offset) {
    printf("ERROR: header size(0x%x) and processed data (0x%x) mismatch\n",
            pFwPkgHdr->phdrInfo->headerSize, offset);
  }


  return pFwPkgHdr;

error_exit:
  if (pFwPkgHdr)
    free_pldm_pkg_data(&pFwPkgHdr);
  return 0;
}

static
int handlePldmReqFwData(pldm_fw_pkg_hdr_t *pkgHdr, pldm_cmd_req *pCmd, pldm_response *pldmRes)
{
  PLDM_RequestFWData_t *pReqDataCmd = (PLDM_RequestFWData_t *)pCmd->payload;
  // for now assumes  it's always component 0
  unsigned char *pComponent =
       (unsigned char*)(pkgHdr->rawHdrBuf + pkgHdr->pCompImgInfo[0]->locationOffset);
  uint32_t componentSize = pkgHdr->pCompImgInfo[0]->size;

  // calculate how much FW data is left to transfer and if any padding is needed
  int compBytesLeft = componentSize - pReqDataCmd->offset;
  int numPaddingNeeded = pReqDataCmd->length > compBytesLeft ?
                   (pReqDataCmd->length - compBytesLeft) : 0;



  printf("%s offset = 0x%x, length = 0x%x, compBytesLeft=%d, numPadding=%d\n",
         __FUNCTION__, pReqDataCmd->offset, pReqDataCmd->length, compBytesLeft,
         numPaddingNeeded);
  memcpy(pldmRes->common, pCmd->common, PLDM_COMMON_REQ_LEN);
  pldmRes->common[3] = CC_SUCCESS;  // hard code success for now, need to check length in the future


  if (numPaddingNeeded > 0) {
    printf("%s %d bytes padding added\n", __FUNCTION__, numPaddingNeeded);
    memcpy(pldmRes->response, pComponent + pReqDataCmd->offset, compBytesLeft);
    memset(pldmRes->response + compBytesLeft, 0, numPaddingNeeded);
  } else {
    memcpy(pldmRes->response, pComponent + pReqDataCmd->offset, pReqDataCmd->length);
  }
  pldmRes->resp_size = PLDM_COMMON_RES_LEN + pReqDataCmd->length;

  return 0;
}

static
int handlePldmFwTransferComplete(pldm_cmd_req *pCmd, pldm_response *pRes)
{
  PLDM_TransferComplete_t *pReqDataCmd = (PLDM_TransferComplete_t *)pCmd->payload;
  int ret = 0;

  if (pReqDataCmd->transferResult != 0) {
    printf("Error, transfer failed, err=%d\n", pReqDataCmd->transferResult);
    ret = -1;
  }

  memcpy(pRes->common, pCmd->common, PLDM_COMMON_REQ_LEN);
  pRes->common[3] = CC_SUCCESS;
  pRes->resp_size = PLDM_COMMON_RES_LEN;
  return ret;
}


static
int handlePldmVerifyComplete(pldm_cmd_req *pCmd, pldm_response *pRes)
{
  PLDM_VerifyComplete_t *pReqDataCmd = (PLDM_VerifyComplete_t *)pCmd->payload;
  int ret = 0;

  if (pReqDataCmd->verifyResult != 0) {
    printf("Error, firmware verify failed, err=0x%x\n", pReqDataCmd->verifyResult);
    ret = -1;
  }

  memcpy(pRes->common, pCmd->common, PLDM_COMMON_REQ_LEN);
  pRes->common[3] = CC_SUCCESS;
  pRes->resp_size = PLDM_COMMON_RES_LEN;
  return ret;
}

static
int handlePldmFwApplyComplete(pldm_cmd_req *pCmd, pldm_response *pRes)
{
  PLDM_ApplyComplete_t *pReqDataCmd = (PLDM_ApplyComplete_t *)pCmd->payload;
  int ret = 0;
  if (pReqDataCmd->applyResult != 0) {
    printf("Error, firmware apply failed, err=%d\n", pReqDataCmd->applyResult);
    ret = -1;
  }

  printf("Apply result = 0x%x, compActivationMethodsModification=0x%x\n",
          pReqDataCmd->applyResult, pReqDataCmd->compActivationMethodsModification);

  memcpy(pRes->common, pCmd->common, PLDM_COMMON_REQ_LEN);
  pRes->common[3] = CC_SUCCESS;
  pRes->resp_size = PLDM_COMMON_RES_LEN;
  return ret;
}

int pldmCmdHandler(pldm_fw_pkg_hdr_t *pkgHdr, pldm_cmd_req *pCmd, pldm_response *pRes)
{
  int cmd = pCmd->common[2];
  int ret = 0;

  // need to check cmd validity
  switch (cmd) {
    case CMD_REQUEST_FIRMWARE_DATA:
      ret = handlePldmReqFwData(pkgHdr, pCmd, pRes);
      break;
    case CMD_TRANSFER_COMPLETE:
      printf("handle CMD_TRANSFER_COMPLETE\n");
      dbgPrintCdb(pCmd);
      ret = handlePldmFwTransferComplete(pCmd, pRes);
      break;
    case CMD_VERIFY_COMPLETE:
      printf("handle CMD_VERIFY_COMPLETE\n");
      dbgPrintCdb(pCmd);
      ret = handlePldmVerifyComplete(pCmd, pRes);
      break;
    case CMD_APPLY_COMPLETE:
      printf("handle CMD_APPLY_COMPLETE\n");
      dbgPrintCdb(pCmd);
      ret = handlePldmFwApplyComplete(pCmd, pRes);
      break;
    default:
      printf("unkown cmd %d\n",cmd);
      dbgPrintCdb(pCmd);
      ret = -1;
      break;
  }
  return ret;
}


// takes NCSI cmd 0x56 response and returns
// PLDM opcode
int ncsiDecodePldmCmd(NCSI_NL_RSP_T *nl_resp,  pldm_cmd_req *pldmReq)
{
  // NCSI response contains at least 4 bytes of Reason code and
  //   Response code.
  if (nl_resp->hdr.payload_length <= 4) {
    // empty response, no PLDM payload
    return -1;
  }

  pldmReq->payload_size = nl_resp->hdr.payload_length - 4; // accout for response and reason code
  memcpy((char *)&(pldmReq->common[0]), (char *)&(nl_resp->msg_payload[4]),
         pldmReq->payload_size);

  // PLDM cmd byte is the 3rd byte of PLDM header
  return pldmReq->common[2];
}

int ncsiDecodePldmCompCode(NCSI_NL_RSP_T *nl_resp)
{
  // NCSI response contains at least 4 bytes of Reason code and
  //   Response code.  PLDM response contains at least 4 bytes header
  if (nl_resp->hdr.payload_length < 8) {
    // empty response, no PLDM payload/or payload incomplete
    return -1;
  }

  // Completion Code is in 4th byte of PLDM header
  return nl_resp->msg_payload[7];
}
