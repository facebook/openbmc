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
#ifndef _PLDM_FW_UPDATE_H_
#define _PLDM_FW_UPDATE_H_

#include "pldm_base.h"
// defines data structure specified in DSP0267 v1.0.1 PLDM Firmware Update spec

#define MAX_VERSION_STRING_LEN 255
#define PLDM_FW_UUID_LEN 16

// fw update cmds completion codes
#define CC_FW_UPDATE_BASE          0x80
#define CC_NOT_IN_UPDATE_MODE      (CC_FW_UPDATE_BASE + 0)
#define CC_ALREADY_IN_UPDATE_MODE  (CC_FW_UPDATE_BASE + 1)
#define CC_DATA_OUT_OF_RANGE       (CC_FW_UPDATE_BASE + 2)
#define CC_INVALID_TRANSFER_LENTH  (CC_FW_UPDATE_BASE + 3)
#define CC_INVALID_STATE_FOR_CMD   (CC_FW_UPDATE_BASE + 4)
#define CC_INCOMPLETE_UPDATE       (CC_FW_UPDATE_BASE + 5)
#define CC_BUSY_IN_BACKGROUND      (CC_FW_UPDATE_BASE + 6)
#define CC_CANCEL_PENDING          (CC_FW_UPDATE_BASE + 7)
#define CC_COMMAND_NOT_EXPECTED    (CC_FW_UPDATE_BASE + 8)
#define CC_RETRY_REQUEST_FW_DATA   (CC_FW_UPDATE_BASE + 9)
#define CC_UNABLE_TO_INITIATE_UPDATE (CC_FW_UPDATE_BASE + 0xA)
#define CC_ACTIVATION_NOT_REQUIRED (CC_FW_UPDATE_BASE + 0xB)
#define CC_SELF_CONTAINED_ACTIVATION_NOT_PEMITTED (CC_FW_UPDATE_BASE + 0xC)
#define CC_NO_DEVICE_METADATA           (CC_FW_UPDATE_BASE + 0xD)
#define CC_RETRY_REQUEST_UPDATE         (CC_FW_UPDATE_BASE + 0xE)
#define CC_NO_PACKAGE_DATA              (CC_FW_UPDATE_BASE + 0xF)
#define CC_INVALID_DATA_TRANSFER_HANDLE (CC_FW_UPDATE_BASE + 0x10)
#define CC_INVALID_TRANSFER_OPERATION_FLAG (CC_FW_UPDATE_BASE + 0x11)

#define NUM_FW_UPDATE_CC (CC_INVALID_TRANSFER_OPERATION_FLAG - \
  CC_NOT_IN_UPDATE_MODE + 1)


// PLDM Firmware update commands
typedef enum pldm_fw_cmds {
  CMD_REQUEST_UPDATE          = 0x10,
  CMD_GET_PACKAGE_DATA        = 0x11,
  CMD_GET_DEVICE_METADATA     = 0x12,
  CMD_PASS_COMPONENT_TABLE    = 0x13,
  CMD_UPDATE_COMPONENT        = 0x14,
  CMD_REQUEST_FIRMWARE_DATA   = 0x15,
  CMD_TRANSFER_COMPLETE       = 0x16,
  CMD_VERIFY_COMPLETE         = 0x17,
  CMD_APPLY_COMPLETE          = 0x18,
  CMD_GET_META_DATA           = 0x19,
  CMD_ACTIVATE_FIRMWARE       = 0x1a,
  CMD_GET_STATUS              = 0x1b,
  CMD_CANCEL_UPDATE_COMPONENT = 0x1c,
  CMD_CANCEL_UPDATE           = 0x1d
} PldmFWCmds;
#define NUM_PLDM_UPDATE_CDMS 14
extern const char *pldm_fw_cmd_string[NUM_PLDM_UPDATE_CDMS];


// Component Classification values   (DSP0267, Table 19)
typedef enum pldm_comp_class {
  C_CLASS_UNKNOWN         = 0,
  C_CLASS_OTHER           = 1,
  C_CLASS_DRIVER          = 2,
  C_CLASS_CONFIG_SW       = 3,
  C_CLASS_APPLICATION_SW  = 4,
  C_CLASS_INSTRUMENTATION = 5,
  C_CLASS_FIRMWARE_BIOS   = 6,
  C_CLASS_DIAG_SW         = 7,
  C_CLASS_OS              = 8,
  C_CLASS_MIDDLEWARE      = 9,
  C_CLASS_FIRMWARE        = 10,
  C_CLASS_BIOS_FCODE      = 11,
  C_CLASS_SUPPORT_SERVICE_PACK = 12,
  C_CLASS_SOFTWARE_BUNDLE = 13,
} PldmComponentClass;
#define NUM_COMP_CLASS 14
extern const char *pldm_comp_class_name[NUM_COMP_CLASS];

// Version String type values  (DSP0267, Table 20)
typedef enum fw_string_type {
  str_UNKNOWN = 0,
  str_ASCII = 1,
  str_UTF8 = 2,
  str_UTF16 = 3,
  str_UTF16LE = 4,
  str_UTF16BE = 5,
} fwStrType;
#define NUM_PLDM_FW_STR_TYPES 6
extern const char *pldm_str_type_name[NUM_PLDM_FW_STR_TYPES];



typedef struct {
  uint8_t      uuid[PLDM_FW_UUID_LEN];
  uint8_t      headerRevision;
  uint16_t     headerSize;
  timestamp104 releaseDateTime;
  uint16_t     componentBitmapBitLength;
  uint8_t      versionStringType;
  uint8_t      versionStringLength;
  char         versionString[MAX_VERSION_STRING_LEN];
} __attribute__((packed)) pldm_fw_pkg_hdr_info_t;



typedef struct {
  uint16_t   recordLength;
  uint8_t    descriptorCnt;
  uint32_t   devUpdateOptFlags;
  uint8_t    compImgSetVersionStringType;
  uint8_t    compImgSetVersionStringLength;
  uint16_t   fwDevPkgDataLength;
} __attribute__((packed)) pldm_fw_dev_id_records_fixed_len_t;

#define PDLM_FW_MAX_DESCRIPTOR_DATA_LEN 128
typedef struct {
  uint16_t type;
  uint16_t length;
  char     data[PDLM_FW_MAX_DESCRIPTOR_DATA_LEN];
} __attribute__((packed)) record_descriptors_t;


// Firmware device ID record, defined in DSP0267 Table 4
// Each record contains
//      - fixed length portion (11 bytes)
//      - variable length portion:
//          - component bitfield (vraiable length, defined in pkg header info)
//          - version string (up to 255 bytes, length defined in record)
//          - record descriptors (1 or more)
//          - firmware device package data (0 or more bytes)
typedef struct {
  // pointer to fixed length portion of the records
  pldm_fw_dev_id_records_fixed_len_t *pRecords;

  // pointer to component bitmap, variable length (defined in hdr info)
  uint8_t    *pApplicableComponents;

  // pointer to version string, length defined in pRecords
  unsigned char *versionString;

  // array of record descriptors, number of descriptors specified in pRecords
  record_descriptors_t **pRecordDes;

  // pointer to fw device package data (0 or more bytes, defined in pRecords)
  char *pDevPkgData;
} __attribute__((packed)) pldm_fw_dev_id_records_t;





// Component image Information, defined in DSP0267 Table 5
// Most of this record is fixed length except for the version string
typedef struct {
  uint16_t   class;
  uint16_t   id;
  uint32_t   compStamp;
  uint16_t   options;
  uint16_t   requestedActivationMethod;
  uint32_t   locationOffset;
  uint32_t   size;
  uint8_t    versionStringType;
  uint8_t    versionStringLength;
  char       versionString[MAX_VERSION_STRING_LEN];
} __attribute__((packed)) pldm_component_img_info_t;


// defines PLDM firmwar package header structure,
//  figure 5 on DSP0267 1.0.0
//  detailed layout in Table 3
typedef struct {
  unsigned char *rawHdrBuf; // unparsed hdr buffer data bytes

  // Use pointers for acccessing/interpreting hdr buffer above

  // header info area
  pldm_fw_pkg_hdr_info_t *phdrInfo;

  // firmware device id area
  uint8_t devIdRecordCnt;
  pldm_fw_dev_id_records_t **pDevIdRecs; // array of pointer to device id records

  // component image information area
  uint16_t componentImageCnt;
  pldm_component_img_info_t **pCompImgInfo; // array of pointer to component images info

  // package header checksum area
  uint32_t pkgHdrChksum;
} __attribute__((packed)) pldm_fw_pkg_hdr_t;

#define PLDM_MAX_XFER_SIZE 512
#define PLDM_MAX_XFER_CNT  1


// cdb for pldm cmd 0x10
typedef struct {
	uint32_t maxTransferSize;
  uint16_t numComponents;
  uint8_t  maxOutstandingTransferRequests;
  uint16_t packageDataLength;
  uint8_t  componentImageSetVersionStringType;
  uint8_t  componentImageSetVersionStringLength;
  char     componentImageSetVersionString[MAX_VERSION_STRING_LEN];
} __attribute__((packed)) PLDM_RequestUpdate_t;

typedef struct {
  uint8_t completionCode;
  uint16_t firmwareDeviceMetaDataLength;
  uint8_t fdWillSendGetPackageDataCommand;
} __attribute__((packed)) PLDM_RequestUpdate_Response_t;


// cdb for pldm cmd 0x13 Pass Component Table
#define TFLAG_START 0x01
#define TFLAG_MID   0x02
#define TFLAG_END   0x04
#define TFLAG_STATRT_END 0x05
typedef struct {
  uint8_t  transferFlag;
  uint16_t class;
  uint16_t id;
  uint8_t  classIndex;
  uint32_t compStamp;
  uint8_t  versionStringType;
  uint8_t  versionStringLength;
  char     versionString[MAX_VERSION_STRING_LEN];
} __attribute__((packed)) PLDM_PassComponentTable_t;

typedef struct {
  uint8_t completionCode;
  uint8_t componentResponse;
  uint8_t componentResponseCode;
} __attribute__((packed)) PLDM_PassComponentTable_Response_t;



// cdb for pldm cmd 0x14 Update Component
typedef struct {
  uint16_t class;
  uint16_t id;
  uint8_t  classIndex;
  uint32_t compStamp;
  uint32_t compSize;
  uint32_t updateOptions;
  uint8_t  versionStringType;
  uint8_t  versionStringLength;
  char     versionString[MAX_VERSION_STRING_LEN];
} __attribute__((packed)) PLDM_UpdateComponent_t;

typedef struct {
  uint8_t completionCode;
  uint8_t componentCompatibilityResponse;
  uint8_t componentCompatibilityResponseCode;
  uint32_t updateOptionFlagEnabled;
  uint16_t estimatedTimeBeforeSendingRequestFWdata;
} __attribute__((packed)) PLDM_UpdateComponent_Response_t;


// cdb for pldm cmd 0x15 request FW data
typedef struct {
  uint32_t offset;
  uint32_t length;
} __attribute__((packed)) PLDM_RequestFWData_t;

typedef struct {
  uint8_t completionCode;
  unsigned char *imageBuf;
} __attribute__((packed)) PLDM_RequestFWData_Response_t;


// cdb for pldm cmd 0x16 transfer complete
typedef struct {
  uint8_t transferResult;
} __attribute__((packed)) PLDM_TransferComplete_t;

typedef struct {
  uint8_t completionCode;
} __attribute__((packed)) PLDM_TransferComplete_Response_t;


// cdb for pldm cmd 0x17 verify complete
typedef struct {
  uint8_t verifyResult;
} __attribute__((packed)) PLDM_VerifyComplete_t;

typedef struct {
  uint8_t completionCode;
} __attribute__((packed)) PLDM_VerifyComplete_Response_t;


// cdb for pldm cmd 0x18 apply complete
typedef struct {
  uint8_t applyResult;
  uint16_t compActivationMethodsModification;
} __attribute__((packed)) PLDM_ApplyComplete_t;

typedef struct {
  uint8_t completionCode;
} __attribute__((packed)) PLDM_ApplyComplete_Response_t;


// cdb for pldm cmd 0x1a activate fw
typedef struct {
  uint8_t selfContainedActivationRequest;
} __attribute__((packed)) PLDM_ActivateFirmware_t;

typedef struct {
  uint8_t completionCode;
  uint16_t estimated;
} __attribute__((packed)) PLDM_ActivateFirmware_Response_t;

// UUID signature specifying package supports PLDM FW Update
const char PLDM_FW_UUID[PLDM_FW_UUID_LEN] = {
            0xf0, 0x18, 0x87, 0x8c, 0xcb, 0x7d, 0x49, 0x43,
            0x98, 0x00, 0xa0, 0x2f, 0x05, 0x9a, 0xca, 0x02
        };

#endif
