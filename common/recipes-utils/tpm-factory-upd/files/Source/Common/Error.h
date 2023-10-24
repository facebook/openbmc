/**
 *  @brief      Declares the error handling methods
 *  @details    This module stores information about the error occurred and maps
 *              errors to outgoing error codes and messages for logging and displaying.
 *  @file       Error.h
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief      Error data structure
 *  @details    Structure holding all parameters needed to describe an error occurrence
 */
typedef struct tdIfxErrorData
{
    /// The internal error code
    unsigned int    unInternalErrorCode;
    /// The internal error message
    wchar_t         wszInternalErrorMessage[MAX_MESSAGE_SIZE];
    /// The module where the error occurred
    wchar_t         wszOccurredInModule[MAX_NAME];
    /// The function in the module where the error occurred
    wchar_t         wszOccurredInFunction[MAX_NAME];
    /// The code line in the module where the error occurred
    int             nOccurredInLine;
    /// Pointer to the previous error code structure
    void*           pPreviousError;
} IfxErrorData;

/// Macro for storing error code information
#define ERROR_STORE(ERRORCODE, ERRORMESSAGE) Error_Store(FILENAME, __func__, __LINE__, ERRORCODE, ERRORMESSAGE, NULL)
#define ERROR_STORE_FMT(ERRORCODE, ERRORMESSAGE, ...) Error_Store(FILENAME, __func__, __LINE__, ERRORCODE, ERRORMESSAGE, __VA_ARGS__)

/// Macro for logging error code information
#define ERROR_LOGCODEANDMESSAGE(ERRORCODE, ERRORMESSAGE) Error_LogCodeAndMessage(FILENAME, __func__, __LINE__, ERRORCODE, ERRORMESSAGE, NULL)
#define ERROR_LOGCODEANDMESSAGE_FMT(ERRORCODE, ERRORMESSAGE, ...) Error_LogCodeAndMessage(FILENAME, __func__, __LINE__, ERRORCODE, ERRORMESSAGE, __VA_ARGS__)

/**
 *  @brief      Function to return the error stack
 *  @details    This function returns the first element in the error stack.
 *
 *  @returns    Returns the pointer to the first element in the error stack
 */
_Check_return_
IfxErrorData*
Error_GetStack();

/**
 *  @brief      Function to clear the error stack
 *  @details    This function clears all elements on the error stack.
 */
void
Error_ClearStack();

/**
 *  @brief      Function to clear the first item in the error stack
 *  @details    This function removes the first item of the error stack.
 */
void
Error_ClearFirstItem();

/**
 *  @brief      Function to store all parameters to an IfxErrorData structure
 *  @details    This function stores an error and its specific parameters for later use.
 *
 *  @param      PszOccurredInModule         Pointer to a char array holding the module name where the error occurred.
 *  @param      PszOccurredInFunction       Pointer to a char array holding the function name where the error occurred.
 *  @param      PnOccurredInLine            The line where the error occurred.
 *  @param      PunInternalErrorCode        Internal error code.
 *  @param      PwszInternalErrorMessage    Format string used to format the internal error message.
 *  @param      PvaArgumentList             Parameters needed to format the error message.
 */
_Check_return_
IfxErrorData*
Error_GetErrorData(
    _In_z_  const char*     PszOccurredInModule,
    _In_z_  const char*     PszOccurredInFunction,
    _In_    int             PnOccurredInLine,
    _In_    unsigned int    PunInternalErrorCode,
    _In_z_  const wchar_t*  PwszInternalErrorMessage,
    _In_    va_list         PvaArgumentList);

/**
 *  @brief      Function to store an error
 *  @details    This function stores an error and its specific parameters for later use.
 *              The error element is stored as the first element of the error list (stack).
 *
 *  @param      PszOccurredInModule         Pointer to a char array holding the module name where the error occurred.
 *  @param      PszOccurredInFunction       Pointer to a char array holding the function name where the error occurred.
 *  @param      PnOccurredInLine            The line where the error occurred.
 *  @param      PunInternalErrorCode        Internal error code.
 *  @param      PwszInternalErrorMessage    Format string used to format the internal error message.
 *  @param      ...                         Parameters needed to format the error message.
 */
void
IFXAPI
Error_Store(
    _In_z_  const char*     PszOccurredInModule,
    _In_z_  const char*     PszOccurredInFunction,
    _In_    int             PnOccurredInLine,
    _In_    unsigned int    PunInternalErrorCode,
    _In_z_  const wchar_t*  PwszInternalErrorMessage,
    ...);

/**
 *  @brief      Return the final error code
 *  @details    This function maps the given error code to the final one.
 *
 *  @retval     The final mapped error code.
 */
_Check_return_
unsigned int
Error_GetFinalCodeFromError(
    _In_ unsigned int PunErrorCode);

/**
 *  @brief      Return the final error code
 *  @details    This function maps the internal error code to the final one displayed to the end user.
 *
 *  @retval     The final mapped error code.
 */
_Check_return_
unsigned int
Error_GetFinalCode();

/**
 *  @brief      Return the final error code
 *  @details    This function maps the internal error code to the final one displayed to the user.
 *
 *  @param      PunErrorCode        Error Code to map to the final message.
 *  @param      PwszErrorMessage    Pointer to a char buffer to copy the error message to.
 *  @param      PpunBufferSize      Size of the error message buffer.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. E.g. PwszErrorMessage == NULL.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
Error_GetFinalMessageFromErrorCode(
    _In_                            unsigned int    PunErrorCode,
    _Out_z_cap_(*PpunBufferSize)    wchar_t*        PwszErrorMessage,
    _Inout_                         unsigned int*   PpunBufferSize);

/**
 *  @brief      Return the final error code
 *  @details    This function maps the internal error code to the final one displayed to the user.
 *
 *  @param      PwszErrorMessage    Pointer to a char buffer to copy the error message to.
 *  @param      PpunBufferSize      Size of the error message buffer.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. E.g. PwszErrorMessage == NULL.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
Error_GetFinalMessage(
    _Out_z_cap_(*PpunBufferSize)    wchar_t*        PwszErrorMessage,
    _Inout_                         unsigned int*   PpunBufferSize);

/**
 *  @brief      Return the internal error code
 *  @details    This function returns the internal error code.
 *
 *  @retval     The internal error code.
 */
_Check_return_
unsigned int
Error_GetInternalCode();

/**
 *  @brief      Log the error stack
 *  @details    This function logs the whole error stack.
 */
void
Error_LogStack();

/**
 *  @brief      Log error code and message
 *  @details    This function logs an error code and message.
 *
 *  @param      PszOccurredInModule         Pointer to a char array holding the module name where the error occurred.
 *  @param      PszOccurredInFunction       Pointer to a char array holding the function name where the error occurred.
 *  @param      PnOccurredInLine            The line where the error occurred.
 *  @param      PunInternalErrorCode        Internal error code.
 *  @param      PwszInternalErrorMessage    Format string used to format the internal error message.
 *  @param      ...                         Parameters needed to format the error message.
 */
void
IFXAPI
Error_LogCodeAndMessage(
    _In_z_  const char*     PszOccurredInModule,
    _In_z_  const char*     PszOccurredInFunction,
    _In_    int             PnOccurredInLine,
    _In_    unsigned int    PunInternalErrorCode,
    _In_z_  const wchar_t*  PwszInternalErrorMessage,
    ...);

/**
 *  @brief      Log IfxErrorData and linked IfxErrorData
 *  @details    This function logs an IfxErrorData object and all linked IfxErrorData objects.
 */
void
Error_LogErrorData(
    _In_    const IfxErrorData* PpErrorData);

#ifdef __cplusplus
}
#endif
