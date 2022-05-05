/**
 *	@brief		Declares types and definitions for field upgrade
 *	@file		TPM2_FieldUpgradeTypes.h
 *	@copyright	Copyright 2013 - 2018 Infineon Technologies AG ( www.infineon.com )
 *
 *	@copyright	All rights reserved.
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

/********************************************************************************
 * Field upgrade value definitions
 ********************************************************************************/
/// TPM2.0 command codes
#define		TPM2_CC_FieldUpgradeStartVendor	(TSS_TPM_CC)(0x2000012F)

/// TPM1.2 command codes
#define		TPM_CC_FieldUpgradeCommand		0x000000AA

/// FieldUpgrade sub commands
#define		TPM_FieldUpgradeInfoRequest		0x10
#define		TPM_FieldUpgradeInfoRequest2	0x11
#define		TPM_FieldUpgradeStart			0x34
#define		TPM_FieldUpgradeUpdate			0x31
#define		TPM_FieldUpgradeComplete		0x32

/// Vendor specific
#define		PT_VENDOR_FIX					0x80000000
#define		TPM_PT_VENDOR_FIX_SMLI2			PT_VENDOR_FIX + 1
#define		MAX_NUM_FW_PACKAGES_DEVICE		0x0002
#define		SMS_FWCONFIG_ACTIVE				0x36A5
#define		SMS_BTLDR_ACTIVE				0x5A3C
#define		INACTIVE_STALE_VERSION			0xFFFFEEEE

/********************************************************************************
 * Field upgrade type definitions
 ********************************************************************************/
/**
 */
typedef	uint32_t	DecryptKeyId_d;
typedef	uint32_t	VersionNumber_d;
typedef uint8_t		SubCmd_d;
typedef uint16_t	SecurityModuleStatus_d;

/********************************************************************************
 * Field upgrade structure definitions
 ********************************************************************************/

/**
 *	@brief		Message digest structure
 */
typedef struct sMessageDigest_d
{
	uint16_t	wSize;
	uint8_t		rgbMessageDigest[32];
} sMessageDigest_d;

/**
 *	@brief		Firmware package structure
 */
typedef struct sFirmwarePackage_d
{
	uint32_t				FwPackageIdentifier;
	VersionNumber_d			Version;
	VersionNumber_d			StaleVersion;
} sFirmwarePackage_d;

/**
 *	@brief		Firmware packages structure
 */
typedef struct sFirmwarePackages_d
{
	uint16_t			wEntries;
	sFirmwarePackage_d	FirmwarePackage[MAX_NUM_FW_PACKAGES_DEVICE];
} sFirmwarePackages_d;

/**
 *	@brief		Versions structure
 */
typedef struct sVersions_d
{
	uint32_t				internal1;
	uint16_t				wEntries;
	VersionNumber_d			Version[8];
} sVersions_d;

/**
 *	@brief		Signed Attributes structure
 */
typedef struct sSignedAttributes_d
{
	uint16_t						internal1;
	uint8_t							internal2[6];
	uint16_t						internal3;
	sMessageDigest_d				sMessageDigest;
	sFirmwarePackage_d				sFirmwarePackage;
	uint8_t							internal6[34];
	uint8_t							internal7[34];
	uint8_t							internal8[16];
	DecryptKeyId_d					DecryptKeyId;
	uint8_t							internal10[20];
	sVersions_d						sVersions;
	uint8_t							internal12[66];
	uint8_t							internal13[42];
} sSignedAttributes_d;

/**
 *	@brief		Signer Info structure
 */
typedef struct sSignerInfo_d
{
	uint16_t				internal1;
	uint32_t				internal2;
	uint16_t				internal3;
	uint16_t				internal4;
	uint16_t				internal5;
	sSignedAttributes_d		sSignedAttributes;
	uint16_t				internal7;
	uint8_t					internal8[256];
} sSignerInfo_d;

/**
 *	@brief		Signed Firmware Data Info structure
 */
typedef struct sSignedData_d
{
	uint16_t				internal1;
	uint16_t				internal2;
	uint16_t				wSignerInfoSize;
	sSignerInfo_d			sSignerInfo;
} sSignedData_d;

/**
 *	@brief		Security Module Logic structure
 */
typedef struct sSecurityModuleLogic_d
{
	uint16_t					internal1;
	uint32_t					internal2;
	uint8_t						internal3[34];
	sFirmwarePackage_d			sBootloaderFirmwarePackage;
	sFirmwarePackages_d			sFirmwareConfiguration;
} sSecurityModuleLogic_d;

/**
 *	@brief		List of Key Identifier IDs structure
 */
typedef struct sKeyList_d
{
	uint16_t			wEntries;
	uint32_t			internal2[4];
	DecryptKeyId_d		DecryptKeyId[4];
} sKeyList_d;

/**
 *	@brief		Security Module Logic Info structure
 */
typedef struct sSecurityModuleLogicInfo_d
{
	uint16_t				internal1;
	uint16_t				wMaxDataSize;
	sSecurityModuleLogic_d	sSecurityModuleLogic;
	SecurityModuleStatus_d	SecurityModuleStatus;
	sFirmwarePackage_d		sProcessFirmwarePackage;
	uint16_t				internal6;
	uint8_t					internal7[6];
	uint16_t				wFieldUpgradeCounter;
} sSecurityModuleLogicInfo_d;

/**
 *	@brief		Security Module Logic Info2 structure
 */
typedef struct sSecurityModuleLogicInfo2_d
{
	uint16_t				internal1;
	uint16_t				wMaxDataSize;
	sSecurityModuleLogic_d	sSecurityModuleLogic;
	SecurityModuleStatus_d	SecurityModuleStatus;
	sFirmwarePackage_d		sProcessFirmwarePackage;
	uint16_t				internal6;
	uint8_t					internal7[6];
	uint16_t				wFieldUpgradeCounter;
	sKeyList_d				sKeyList;
} sSecurityModuleLogicInfo2_d;

/**
 *	@brief		TPML_MAX_BUFFER structure
 *	@details	TPML_MAX_BUFFER structure definition
 */
typedef struct _TSS_TPML_MAX_BUFFER
{
	/// Number of properties
	/// A value of zero is allowed.
	uint32_t count;
	/// An array of bytes
	TSS_TPM2B_MAX_BUFFER buffer[1];
} TSS_TPML_MAX_BUFFER;

/**
 *	@brief		TPMU_VENDOR_CAPABILITY union
 *	@details	TPMU_VENDOR_CAPABILITY union definition
 */
typedef union _TSS_TPMU_VENDOR_CAPABILITY
{
	TSS_TPML_MAX_BUFFER vendorData;
} TSS_TPMU_VENDOR_CAPABILITY;

/**
 *	@brief		TPMU_VENDOR_CAPABILITY union
 *	@details	TPMU_VENDOR_CAPABILITY union definition\n
 *				Table 105 - Definition of TPMS_CAPABILITY_DATA Structure [OUT]
 */
typedef struct _TSS_TPMS_VENDOR_CAPABILITY_DATA {
	/// The capability
	TSS_TPM_CAP capability;
	/// The capability data
	TSS_TPMU_VENDOR_CAPABILITY data;
} TSS_TPMS_VENDOR_CAPABILITY_DATA;
