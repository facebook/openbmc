/**
 *  @brief      Implements application configuration settings
 *  @details    Module to verify and store configuration settings. Implements the IConfigSettings interface.
 *  @file       TPMFactoryUpd\ConfigSettings.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ConfigSettings.h"
#include "IConfigSettings.h"
#include "Platform.h"

const wchar_t CwszErrorMsgFormatSetDefaultFailed[] = L"Set default for property '%ls' failed.";
const wchar_t CwszErrorMsgFormatGetValueByKey[] = L"PropertyStorage_GetValueByKey failed while getting the property '%ls'.";
const wchar_t CwszErrorMsgFormatChangeValueByKey[] = L"PropertyStorage_ChangeValueByKey failed while updating the property '%ls'.";

/**
 *  @brief      Initialize configuration settings parsing
 *  @details
 *
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     RC_E_FAIL   An unexpected error occurred.
 */
_Check_return_
unsigned int
ConfigSettings_InitializeParsing()
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Set default LogLevel: LOGGING_LEVEL_1
        if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, LOGGING_LEVEL_1))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_LOGGING_LEVEL);
            break;
        }

        {
            // Calculate default log file path
            wchar_t wszDefaultLogPath[PROPERTY_STORAGE_MAX_VALUE];
            unsigned int unCapacity = RG_LEN(wszDefaultLogPath);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszDefaultLogPath, RG_LEN(wszDefaultLogPath)));
            unReturnValue = Platform_StringCopy(wszDefaultLogPath, &unCapacity, GwszToolName);
            if (RC_SUCCESS != unReturnValue)
                break;
            unCapacity = RG_LEN(wszDefaultLogPath);
            unReturnValue = Platform_StringConcatenate(wszDefaultLogPath, &unCapacity, L".log");
            if (RC_SUCCESS != unReturnValue)
                break;

            // Set default log file path
            if (!PropertyStorage_SetValueByKey(PROPERTY_LOGGING_PATH, wszDefaultLogPath))
            {
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_LOGGING_PATH);
                break;
            }
        }

        // Set default LogFileMaxSize
        if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_LOGGING_MAXSIZE, LOGGING_FILE_MAX_SIZE))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_LOGGING_MAXSIZE);
            break;
        }

        // Set default console mode: CONSOLE_BUFFER_NONE
        if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_CONSOLE_MODE, CONSOLE_BUFFER_NONE))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_CONSOLE_MODE);
            break;
        }

        // Set default access mode: LOCALITY_0
        if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_LOCALITY, LOCALITY_0))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_LOCALITY);
            break;
        }

        // Set default device access mode: MODE = TPM_DEVICE_ACCESS_DRIVER
        if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, TPM_DEVICE_ACCESS_DRIVER))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_TPM_DEVICE_ACCESS_MODE);
            break;
        }

        // Set default device access path:
        if (!PropertyStorage_SetValueByKey(PROPERTY_TPM_DEVICE_ACCESS_PATH, TPM_DEVICE_ACCESS_PATH))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_TPM_DEVICE_ACCESS_PATH);
            break;
        }

        // Set locality access protocol: Don't release between commands. Release only when TPMFactoryUpd exits.
        if (!PropertyStorage_SetBooleanValueByKey(PROPERTY_KEEP_LOCALITY_ACTIVE, TRUE))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_KEEP_LOCALITY_ACTIVE);
            break;
        }

        // Set default abandon firmware update mode (no action)
        if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_ABANDON_UPDATE_MODE, ABANDON_UPDATE_NO_ACTION))
        {
            ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatSetDefaultFailed, PROPERTY_ABANDON_UPDATE_MODE);
            break;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Parses the configuration setting
 *  @details    Parses the configuration setting identified by the common configuration module code
 *
 *  @param      PwszSection         Pointer to a wide character array containing the current section.
 *  @param      PunSectionSize      Size of the section buffer in elements including the zero termination.
 *  @param      PwszKey             Pointer to a wide character array containing the current key.
 *  @param      PunKeySize          Size of the key buffer in elements including the zero termination.
 *  @param      PwszValue           Pointer to a wide character array containing the current value.
 *  @param      PunValueSize        Size of the value buffer in elements including the zero termination.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. It is NULL or empty.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 */
_Check_return_
unsigned int
ConfigSettings_Parse(
    _In_z_count_(PunSectionSize)    const wchar_t*  PwszSection,
    _In_                            unsigned int    PunSectionSize,
    _In_z_count_(PunKeySize)        const wchar_t*  PwszKey,
    _In_                            unsigned int    PunKeySize,
    _In_z_count_(PunValueSize)      const wchar_t*  PwszValue,
    _In_                            unsigned int    PunValueSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    UNREFERENCED_PARAMETER(PunValueSize);

    do
    {
        // Check parameters
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszSection) ||
                0 == PunSectionSize ||
                PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszKey) ||
                0 == PunKeySize ||
                PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszValue) ||
                0 == PunValueSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more input parameters are NULL or empty.");
            break;
        }

        // Check section "LOGGING"
        if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_LOGGING, PunSectionSize, FALSE))
        {
            // Check logging level
            if (0 == Platform_StringCompare(PwszKey, CONFIG_KEY_LOGGING_LEVEL, PunKeySize, FALSE))
            {
                // Store setting value
                if (FALSE == PropertyStorage_ChangeValueByKey(PROPERTY_LOGGING_LEVEL, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatChangeValueByKey, PROPERTY_LOGGING_LEVEL);
                    break;
                }

                unReturnValue = RC_SUCCESS;
                break;
            }

            // Check logging file path
            if (0 == Platform_StringCompare(PwszKey, CONFIG_KEY_LOGGING_PATH, PunKeySize, FALSE))
            {
                // Store setting value
                if (FALSE == PropertyStorage_ChangeValueByKey(PROPERTY_LOGGING_PATH, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatChangeValueByKey, PROPERTY_LOGGING_PATH);
                    break;
                }

                unReturnValue = RC_SUCCESS;
                break;
            }

            // Check logging file max size
            if (0 == Platform_StringCompare(PwszKey, CONFIG_KEY_LOGGING_MAXSIZE, PunKeySize, FALSE))
            {
                // Store setting value
                if (FALSE == PropertyStorage_ChangeValueByKey(PROPERTY_LOGGING_MAXSIZE, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatChangeValueByKey, PROPERTY_LOGGING_MAXSIZE);
                    break;
                }

                unReturnValue = RC_SUCCESS;
                break;
            }

            // Unknown setting in current section
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Check section ACCESS_MODE options
        if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_ACCESS_MODE, PunSectionSize, FALSE))
        {
            // Check locality option
            if (0 == Platform_StringCompare(PwszKey, CONFIG_KEY_ACCESS_MODE_LOCALITY, PunKeySize, FALSE))
            {
                // Store setting value
                if (FALSE == PropertyStorage_ChangeValueByKey(PROPERTY_LOCALITY, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatChangeValueByKey, PROPERTY_LOCALITY);
                    break;
                }

                unReturnValue = RC_SUCCESS;
                break;
            }

            // Unknown setting in current section
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Check section CONSOLE options
        if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_CONSOLE, PunSectionSize, FALSE))
        {
            // Check mode option
            if (0 == Platform_StringCompare(PwszKey, CONFIG_KEY_CONSOLE_MODE, PunKeySize, FALSE))
            {
                // Store setting value
                if (FALSE == PropertyStorage_ChangeValueByKey(PROPERTY_CONSOLE_MODE, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatChangeValueByKey, PROPERTY_CONSOLE_MODE);
                    break;
                }

                unReturnValue = RC_SUCCESS;
                break;
            }

            // Unknown setting in current section
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Check section TPM_DEVICE_ACCESS options
        if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_TPM_DEVICE_ACCESS, PunSectionSize, FALSE))
        {
            // Check MODE option
            if (0 == Platform_StringCompare(PwszKey, CONFIG_KEY_TPM_DEVICE_ACCESS_MODE, PunKeySize, FALSE))
            {
                // Store setting value
                if(!PropertyStorage_ChangeValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatChangeValueByKey, PROPERTY_TPM_DEVICE_ACCESS_MODE);
                    break;
                }

                // Check TPM device access mode
                {
                    unsigned int unAccessMode = 0;
                    if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unAccessMode))
                    {
                        unReturnValue = RC_E_INVALID_SETTING;
                        ERROR_STORE_FMT(unReturnValue, L"An invalid value (%ls) was passed in the %ls/%ls configuration of the config file.", PwszValue, CONFIG_SECTION_TPM_DEVICE_ACCESS, CONFIG_KEY_TPM_DEVICE_ACCESS_MODE);
                        break;
                    }
                    switch (unAccessMode)
                    {
#if !(defined (__aarch64__) || defined (__arm__))
                        case TPM_DEVICE_ACCESS_MEMORY_BASED:
                        {
                            unReturnValue = RC_SUCCESS;
                            break;
                        }
#endif
#ifdef LINUX
                        case TPM_DEVICE_ACCESS_DRIVER:
                        {   unReturnValue = RC_SUCCESS;
                            break;
                        }
#endif
                        default:
                        {
                            unReturnValue = RC_E_INVALID_SETTING;
                            ERROR_STORE_FMT(unReturnValue, L"An invalid value (%d) was passed in the %ls/%ls configuration of the config file.", unAccessMode, CONFIG_SECTION_TPM_DEVICE_ACCESS, CONFIG_KEY_TPM_DEVICE_ACCESS_MODE);
                            break;
                        }
                    }

                    if (RC_SUCCESS != unReturnValue)
                        break;
                }

                unReturnValue = RC_SUCCESS;
                break;
            }
            // Unknown setting in current section
            unReturnValue = RC_SUCCESS;
            break;
        }
        // Unknown section
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Finalize configuration settings parsing
 *  @details
 *
 *  @param      PunReturnValue  Current return code which can be overwritten here.
 *  @retval     PunReturnValue  In case PunReturnValue is not equal to RC_SUCCESS.
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred.
 */
_Check_return_
unsigned int
ConfigSettings_FinalizeParsing(
    _In_ const unsigned int PunReturnValue)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        if (RC_SUCCESS != PunReturnValue)
        {
            LOGGING_WRITE_LEVEL1_FMT(L"Error occurred during configuration initialization: 0x%.8X", PunReturnValue);
            unReturnValue = PunReturnValue;
            break;
        }

        // Builds the final log file path
        IGNORE_RETURN_VALUE(Logging_BuildLoggingFilePath());

        // Log the current configuration
        {
            wchar_t wszValue[MAX_STRING_1024];
            unsigned int unValueSize = RG_LEN(wszValue);
            LOGGING_WRITE_LEVEL4(L"Used configuration:");
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszValue, RG_LEN(wszValue)));

            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_LOGGING_LEVEL, wszValue, &unValueSize))
            {
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatGetValueByKey, PROPERTY_LOGGING_LEVEL);
                break;
            }
            LOGGING_WRITE_LEVEL4_FMT(L"%ls: %ls", PROPERTY_LOGGING_LEVEL, wszValue);

            unValueSize = RG_LEN(wszValue);
            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_LOGGING_PATH, wszValue, &unValueSize))
            {
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatGetValueByKey, PROPERTY_LOGGING_PATH);
                break;
            }
            LOGGING_WRITE_LEVEL4_FMT(L"%ls: %ls", PROPERTY_LOGGING_PATH, wszValue);

            unValueSize = RG_LEN(wszValue);
            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_LOGGING_MAXSIZE, wszValue, &unValueSize))
            {
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatGetValueByKey, PROPERTY_LOGGING_MAXSIZE);
                break;
            }
            LOGGING_WRITE_LEVEL4_FMT(L"%ls: %ls", PROPERTY_LOGGING_MAXSIZE, wszValue);

            unValueSize = RG_LEN(wszValue);
            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_LOCALITY, wszValue, &unValueSize))
            {
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatGetValueByKey, PROPERTY_LOCALITY);
                break;
            }
            LOGGING_WRITE_LEVEL4_FMT(L"%ls: %ls", PROPERTY_LOCALITY, wszValue);

            unValueSize = RG_LEN(wszValue);
            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_CONSOLE_MODE, wszValue, &unValueSize))
            {
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatGetValueByKey, PROPERTY_CONSOLE_MODE);
                break;
            }
            LOGGING_WRITE_LEVEL4_FMT(L"%ls: %ls", PROPERTY_CONSOLE_MODE, wszValue);

            unValueSize = RG_LEN(wszValue);
            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, wszValue, &unValueSize))
            {
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatGetValueByKey, PROPERTY_TPM_DEVICE_ACCESS_MODE);
                break;
            }
            LOGGING_WRITE_LEVEL4_FMT(L"%ls: %ls", PROPERTY_TPM_DEVICE_ACCESS_MODE, wszValue);

            unReturnValue = RC_SUCCESS;
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}
