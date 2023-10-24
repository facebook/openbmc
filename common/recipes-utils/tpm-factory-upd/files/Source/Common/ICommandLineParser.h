/**
 *  @brief      Declares the command line parser interface
 *  @details    This interface declares some functions for a command line parser implementation
 *  @file       ICommandLineParser.h
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
 *  @brief      Initialize command line parsing
 *  @details
 *
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     ...         Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_InitializeParsing();

/**
 *  @brief      Parses the command line option
 *  @details    Parses the command line option identified by the common command line module code
 *
 *  @param      PwszCommandLineOption   Pointer to a wide character array containing the current option to work on.
 *  @param      PpunCurrentArgIndex     Pointer to an index for the current position in the argument list.
 *  @param      PnMaxArg                Maximum number of argument is the arguments list.
 *  @param      PrgwszArgv              Pointer to an array of wide character arrays representing the command line.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_BAD_COMMANDLINE    In case of an invalid command line.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_Parse(
    _In_z_                  const wchar_t*          PwszCommandLineOption,
    _Inout_                 unsigned int*           PpunCurrentArgIndex,
    _In_                    int                     PnMaxArg,
    _In_reads_z_(PnMaxArg)  const wchar_t* const    PrgwszArgv[]);

/**
 *  @brief      Finalize command line parsing
 *  @details
 *
 *  @param      PunReturnValue      Current return code which can be overwritten here.
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     ...         Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_FinalizeParsing(
    _In_                    unsigned int    PunReturnValue);

#ifdef __cplusplus
}
#endif
