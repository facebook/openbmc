/**
 *	@brief		Implements the TPM access connection
 *	@details	Implements the connection to the underlying TPM access module (TpmIO interface).
 *	@file		DeviceManagement.c
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

#include "DeviceManagement.h"
#include "TpmIO.h"
#include "Logging.h"
#include "Platform.h"

/// Function pointer to method for connecting to the TPM
PFN_TPMIO_Connect		s_fpTpmIoConnect = NULL;

/// Function pointer to method for disconnecting from the TPM
PFN_TPMIO_Disconnect	s_fpTpmIoDisconnect = NULL;

/// Function pointer to method for transmitting data to the TPM
PFN_TPMIO_Transmit		s_fpTpmIoTransmit = NULL;

/// Function pointer to read a byte from a register of the TPM
PFN_TPMIO_ReadRegister	s_fpTpmIoReadRegister = NULL;

/// Function pointer to write a byte to a register of the TPM
PFN_TPMIO_WriteRegister	s_fpTpmIoWriteRegister = NULL;

/// Flag indicating TPM connection established or not
BOOL					s_fTpmConnected = FALSE;

/// Flag indicating device management initialization performed or not
BOOL					s_fInitialized = FALSE;

/// Caches the last TPM command
BYTE					g_rgbLastRequest[4096] = {0};

/// Caches the size of the last TPM command
unsigned int			g_unSizeLastRequest = 0;

/// Caches the last TPM response
BYTE					g_rgbLastResponse[4096] = {0};

/// Caches the size of the last TPM response
unsigned int			g_unSizeLastResponse = 0;

/// Maximum wait time in TIS protocol for commands of category SMALL_DURATION: 10 seconds
#define SMALL_DURATION 10000000
/// Maximum wait time in TIS protocol for commands of category MEDIUM_DURATION: 20 seconds
#define MEDIUM_DURATION 20000000
/// Maximum wait time in TIS protocol for commands of category LONG_DURATION: 120 seconds
#define LONG_DURATION 120000000

/// List of available TPM1.2 command names and their properties: command code, maximum command duration
IfxTpmCommand s_sTpm1Commands[] = {
	{L"None", 0, LONG_DURATION},
	{L"TPM_OIAP", 0x0000000A, SMALL_DURATION},
	{L"TPM_OSAP", 0x0000000B, SMALL_DURATION},
	{L"TPM_TakeOwnership", 0x0000000D, LONG_DURATION},
	{L"TPM_ChangeAuthOwner", 0x00000010, MEDIUM_DURATION},
	{L"TPM_SetCapability", 0x0000003F, SMALL_DURATION},
	{L"TPM_GetTestResult", 0x00000054, SMALL_DURATION},
	{L"TPM_OwnerClear", 0x0000005B, SMALL_DURATION},
	{L"TPM_GetCapability", 0x00000065, SMALL_DURATION},
	{L"TPM_ReadPubEK", 0x0000007C, MEDIUM_DURATION},
	{L"TPM_OwnerReadInternalPub", 0x00000081, SMALL_DURATION},
	{L"TPM_Startup", 0x00000099, SMALL_DURATION},
	{L"TPM_FieldUpgrade", 0x000000AA, MEDIUM_DURATION},
	{L"TPM_FlushSpecific", 0x000000BA, SMALL_DURATION},
	{L"TSC_PhysicalPresence", 0x4000000A, SMALL_DURATION}
};

/// List of available TPM2.0 command names and their properties: command code, maximum command duration
IfxTpmCommand s_sTpm2Commands[] = {
	{L"None", 0, LONG_DURATION},										{L"TPM2_NV_UndefineSpaceSpecial", 0x0000011F, LONG_DURATION},	{L"TPM2_EvictControl", 0x00000120, LONG_DURATION},
	{L"TPM2_HierarchyControl", 0x00000121, LONG_DURATION},				{L"TPM2_NV_UndefineSpace", 0x00000122, LONG_DURATION},			{L"TPM2_ChangeEPS", 0x00000124, LONG_DURATION},
	{L"TPM2_ChangePPS", 0x00000125, LONG_DURATION},						{L"TPM2_Clear", 0x00000126, LONG_DURATION},						{L"TPM2_ClearControl", 0x00000127, LONG_DURATION},
	{L"TPM2_ClockSet", 0x00000128, LONG_DURATION},						{L"TPM2_HierarchyChangeAuth", 0x00000129, LONG_DURATION},		{L"TPM2_NV_DefineSpace", 0x0000012A, LONG_DURATION},
	{L"TPM2_PCR_Allocate", 0x0000012B, LONG_DURATION},					{L"TPM2_PCR_SetAuthPolicy", 0x0000012C, LONG_DURATION},			{L"TPM2_PP_Commands", 0x0000012D, LONG_DURATION},
	{L"TPM2_SetPrimaryPolicy", 0x0000012E, LONG_DURATION},				{L"TPM2_FieldUpgradeStart", 0x0000012F, LONG_DURATION},			{L"TPM2_ClockRateAdjust", 0x00000130, LONG_DURATION},
	{L"TPM2_CreatePrimary", 0x00000131, LONG_DURATION},					{L"TPM2_NV_GlobalWriteLock", 0x00000132, LONG_DURATION},		{L"TPM2_GetCommandAuditDigest", 0x00000133, LONG_DURATION},
	{L"TPM2_NV_Increment", 0x00000134, LONG_DURATION},					{L"TPM2_NV_SetBits", 0x00000135, LONG_DURATION},				{L"TPM2_NV_Extend", 0x00000136, LONG_DURATION},
	{L"TPM2_NV_Write", 0x00000137, LONG_DURATION},						{L"TPM2_NV_WriteLock", 0x00000138, LONG_DURATION},				{L"TPM2_DictionaryAttackLockReset", 0x00000139, LONG_DURATION},
	{L"TPM2_DictionaryAttackParameters", 0x0000013A, LONG_DURATION},	{L"TPM2_NV_ChangeAuth", 0x0000013B, LONG_DURATION},				{L"TPM2_PCR_Event", 0x0000013C, LONG_DURATION},
	{L"TPM2_PCR_Reset", 0x0000013D, LONG_DURATION},						{L"TPM2_SequenceComplete", 0x0000013E, LONG_DURATION},			{L"TPM2_SetAlgorithmSet", 0x0000013F, LONG_DURATION},
	{L"TPM2_SetCommandCodeAuditStatus", 0x00000140, LONG_DURATION},		{L"TPM2_FieldUpgradeData", 0x00000141, LONG_DURATION},			{L"TPM2_IncrementalSelfTest", 0x00000142, LONG_DURATION},
	{L"TPM2_SelfTest", 0x00000143, LONG_DURATION},						{L"TPM2_Startup", 0x00000144, LONG_DURATION},					{L"TPM2_Shutdown", 0x00000145, LONG_DURATION},
	{L"TPM2_StirRandom", 0x00000146, LONG_DURATION},					{L"TPM2_ActivateCredential", 0x00000147, LONG_DURATION},		{L"TPM2_Certify", 0x00000148, LONG_DURATION},
	{L"TPM2_PolicyNV", 0x00000149, LONG_DURATION},						{L"TPM2_CertifyCreation", 0x0000014A, LONG_DURATION},			{L"TPM2_Duplicate", 0x0000014B, LONG_DURATION},
	{L"TPM2_GetTime", 0x0000014C, LONG_DURATION},						{L"TPM2_GetSessionAuditDigest", 0x0000014D, LONG_DURATION},		{L"TPM2_NV_Read", 0x0000014E, LONG_DURATION},
	{L"TPM2_NV_ReadLock", 0x0000014F, LONG_DURATION},					{L"TPM2_ObjectChangeAuth", 0x00000150, LONG_DURATION},			{L"TPM2_PolicySecret", 0x00000151, LONG_DURATION},
	{L"TPM2_Rewrap", 0x00000152, LONG_DURATION},						{L"TPM2_Create", 0x00000153, LONG_DURATION},					{L"TPM2_ECDH_ZGen", 0x00000154, LONG_DURATION},
	{L"TPM2_HMAC", 0x00000155, LONG_DURATION},							{L"TPM2_Import", 0x00000156, LONG_DURATION},					{L"TPM2_Load", 0x00000157, LONG_DURATION},
	{L"TPM2_Quote", 0x00000158, LONG_DURATION},							{L"TPM2_RSA_Decrypt", 0x00000159, LONG_DURATION},				{L"TPM2_HMAC_Start", 0x0000015B, LONG_DURATION},
	{L"TPM2_SequenceUpdate", 0x0000015C, LONG_DURATION},				{L"TPM2_Sign", 0x0000015D, LONG_DURATION},						{L"TPM2_Unseal", 0x0000015E, LONG_DURATION},
	{L"TPM2_PolicySigned", 0x00000160, LONG_DURATION},					{L"TPM2_ContextLoad", 0x00000161, LONG_DURATION},				{L"TPM2_ContextSave", 0x00000162, LONG_DURATION},
	{L"TPM2_ECDH_KeyGen", 0x00000163, LONG_DURATION},					{L"TPM2_EncryptDecrypt", 0x00000164, LONG_DURATION},			{L"TPM2_FlushContext", 0x00000165, LONG_DURATION},
	{L"TPM2_LoadExternal", 0x00000167, LONG_DURATION},					{L"TPM2_MakeCredential", 0x00000168, LONG_DURATION},			{L"TPM2_NV_ReadPublic", 0x00000169, LONG_DURATION},
	{L"TPM2_PolicyAuthorize", 0x0000016A, LONG_DURATION},				{L"TPM2_PolicyAuthValue", 0x0000016B, LONG_DURATION},			{L"TPM2_PolicyCommandCode", 0x0000016C, LONG_DURATION},
	{L"TPM2_PolicyCounterTimer", 0x0000016D, LONG_DURATION},			{L"TPM2_PolicyCpHash", 0x0000016E, LONG_DURATION},				{L"TPM2_PolicyLocality", 0x0000016F, LONG_DURATION},
	{L"TPM2_PolicyNameHash", 0x00000170, LONG_DURATION},				{L"TPM2_PolicyOR", 0x00000171, LONG_DURATION},					{L"TPM2_PolicyTicket", 0x00000172, LONG_DURATION},
	{L"TPM2_ReadPublic", 0x00000173, LONG_DURATION},					{L"TPM2_RSA_Encrypt", 0x00000174, LONG_DURATION},				{L"TPM2_StartAuthSession", 0x00000176, LONG_DURATION},
	{L"TPM2_VerifySignature", 0x00000177, LONG_DURATION},				{L"TPM2_ECC_Parameters", 0x00000178, LONG_DURATION},			{L"TPM2_FirmwareRead", 0x00000179, LONG_DURATION},
	{L"TPM2_GetCapability", 0x0000017A, LONG_DURATION},					{L"TPM2_GetRandom", 0x0000017B, LONG_DURATION},					{L"TPM2_GetTestResult", 0x0000017C, LONG_DURATION},
	{L"TPM2_Hash", 0x0000017D, LONG_DURATION},							{L"TPM2_PCR_Read", 0x0000017E, LONG_DURATION},					{L"TPM2_PolicyPCR", 0x0000017F, LONG_DURATION},
	{L"TPM2_PolicyRestart", 0x00000180, LONG_DURATION},					{L"TPM2_ReadClock", 0x00000181, LONG_DURATION},					{L"TPM2_PCR_Extend", 0x00000182, LONG_DURATION},
	{L"TPM2_PCR_SetAuthValue", 0x00000183, LONG_DURATION},				{L"TPM2_NV_Certify", 0x00000184, LONG_DURATION},				{L"TPM2_EventSequenceComplete", 0x00000185, LONG_DURATION},
	{L"TPM2_HashSequenceStart", 0x00000186, LONG_DURATION},				{L"TPM2_PolicyPhysicalPresence", 0x00000187, LONG_DURATION},	{L"TPM2_PolicyDuplicationSelect", 0x00000188, LONG_DURATION},
	{L"TPM2_PolicyGetDigest", 0x00000189, LONG_DURATION},				{L"TPM2_TestParms", 0x0000018A, LONG_DURATION},					{L"TPM2_Commit", 0x0000018B, LONG_DURATION},
	{L"TPM2_PolicyPassword", 0x0000018C, LONG_DURATION},				{L"TPM2_ZGen_2Phase", 0x0000018D, LONG_DURATION},				{L"TPM2_EC_Ephemeral", 0x0000018E, LONG_DURATION},
	{L"TPM2_PolicyNvWritten", 0x0000018F, LONG_DURATION},				{L"TPM2_FieldUpgradeStartVendor", 0x2000012F, LONG_DURATION},	{L"TPM2_SetCapabilityVendor", 0x20000400, LONG_DURATION}
};

/**
 *	@brief		Device management initialization function
 *	@details	This function initializes the device IO.
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 */
_Check_return_
unsigned int
DeviceManagement_Initialize()
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check if already initialized
		if (FALSE == DeviceManagement_IsInitialized())
		{
			// Initialize the TPM IO Function pointers
			s_fpTpmIoConnect		= &TPMIO_Connect;
			s_fpTpmIoDisconnect		= &TPMIO_Disconnect;
			s_fpTpmIoTransmit		= &TPMIO_Transmit;
			s_fpTpmIoReadRegister	= &TPMIO_ReadRegister;
			s_fpTpmIoWriteRegister	= &TPMIO_WriteRegister;
			s_fInitialized = TRUE;
		}
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Device management uninitialization function
 *	@details	This function uninitializes the device IO.
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 */
_Check_return_
unsigned int
DeviceManagement_Uninitialize()
{
	unsigned int unReturnValue = RC_E_FAIL;
	do
	{
		// Check if initialized
		if (TRUE == DeviceManagement_IsInitialized())
		{
			// Uninitialize the TPM IO Function pointers
			s_fpTpmIoConnect		= NULL;
			s_fpTpmIoDisconnect		= NULL;
			s_fpTpmIoTransmit		= NULL;
			s_fpTpmIoReadRegister	= NULL;
			s_fpTpmIoWriteRegister	= NULL;
			s_fInitialized = FALSE;
		}
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Device management IsInitialized function
 *	@details	This function returns the flag signalized a module which was initialized or not
 *
 *	@retval		TRUE	If module is initialized
 *	@retval		FALSE	If module is not initialized
 */
_Check_return_
BOOL
DeviceManagement_IsInitialized()
{
	return s_fInitialized;
}

/**
 *	@brief		Device management IsConnected function
 *	@details	This function returns the flag signalized a module which was connected to the TPM or not
 *
 *	@retval		TRUE	If module is connected
 *	@retval		FALSE	If module is not connected
 */
_Check_return_
BOOL
DeviceManagement_IsConnected()
{
	return s_fTpmConnected;
}

/**
 *	@brief		Connect to TPM
 *	@details	This function opens a connection to the underlying TPM access module (TpmIO interface).
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_NOT_INITIALIZED	If this module is not initialized
 *	@retval		...						Error codes from TPMIO_Connect function
 */
_Check_return_
unsigned int
DeviceManagement_Connect()
{
	unsigned int unReturnValue = RC_E_FAIL;
	do
	{
		LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

		// Check if Module is initialized
		if (FALSE == DeviceManagement_IsInitialized())
		{
			// Set error code and fill error object
			unReturnValue = RC_E_NOT_INITIALIZED;
			ERROR_STORE(unReturnValue, L"Module not initialized (DeviceManagement)");
			break;
		}
		// Check if TPMIO is not connected
		if (FALSE == DeviceManagement_IsConnected())
		{
			// Try to connect to the TPMIO
			unReturnValue = s_fpTpmIoConnect();
			// Check if error occurred and fill error object
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE_FMT(unReturnValue, L"TPMConnect failed: 0x%.8X", unReturnValue);
				s_fTpmConnected = FALSE;
				break;
			}

			s_fTpmConnected = TRUE;
		}
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Disconnect from TPM
 *	@details	This function closes the connection to the underlying TPM access module (TpmIO interface).
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_NOT_INITIALIZED	If this module is not initialized
 *	@retval	...							Error codes from TPMIO_Disconnect function
 */
_Check_return_
unsigned int
DeviceManagement_Disconnect()
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check if Module is initialized
		if (FALSE == DeviceManagement_IsInitialized())
		{
			// Set error code and fill error object
			unReturnValue = RC_E_NOT_INITIALIZED;
			ERROR_STORE(unReturnValue, L"Module not initialized (DeviceManagement)");
			break;
		}
		// Check if TPMIO is connected
		if (TRUE == DeviceManagement_IsConnected())
		{
			// Try to disconnect from the TPMIO
			unReturnValue = s_fpTpmIoDisconnect();

			// Check if error occurred and fill error object
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE_FMT(unReturnValue, L"TPMDisconnect failed: 0x%.8X", unReturnValue);
				break;
			}

			s_fTpmConnected = FALSE;
		}
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Device transmit function
 *	@details	This function submits the TPM command to the underlying TPM access module (TpmIO interface).
 *
 *	@param		PrgbRequestBuffer		Pointer to a byte array containing the TPM command request bytes
 *	@param		PunRequestBufferSize	Size of command request in bytes
 *	@param		PrgbResponseBuffer		Pointer to a byte array receiving the TPM command response bytes
 *	@param		PpunResponseBufferSize	Input size of response buffer, output size of TPM command response in bytes
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. Invalid buffer or buffer size
 *	@retval		RC_E_NOT_INITIALIZED	The module could not be initialized
 *	@retval		RC_E_NOT_CONNECTED		The connection to the TPM failed
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval	...							Error codes from s_fpTpmIoTransmit function
 */
_Check_return_
unsigned int
DeviceManagement_Transmit(
	_In_bytecount_(PunRequestBufferSize)		const BYTE*		PrgbRequestBuffer,
	_In_										unsigned int	PunRequestBufferSize,
	_Out_bytecap_(*PpunResponseBufferSize)		BYTE*			PrgbResponseBuffer,
	_Inout_										unsigned int*	PpunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		unsigned int unCommandCode = 0;
		unsigned int unTisMaxDuration = LONG_DURATION;

		// Check parameters
		if (NULL == PrgbRequestBuffer || NULL == PrgbResponseBuffer)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpbRequestBuffer or PpunResponseBufferSize is NULL)");
			break;
		}
		if (0 == PunRequestBufferSize || 0 == PpunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter PunRequestBufferSize or PpunResponseBufferSize has invalid value 0");
			break;
		}

		// Check if module is initialized
		if (FALSE == DeviceManagement_IsInitialized())
		{
			// Set error code and fill error object
			unReturnValue = RC_E_NOT_INITIALIZED;
			ERROR_STORE(unReturnValue, L"Module not initialized (DeviceManagement)");
			break;
		}

		// Check if TPMIO is connected
		if (FALSE == DeviceManagement_IsConnected())
		{
			unReturnValue = RC_E_NOT_CONNECTED;
			ERROR_STORE(unReturnValue, L"TPM not connected");
			break;
		}

		// Check if the request buffer holds at least enough bytes for the command length and code
		if (PunRequestBufferSize >= 10)
		{
			unsigned int unShiftedCommandCode = 0;
			// Get TPM command code
			unReturnValue = Platform_MemoryCopy(&unCommandCode, sizeof(unCommandCode), (const void*) &PrgbRequestBuffer[6], sizeof(unsigned int));
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
				break;
			}
			// Switch command code endianness
			unShiftedCommandCode = Platform_SwapBytes32(unCommandCode);
			// Output the corresponding command name
			DeviceManagement_TpmCommandName(unShiftedCommandCode, &unTisMaxDuration);
		}
		else
		{
			LOGGING_WRITE_LEVEL3(L"Sending unknown or invalid TPM Command");
		}

		LOGGING_WRITE_LEVEL3_FMT(L"DeviceManagement_Transmit: Sending:  TxLen = %4d", PunRequestBufferSize);
		LOGGING_WRITEHEX_LEVEL3(PrgbRequestBuffer, PunRequestBufferSize);

		// Cache the request for troubleshooting and clear the last TPM response cache
		unReturnValue = Platform_MemoryCopy(g_rgbLastRequest, sizeof(g_rgbLastRequest), PrgbRequestBuffer, PunRequestBufferSize);
		if (RC_SUCCESS != unReturnValue)
			break;
		g_unSizeLastRequest = PunRequestBufferSize;
		g_unSizeLastResponse = 0;

		unReturnValue = s_fpTpmIoTransmit(
							PrgbRequestBuffer,
							PunRequestBufferSize,
							PrgbResponseBuffer,
							PpunResponseBufferSize,
							unTisMaxDuration);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Error during TpmIOTransmit");

			// Log the last TPM command/response for troubleshooting
			LOGGING_WRITE_LEVEL1(L"Last TPM command:");
			LOGGING_WRITEHEX_LEVEL1(g_rgbLastRequest, g_unSizeLastRequest);
			LOGGING_WRITE_LEVEL1(L"Last TPM response:");
			LOGGING_WRITEHEX_LEVEL1(g_rgbLastResponse, g_unSizeLastResponse);

			break;
		}

		LOGGING_WRITE_LEVEL3_FMT(L"DeviceManagement_Transmit: Received:  RxLen = %4d", *PpunResponseBufferSize);
		LOGGING_WRITEHEX_LEVEL3(PrgbResponseBuffer, *PpunResponseBufferSize);

		// Cache the response for troubleshooting
		unReturnValue = Platform_MemoryCopy(g_rgbLastResponse, sizeof(g_rgbLastResponse), PrgbResponseBuffer, *PpunResponseBufferSize);
		if (RC_SUCCESS != unReturnValue)
			break;
		g_unSizeLastResponse = *PpunResponseBufferSize;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Function to output TPM command name and return the duration.
 *	@details	This function determines the TPM command name from the command ordinal and puts it to the log file.
 *
 *	@param		PunCommandCode			TPM command ordinal
 *	@param		PpunMaxDuration			Maximum command duration in microseconds (relevant for memory based access / TIS protocol only)
 */
void
DeviceManagement_TpmCommandName(
	_In_	unsigned int	PunCommandCode,
	_Out_	unsigned int*	PpunMaxDuration)
{
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		unsigned int unIndex = 0, unMaxCount = 0;
		IfxTpmCommand* prgTpmCommands = NULL;

		// Initialize output parameters
		*PpunMaxDuration = LONG_DURATION;

		// Determine if it is a TPM1.2 or TPM2.0 command code
		if (PunCommandCode & 0x00000100)
		{
			// TPM2.0 command code
			prgTpmCommands = s_sTpm2Commands;
			unMaxCount = sizeof(s_sTpm2Commands) / sizeof(IfxTpmCommand);
		}
		else
		{
			// TPM1.2 command code
			prgTpmCommands = s_sTpm1Commands;
			unMaxCount = sizeof(s_sTpm1Commands) / sizeof(IfxTpmCommand);
		}

		// Check for a known command code and eventually print out the command name
		do
		{
			if (prgTpmCommands[unIndex].unCommandCode == PunCommandCode)
			{
				*PpunMaxDuration = prgTpmCommands[unIndex].unMaxDuration;
				LOGGING_WRITE_LEVEL3_FMT(L"Sending TPM Command: %ls", prgTpmCommands[unIndex].pwszCommandName);
				break;
			}
			unIndex++;
		}
		while(unIndex < unMaxCount);

		// Print a warning message in case command code was not found
		if (unIndex == unMaxCount)
		{
			LOGGING_WRITE_LEVEL3(L"Sending unknown TPM Command");
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);

	return;
}

/**
 *	@brief		Register read function
 *	@details	This function reads a byte from a register address.
 *
 *	@param		PunRegisterAddress		Register address to read from
 *	@param		PpbRegisterValue		Pointer to a byte to store the register value
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. Invalid buffer or buffer size
 *	@retval		RC_E_NOT_INITIALIZED	The module could not be initialized
 *	@retval		RC_E_NOT_CONNECTED		The connection to the TPM failed
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval	...							Error codes from s_fpTpmIoReadRegister function
 */
_Check_return_
unsigned int
DeviceManagement_ReadRegister(
	_In_	unsigned int	PunRegisterAddress,
	_Out_	BYTE*			PpbRegisterValue)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check parameters
		if (NULL == PpbRegisterValue)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly");
			break;
		}

		// Set out parameter to default value
		*PpbRegisterValue = 0;

		// Check if Module is initialized
		if (FALSE == DeviceManagement_IsInitialized())
		{
			// Set error code and fill error object
			unReturnValue = RC_E_NOT_INITIALIZED;
			ERROR_STORE(unReturnValue, L"Module not initialized (DeviceManagement)");
			break;
		}

		// Check if TPMIO is connected
		if (FALSE == DeviceManagement_IsConnected())
		{
			unReturnValue = RC_E_NOT_CONNECTED;
			ERROR_STORE(unReturnValue, L"TPMnot connected");
			break;
		}

		unReturnValue = s_fpTpmIoReadRegister(PunRegisterAddress, PpbRegisterValue);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE_FMT(unReturnValue, L"Register read from address 0x%.8X failed.", PunRegisterAddress);
			break;
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Register write function
 *	@details	This function writes a byte to a register address.
 *				Validity of the address must be checked by the calling function.
 *
 *	@param		PunRegisterAddress		Register address to write to
 *	@param		PbRegisterValue			Byte value to write to the register
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_NOT_INITIALIZED	The module could not be initialized
 *	@retval		RC_E_NOT_CONNECTED		The connection to the TPM failed
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval	...							Error codes from s_fpTpmIoReadRegister function
 */
_Check_return_
unsigned int
DeviceManagement_WriteRegister(
	_In_	unsigned int	PunRegisterAddress,
	_In_	BYTE			PbRegisterValue)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check if Module is initialized
		if (FALSE == DeviceManagement_IsInitialized())
		{
			// Set error code and fill error object
			unReturnValue = RC_E_NOT_INITIALIZED;
			ERROR_STORE(unReturnValue, L"Module not initialized (DeviceManagement)");
			break;
		}

		// Check if TPMIO is connected
		if (FALSE == DeviceManagement_IsConnected())
		{
			unReturnValue = RC_E_NOT_CONNECTED;
			ERROR_STORE(unReturnValue, L"TPM not connected");
			break;
		}

		unReturnValue = s_fpTpmIoWriteRegister(PunRegisterAddress, PbRegisterValue);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE_FMT(unReturnValue, L"Write value 0x%.2X to register address 0x%.8X failed.", PbRegisterValue, PunRegisterAddress);
			break;
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}
