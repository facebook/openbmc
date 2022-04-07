/**
 *	@brief		Declares application configuration settings
 *	@details	Module to verify and store configuration settings. Implements the IConfigSettings interface.
 *	@file		TPMFactoryUpd\ConfigSettings.h
 *	@copyright	Copyright 2014 - 2018 Infineon Technologies AG ( www.infineon.com )
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

#ifdef __cplusplus
extern "C" {
#endif

/// Define for configuration section LOGGING
#define CONFIG_SECTION_LOGGING			L"LOGGING"
/// Define for LOGGING section setting LEVEL
#define CONFIG_KEY_LOGGING_LEVEL		L"LEVEL"
/// Define for LOGGING section setting PATH
#define CONFIG_KEY_LOGGING_PATH			L"PATH"
/// Define for LOGGING section setting MAXSIZE
#define CONFIG_KEY_LOGGING_MAXSIZE		L"MAXSIZE"

/// Define for configuration section ACCESS_MODE
#define CONFIG_SECTION_ACCESS_MODE		L"ACCESS_MODE"
/// Define for ACCESS_MODE section setting LOCALITY
#define CONFIG_KEY_ACCESS_MODE_LOCALITY	L"LOCALITY"
/// Define for configuration section CONSOLE
#define CONFIG_SECTION_CONSOLE			L"CONSOLE"
/// Define for CONSOLE section setting MODE
#define CONFIG_KEY_CONSOLE_MODE			L"MODE"

/// Define for configuration section TPM_DEVICE_ACCESS
#define CONFIG_SECTION_TPM_DEVICE_ACCESS	L"TPM_DEVICE_ACCESS"
/// Define for TPM_DEVICE_ACCESS section setting MODE
#define CONFIG_KEY_TPM_DEVICE_ACCESS_MODE	L"MODE"

/// Define for update-file configuration section UpdateType
#define CONFIG_SECTION_UPDATE_TYPE		L"UpdateType"
/// Define for UpdateType section setting tpm12
#define CONFIG_UPDATE_TYPE_TPM12		L"tpm12"
/// Define for UpdateType section setting tpm20
#define CONFIG_UPDATE_TYPE_TPM20		L"tpm20"
/// Define for update-file configuration section TargetFirmware
#define CONFIG_SECTION_TARGET_FIRMWARE	L"TargetFirmware"
/// Define for target firmware section setting generic version
#define CONFIG_TARGET_FIRMWARE_VERSION_GENERIC	L"version"
/// Define for target firmware section setting version_SLB965x (LPC TPM)
#define CONFIG_TARGET_FIRMWARE_VERSION_SLB965x	L"version_SLB965x"
/// Define for target firmware section setting version_SLB966x (LPC TPM)
#define CONFIG_TARGET_FIRMWARE_VERSION_SLB966x	L"version_SLB966x"
/// Define for target firmware section setting version_SLB9670 (SPI TPM)
#define CONFIG_TARGET_FIRMWARE_VERSION_SLB9670	L"version_SLB9670"
/// Define for target firmware section setting version_SLI9670 (SPI TPM)
#define CONFIG_TARGET_FIRMWARE_VERSION_SLI9670	L"version_SLI9670"
/// Define for target firmware section setting version_SLB9615 (I2C TPM)
#define CONFIG_TARGET_FIRMWARE_VERSION_SLB9615	L"version_SLB9615"
/// Define for target firmware section setting version_SLB9645 (I2C TPM)
#define CONFIG_TARGET_FIRMWARE_VERSION_SLB9645	L"version_SLB9645"
/// Define for update-file configuration section FirmwareFolder
#define CONFIG_SECTION_FIRMWARE_FOLDER	L"FirmwareFolder"
/// Define for firmware folder section setting path
#define CONFIG_FIRMWARE_FOLDER_PATH		L"path"

/// Default max log file size in kilobyte
#define LOGGING_FILE_MAX_SIZE			1024

/// Definition of Locality 0 for accessing TPM
#define LOCALITY_0						0

#ifdef __cplusplus
}
#endif