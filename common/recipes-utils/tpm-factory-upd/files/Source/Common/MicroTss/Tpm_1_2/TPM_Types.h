/**
 *  @brief      Declares definitions for TPM types and structures
 *  @details
 *  @file       TPM_Types.h
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "StdInclude.h"
#include "TPM2_Types.h"

// Maximum sizes for dynamic command response structure elements
// (Values are arbitrary and may be changed if necessary.)
#define ST_EL_BIG_SZ    100
#define ST_EL_SML_SZ    50

/// Typedef and defines for TPM_TAG
typedef TSS_UINT16                  TSS_TPM_TAG;
#define TSS_TPM_TAG_RQU_COMMAND         (TSS_TPM_TAG) 0x00C1
#define TSS_TPM_TAG_RQU_AUTH1_COMMAND   (TSS_TPM_TAG) 0x00C2    // An authenticated command with one authentication handle
#define TSS_TPM_TAG_RSP_COMMAND         (TSS_TPM_TAG) 0x00C4

/// Typedef and defines for TPM_COMMAND_CODE
typedef TSS_UINT32                          TSS_TPM_COMMAND_CODE;
#define TSS_TPM_ORD_OIAP                    (TSS_TPM_COMMAND_CODE) 0x0000000A
#define TSS_TPM_ORD_OSAP                    (TSS_TPM_COMMAND_CODE) 0x0000000B
#define TSS_TPM_ORD_TakeOwnership           (TSS_TPM_COMMAND_CODE) 0x0000000D
#define TSS_TPM_ORD_ChangeAuthOwner         (TSS_TPM_COMMAND_CODE) 0x00000010
#define TSS_TPM_ORD_SetCapability           (TSS_TPM_COMMAND_CODE) 0x0000003F
#define TSS_TPM_ORD_GetTestResult           (TSS_TPM_COMMAND_CODE) 0x00000054
#define TSS_TPM_ORD_OwnerClear              (TSS_TPM_COMMAND_CODE) 0x0000005B
#define TSS_TPM_ORD_GetCapability           (TSS_TPM_COMMAND_CODE) 0x00000065
#define TSS_TPM_ORD_ReadPubEK               (TSS_TPM_COMMAND_CODE) 0x0000007C
#define TSS_TPM_ORD_OwnerReadInternalPub    (TSS_TPM_COMMAND_CODE) 0x00000081
#define TSS_TPM_ORD_Startup                 (TSS_TPM_COMMAND_CODE) 0x00000099
#define TSS_TPM_ORD_FlushSpecific           (TSS_TPM_COMMAND_CODE) 0x000000BA
#define TSS_TSC_ORD_PhysicalPresence        (TSS_TPM_COMMAND_CODE) 0x4000000A

/// Defines for TPM Protocol types
#define TSS_TPM_PID_OWNER       0x0005

/// Defines for TPM_RESOURCE_TYPE
#define TSS_TPM_RT_AUTH             0x00000002 ///< The handle is an authorization handle. Authorization handles come from TPM_OIAP, TPM_OSAP and TPM_DSAP

/// Typedef for TPM_RESULT
typedef TSS_UINT32              TSS_TPM_RESULT;
#define TSS_TPM_BASE            0x00000000
#define TSS_TPM_NON_FATAL       0x800
/// Self-test has failed and the TPM has shutdown
#define TSS_TPM_AUTHFAIL            ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x01))
#define TSS_TPM_BAD_PARAMETER       ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x03))
#define TSS_TPM_DEACTIVATED         ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x06))
#define TSS_TPM_DISABLED            ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x07))
#define TSS_TPM_FAIL                ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x09))
#define TSS_TPM_BAD_ORDINAL         ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x0A))
#define TSS_TPM_BAD_PARAM_SIZE      ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x19))
#define TSS_TPM_NOSRK               ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x12))
#define TSS_TPM_RESOURCES           ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x15))
#define TSS_TPM_FAILEDSELFTEST      ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x1C))
#define TSS_TPM_BADTAG              ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x1E))
#define TSS_TPM_INVALID_POSTINIT    ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x26))
#define TSS_TPM_BAD_PRESENCE        ((TSS_TPM_RESULT)(TSS_TPM_BASE + 0x2D))
#define TSS_TPM_DEFEND_LOCK_RUNNING ((TSS_TPM_RESULT)(TSS_TPM_BASE + TSS_TPM_NON_FATAL + 0x003))

/// Typedef and defines for TPM_STARTUP_TYPE
typedef TSS_UINT16                  TSS_TPM_STARTUP_TYPE;
#define TSS_TPM_ST_CLEAR            (TSS_TPM_STARTUP_TYPE) 0x0001
#define TSS_TPM_ST_STATE            (TSS_TPM_STARTUP_TYPE) 0x0002
#define TSS_TPM_ST_DEACTIVATED      (TSS_TPM_STARTUP_TYPE) 0x0003

/// Typedef and defines for TPM_CAPABILITY_AREA
typedef TSS_UINT32                  TSS_TPM_CAPABILITY_AREA;
#define TSS_TPM_CAP_FLAG            0x00000004
#define TSS_TPM_CAP_PROPERTY        0x00000005
#define TSS_TPM_CAP_DA_LOGIC        0x00000019
#define TSS_TPM_CAP_VERSION_VAL     0x0000001A
#define TSS_TPM_CAP_FLAG_PERMANENT  0x00000108
#define TSS_TPM_CAP_FLAG_VOLATILE   0x00000109
#define TSS_TPM_CAP_PROP_OWNER      0x00000111
#define TSS_TPM_SET_PERM_FLAGS      0x00000001

/// Defines for Permanent Flags
#define TSS_TPM_PF_READPUBEK        0x00000004
#define TSS_TPM_TAG_PERMANENT_FLAGS 0x001f
#define TSS_TPM_TAG_STCLEAR_FLAGS   0x0020

/// Defines for TPM_SetCapability
#define TSS_TPM_SET_STCLEAR_DATA                0x00000004 ///< The ability to set a value is field specific and a review of the structure will disclose the ability and requirements to set a value
#define TSS_TPM_SD_DEFERREDPHYSICALPRESENCE     0x00000006

/// Defines for TPM_PHYSICAL_PRESENCE
#define TSS_TPM_PHYSICAL_PRESENCE_CMD_ENABLE    0x0020 ///< Sets the physicalPresenceCMDEnable to TRUE
#define TSS_TPM_PHYSICAL_PRESENCE_PRESENT       0x0008 ///< Sets PhysicalPresence = TRUE
#define TSS_TPM_PHYSICAL_PRESENCE_NOTPRESENT    0x0010 ///< Sets PhysicalPresence = FALSE
#define TSS_TPM_PHYSICAL_PRESENCE_LOCK          0x0004 ///< Sets PhysicalPresenceLock = TRUE

/// Defines for FieldUpgradeInfoRequest
#define OWNER_SET_FLAG      0x0001

/// Defines for TPM_KEY_USAGE
#define TSS_TPM_KEY_STORAGE     0x0011

/// Defines for TPM_AUTH_DATA_USAGE
#define TSS_TPM_AUTH_ALWAYS     0x01

/// Defines for TPM_SIG_SCHEME
#define TSS_TPM_SS_NONE         0x0001

/// Defines for dictionary attack
#define TSS_TPM_ET_XOR              0x00
#define TSS_TPM_ET_OWNER            0x02
#define TSS_TPM_TAG_DA_INFO         0x37
#define TSS_TPM_TAG_DA_INFO_LIMITED 0x38
#define TSS_TPM_DA_STATE_INACTIVE   0x00 ///< The dictionary attack mitigation logic is currently inactive
#define TSS_TPM_DA_STATE_ACTIVE     0x01 ///< The dictionary attack mitigation logic is active. TPM_DA_ACTION_TYPE (21.10) is in progress.

#define TSS_TPM_KH_OWNER            0x40000001 ///< The handle points to the TPM Owner
#define TSS_TPM_KH_EK               0x40000006 ///< The handle points to the PUBEK, only usable with TPM_OwnerReadInternalPub

#define TSS_TPM_PID_ADCP            0X0004 ///< The ADCP protocol.

/// Typedef for TPM_STRUCTURE_TAG;
typedef TSS_UINT16 TSS_TPM_STRUCTURE_TAG;

/// Typedef for TPM_VERSION_BYTE;
typedef TSS_BYTE TSS_TPM_VERSION_BYTE;

/// Typedef for TPM_AUTHHANDLE.
/// Handle to an authorization session
typedef TSS_UINT32 TSS_TPM_AUTHHANDLE;

/// Typedef for TPM_RESOURCE_TYPE.
/// The types of resources that a TPM may have using internal resources
typedef TSS_UINT32 TSS_TPM_RESOURCE_TYPE;

/// Typedef for TPM_ENC_SCHEME.
/// The definition of the encryption scheme
typedef TSS_UINT16 TSS_TPM_ENC_SCHEME;

/// Typedef for TPM_SIG_SCHEME
/// The definition of the signature scheme
typedef TSS_UINT16 TSS_TPM_SIG_SCHEME;

/// Typedef for TPM_PROTOCOL_ID
/// The protocol in use
typedef TSS_UINT16 TSS_TPM_PROTOCOL_ID;

/// Typedef for TPM_AUTH_DATA_USAGE
/// When authorization is required for an entity
typedef TSS_BYTE TSS_TPM_AUTH_DATA_USAGE;

/// Typedef for TPM_KEY_FLAGS
typedef TSS_UINT32 TSS_TPM_KEY_FLAGS;

/// Typedef for TPM_KEY_USAGE
/// Indicates the permitted usage of the key. See 4.16.6
typedef TSS_UINT16 TSS_TPM_KEY_USAGE;

/// Typedef for TPM_ENTITY_TYPE
typedef TSS_UINT16 TSS_TPM_ENTITY_TYPE;

/// Typedef for TPM_DA_STATE
typedef TSS_BYTE TSS_TPM_DA_STATE;

// Typedef for TPM_KEY_HANDLE
typedef TSS_UINT32 TSS_TPM_KEY_HANDLE;
/**
 *  @brief      TPM_VERSION structure
 *  @details    TPM_VERSION structure definition
 *
 **/
typedef struct tdTSS_TPM_VERSION
{
    TSS_TPM_VERSION_BYTE major;
    TSS_TPM_VERSION_BYTE minor;
    TSS_BYTE revMajor;
    TSS_BYTE revMinor;
} TSS_TPM_VERSION;

typedef TSS_TPM_VERSION TSS_TCPA_VERSION;

/**
 *  @brief      TPM_CAP_VERSION_INFO structure
 *  @details    TPM_CAP_VERSION_INFO structure definition
 *
 **/
typedef struct tdTSS_TPM_CAP_VERSION_INFO
{
    TSS_TPM_STRUCTURE_TAG tag;
    TSS_TPM_VERSION version;
    TSS_UINT16 specLevel;
    TSS_BYTE specLetter;
    TSS_BYTE tpmVendorID[4];
    TSS_UINT16 vendorSpecificSize;
    TSS_BYTE vendorSpecific[ST_EL_SML_SZ];  // Size_is(vendorSpecificSize)
} TSS_TPM_CAP_VERSION_INFO;

/**
 *  @brief      TPM_DA_ACTION_TYPE structure
 *  @details    TPM_DA_ACTION_TYPE structure definition
 *
 **/
typedef struct tdTSS_TPM_DA_ACTION_TYPE
{
    TSS_TPM_STRUCTURE_TAG tag;
    TSS_UINT32 actions;
} TSS_TPM_DA_ACTION_TYPE;

/**
 *  @brief      TPM_DA_INFO structure
 *  @details    TPM_DA_INFO structure definition
 *
 **/
typedef struct tdTSS_TPM_DA_INFO
{
    TSS_TPM_STRUCTURE_TAG tag;
    TSS_TPM_DA_STATE state;
    TSS_UINT16 currentCount;
    TSS_UINT16 thresholdCount;
    TSS_TPM_DA_ACTION_TYPE actionAtThreshold;
    TSS_UINT32 actionDependValue;
    TSS_UINT32 vendorDataSize;
    TSS_BYTE vendorData[ST_EL_BIG_SZ];
} TSS_TPM_DA_INFO;

/// Size of a TSS_TPM_NONCE
#define TPM_NONCE_SIZE  20

/**
 *  @brief      TPM_NONCE structure
 *  @details    TPM_NONCE structure definition
 *
 **/
typedef struct tdTSS_TPM_NONCE
{
    TSS_BYTE nonce[TPM_NONCE_SIZE];
} TSS_TPM_NONCE;

/**
 *  @brief      TPM_PERMANENT_FLAGS structure
 *  @details    TPM_PERMANENT_FLAGS structure definition (according to spec revision >=103)
 *
 **/
typedef struct tdTSS_TPM_PERMANENT_FLAGS
{
    TSS_TPM_STRUCTURE_TAG tag;
    TSS_BYTE disable;                       // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE ownership;                     // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE deactivated;                   // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE readPubek;                     // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE disableOwnerClear;             // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE allowMaintenance;              // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE physicalPresenceLifetimeLock;  // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE physicalPresenceHWEnable;      // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE physicalPresenceCMDEnable;     // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE CEKPUsed;                      // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE TPMpost;                       // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE TPMpostLock;                   // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE FIPS;                          // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE bOperator;                     // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE enableRevokeEK;                // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE nvLocked;                      // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE readSRKPub;                    // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE tpmEstablished;                // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE maintenanceDone;               // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE disableFullDALogicInfo;        // Actually: BOOL value using 1 Byte of memory
} TSS_TPM_PERMANENT_FLAGS;

/**
 *  @brief      TPM_STCLEAR_FLAGS structure
 *  @details    TPM_STCLEAR_FLAGS structure definition (according to spec revision 116)
 **/
typedef struct tdTSS_TPM_STCLEAR_FLAGS
{
    TSS_TPM_STRUCTURE_TAG tag;
    TSS_BYTE deactivated;                   // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE disableForceClear;             // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE physicalPresence;              // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE physicalPresenceLock;          // Actually: BOOL value using 1 Byte of memory
    TSS_BYTE bGlobalLock;                   // Actually: BOOL value using 1 Byte of memory
} TSS_TPM_STCLEAR_FLAGS;

/// Size of a TSS_TPM_AUTHDATA
#define TPM_AUTHDATA_SIZE   20

/**
 *  @brief      TPM_AUTHDATA structure
 *  @details    TPM_AUTHDATA structure definition
 *
 **/
typedef struct tdTSS_TPM_AUTHDATA
{
    TSS_BYTE authdata[TPM_AUTHDATA_SIZE];
} TSS_TPM_AUTHDATA;

/// Typedef for TPM_ENCAUTH
typedef TSS_TPM_AUTHDATA TSS_TPM_ENCAUTH;

/**
 *  @brief      TPM_AUTH_IN structure
 *  @details    TPM_AUTH_IN structure definition
 *
 **/
typedef struct tdTSS_TPM_AUTH_IN
{
    TSS_TPM_AUTHHANDLE dwAuthHandle;
    TSS_TPM_NONCE sNonceOdd;
    TSS_BYTE bContinueAuthSession; // Actually: BOOL value using 1 Byte of memory
    TSS_TPM_AUTHDATA sOwnerAuth;
} TSS_TPM_AUTH_IN;

/**
 *  @brief      TPM_RSA_KEY_PARMS structure
 *  @details    TPM_RSA_KEY_PARMS structure definition
 *
 **/
typedef struct tdTSS_TPM_RSA_KEY_PARMS
{
    TSS_UINT32 keyLength;
    TSS_UINT32 numPrimes;
    TSS_UINT32 exponentSize;
} TSS_TPM_RSA_KEY_PARMS;

/**
 *  @brief      TPM_KEY_PARMS structure
 *  @details    TPM_KEY_PARMS structure definition
 *
 **/
typedef struct tdTSS_TPM_KEY_PARMS
{
    TSS_TPM_ALGORITHM_ID algorithmID;
    TSS_TPM_ENC_SCHEME encScheme;
    TSS_TPM_SIG_SCHEME sigScheme;
    TSS_UINT32 parmSize;
    // Only for RSA-2048! actually: parms; size_is(parmSize)
    TSS_TPM_RSA_KEY_PARMS parms;
} TSS_TPM_KEY_PARMS;

/// Public Endorsement Key length in Bytes
// IMPORTANT: KEY_LEN is NOT a max value, but the RSA-2048 key length!
#define KEY_LEN 256

/**
 *  @brief      TPM_STORE_PUBKEY structure
 *  @details    TPM_STORE_PUBKEY structure definition
 *
 **/
typedef struct tdTSS_TPM_STORE_PUBKEY
{
    TSS_UINT32 keyLength;
    TSS_BYTE key[KEY_LEN];  // Actually: size_is(keyLength)
} TSS_TPM_STORE_PUBKEY;

/**
 *  @brief      TPM_PUBKEY structure
 *  @details    TPM_PUBKEY structure definition
 *
 **/
typedef struct tdTSS_TPM_PUBKEY
{
    TSS_TPM_KEY_PARMS algorithmParms;
    TSS_TPM_STORE_PUBKEY pubKey;
} TSS_TPM_PUBKEY;

/**
 *  @brief      TPM_STRUCT_VER structure
 *  @details    TPM_STRUCT_VER structure definition
 *
 **/
typedef struct tdTSS_TPM_STRUCT_VER
{
    TSS_BYTE major;
    TSS_BYTE minor;
    TSS_BYTE revMajor;
    TSS_BYTE revMinor;
} TSS_TPM_STRUCT_VER;

/**
 *  @brief      TPM_KEY structure
 *  @details    TPM_KEY structure definition
 *
 **/
typedef struct tdTSS_TPM_KEY
{
    TSS_TPM_STRUCT_VER ver;
    TSS_TPM_KEY_USAGE keyUsage;
    TSS_TPM_KEY_FLAGS keyFlags;
    TSS_TPM_AUTH_DATA_USAGE authDataUsage;
    TSS_TPM_KEY_PARMS algorithmParms;
    TSS_UINT32 PCRInfoSize;
//  PCRInfo; empty for unbound keys // Actually: size_is(PCRInfoSize)
    TSS_TPM_STORE_PUBKEY pubKey;
    TSS_UINT32 encSize;
    TSS_BYTE encData[ST_EL_BIG_SZ];
} TSS_TPM_KEY;

/**
 *  @brief      IFX_FIELDUPGRADEINFO structure definition
 **/
typedef struct tdIFX_FIELDUPGRADEINFO
{
    uint8_t internal1;
    uint8_t internal2[4];
    TSS_TCPA_VERSION internal3;
    uint8_t internal4[3];
    uint16_t wMaxDataSize;
    uint16_t internal6;
    uint8_t internal7;
    uint8_t internal8;
    uint16_t wFlagsFieldUpgrade;
} TSS_IFX_FIELDUPGRADEINFO;
