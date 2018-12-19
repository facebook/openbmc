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


static uint8_t pldm_iid = 0; // technically only 5 bits as per DSP0240 v1.0.0


/* PLDM Firmware Update command name*/
const char *pldm_cmd_string[NUM_PLDM_UPDATE_CDMS] = {
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

/* PLDM Firmware Update command name*/
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


/* PLDM Firmware Update command name*/
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
  DBG_PRINT("%s, common=0x%x, payload:\n", __FUNCTION__, cdb->common);
  for (int i=0; i<cdb->payload_size; ++i)
    DBG_PRINT("0x%x ", cdb->payload[i]);
  DBG_PRINT("\n");
}


uint32_t genCommonFields(PldmFWCmds cmd)
{
  uint32_t ret;
  uint8_t iid = pldm_iid++;
  ret =   (PLDM_CM_RQ | ((iid<<24) & PLDM_CM_IID_MASK))
        | (PLDM_TYPE_FIRMWARE_UPDATE << 16)
        | ((cmd << 8) & PLDM_CM_CMD_MASK);
  return ret;
}


int pldmRequestUpdateOp()
{
  pldm_cmd_req cdb;
  PLDM_RequestUpdate_t *pCmd = (PLDM_RequestUpdate_t *)&(cdb.payload);

  cdb.common = genCommonFields(CMD_REQUEST_UPDATE);
  cdb.payload_size = sizeof(PLDM_RequestUpdate_t);
  pCmd->maxTransferSize = 256;
  pCmd->numComponents = 1;
  pCmd->maxOutstandingTransferRequests = 1;

  dbgPrintCdb(&cdb);
  return 0;
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
  // temp buffer to store hdr info
  pldm_fw_pkg_hdr_info_t hdr_info;


  // Open the file exclusively for read
  fp = fopen(path, "r");
  if (!fp) {
    printf("ERROR: invalid file path :%s!\n", path);
    return -1;
  }

  stat(path, &buf);
  size = buf.st_size;
  printf("size of file is %d bytes\n", size);

  // read partial hdr info, enough to  check pkg uuid and get total header size
  rcnt = fread(&hdr_info, 1, offsetof(pldm_fw_pkg_hdr_info_t, versionString), fp);
  DBG_PRINT("Read hdr_info, read  %d bytes\n", rcnt);
  if (rcnt < offsetof(pldm_fw_pkg_hdr_info_t, versionString)) {
    printf("ERROR: hdr read, rcnt(%d) < %d", rcnt,
           offsetof(pldm_fw_pkg_hdr_info_t, versionString));
    fclose(fp);
    return -1;
  }

  // allocate pointer structure to access fw pkg header
  *pFwPkgHdr = (pldm_fw_pkg_hdr_t *)calloc(1, sizeof(pldm_fw_pkg_hdr_t));
  if (!(*pFwPkgHdr)) {
    printf("ERROR: pFwPkgHdr malloc failed, size %d\n", sizeof(pldm_fw_pkg_hdr_t));
    fclose(fp);
    return -1;
  }

  // allocate actual buffer to store fw package header
  (*pFwPkgHdr)->rawHdrBuf = (unsigned char *)calloc(1, hdr_info.headerSize);
  if (!(*pFwPkgHdr)->rawHdrBuf) {
    printf("ERROR: rawHdrBuf malloc failed, size %d\n", hdr_info.headerSize);
    fclose(fp);
    return -1;
  }

  // Move the file pointer to the start and read the entire package header
  fseek(fp, 0, SEEK_SET);
  rcnt = fread((*pFwPkgHdr)->rawHdrBuf, 1, hdr_info.headerSize, fp);
  if (rcnt < hdr_info.headerSize) {
    printf("ERROR: rawHdrBuf read, rcnt(%d) < %d", rcnt,
           hdr_info.headerSize);
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
int
free_pldm_pkg_data(pldm_fw_pkg_hdr_t **pFwPkgHdr)
{
  if ((!pFwPkgHdr) || (!(*pFwPkgHdr)))
    return 0;

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
  return 0;
}



int
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

error_exit:
  if (pFwPkgHdr)
    free_pldm_pkg_data(&pFwPkgHdr);
  return ret;
}
