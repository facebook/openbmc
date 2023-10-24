/**
 *  @brief      Implements the TPMFactoryUpd main program
 *  @details
 *  @file       Linux\TPMFactoryUpd.c
 *
 *  Copyright 2016 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <malloc.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include "StdInclude.h"
#include "Controller.h"

/**
 *  @brief      Main program entry point
 *  @details
 *
 *  @param      PnArgc      Number of command line arguments.
 *  @param      PrgszArgv   Pointer to the command line arguments.
 *  @retval     0           TPMFactoryUpd completed successfully.
 *  @retval     1           TPMFactoryUpd failed.
 */
int main(int PnArgc, char* PrgszArgv[])
{
    wchar_t** prgwszArgv = NULL;
    unsigned int unReturnCode = -1;
    int nCount = -1;
    int nFreeCount = -1;

    do
    {
        // Convert command line arguments from multibyte character strings to wide character
        // strings.
        prgwszArgv = (wchar_t**)malloc(PnArgc * sizeof(wchar_t*));
        for (nCount = 0; nCount < PnArgc; nCount++)
        {
            size_t sizeRet = 0;

            // First get the size of the wide character string.
            size_t sizeArgv = mbstowcs(NULL, PrgszArgv[nCount], 0);

            // Allocate wide character string and add additional byte to allow
            // for \0 termination.
            prgwszArgv[nCount] = (wchar_t*)calloc(sizeArgv + 1, sizeof(wchar_t));
            if (NULL == prgwszArgv[nCount])
            {
                perror("Memory allocation failed");
                unReturnCode = RC_E_FAIL;
                break;
            }

            // Convert the multibyte character string to a wide character string
            sizeRet = mbstowcs(prgwszArgv[nCount], PrgszArgv[nCount], sizeArgv + 1);
            if ((size_t) - 1 == sizeRet)
            {
                perror("String conversion failed");
                unReturnCode = RC_E_FAIL;
                break;
            }
            unReturnCode = RC_SUCCESS;
        }

        // Verify if conversion succeeded
        if (RC_SUCCESS != unReturnCode)
        {
            break;
        }

        // Run TPMFactoryUpd
        unReturnCode = Controller_Proceed(PnArgc, (const wchar_t* const*)prgwszArgv);
    }
    WHILE_FALSE_END;

    // Free up conversion memory
    for (nFreeCount = 0; nFreeCount < nCount; nFreeCount++)
    {
        free(prgwszArgv[nFreeCount]);
    }
    free(prgwszArgv);

    // Map "unsigned int" error code to 0/1:
    //  0   => 0
    //  !0  => 1
    return unReturnCode == 0 ? 0 : 1;
}
