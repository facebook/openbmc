/*
Copyright (c) 2017, Intel Corporation
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "logging.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static bool prnt_irdr = false, prnt_net = false,
     prnt_jtagMsg = false, prnt_Debug = false,
     write_to_syslog = false;

static bool ShouldLog(ASD_LogType log_type)
{
    bool log = false;
    if (log_type == LogType_IRDR && prnt_irdr) {
        log = true;
    } else if (log_type == LogType_NETWORK && prnt_net) {
        log = true;
    } else if (log_type == LogType_JTAG && prnt_jtagMsg) {
        log = true;
    } else if (log_type == LogType_Debug && prnt_Debug) {
        log = true;
    } else if (log_type == LogType_Error) {
        log = true;
    }
    return log;
}

void ASD_log(ASD_LogType log_type, const char *format, ...)
{
    if (!ShouldLog(log_type))
        return;

    va_list args;
    va_start(args, format);
    if (write_to_syslog) {
        vsyslog(LOG_USER, format, args);
    } else {
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
    va_end(args);
}

void ASD_log_buffer(ASD_LogType log_type,
                    const unsigned char *ptr, size_t len,
                    const char *prefixPtr)
{
    const unsigned char *ubuf = ptr;
    unsigned int i = 0, l = 0;
    unsigned char *h;
    char line[256];
    static const char itoh[] = "0123456789abcdef";

    if (!ShouldLog(log_type))
        return;

    /*  0         1         2         3         4         5         6
     *  0123456789012345678901234567890123456789012345678901234567890123456789
     *  PREFIX: 0000000: 0000 0000 0000 0000 0000 0000 0000 0000
     */
    while (i < len) {
        memset(line, '\0', sizeof(line));
        snprintf(line, sizeof(line), "%-6.6s: %07x: ", prefixPtr, i);
        h = (unsigned char *)&line[17];
        for (l = 0; l < 16 && (l + i) < len; l++) {
            *h++ = itoh[(*ubuf) >> 4];
            *h++ = itoh[(*ubuf) & 0xf];
            if (l & 1)
                *h++ = ' ';
            ubuf++;
        }
        *h++ = '\n';
        i += l;
        if (write_to_syslog)
            syslog(LOG_USER, "%s", line);
        else
            fprintf(stderr, "%s", line);
    }
}

void ASD_initialize_log_settings(ASD_LogType type)
{
    prnt_irdr = false;
    prnt_net = false;
    prnt_jtagMsg = false;
    prnt_Debug = false;

    switch (type) {
        case LogType_Syslog:
            write_to_syslog = true;
            break;
        case LogType_IRDR: {
            prnt_irdr = true;
            fprintf(stderr, "IR/DR messages are enabled\n");
            break;
        }
        case LogType_NETWORK: {
            prnt_net = true;
            fprintf(stderr, "Network response messages are enabled\n");
            break;
        }
        case LogType_JTAG: {
            prnt_jtagMsg = true;
            fprintf(stderr, "JTAG packet messages are enabled\n");
            break;
        }
        case LogType_All: {
            prnt_jtagMsg = true;
            prnt_irdr = true;
            prnt_net = true;
            prnt_Debug = true;
            fprintf(stderr, "IR/DR messages are enabled\n");
            fprintf(stderr, "Network response messages are enabled\n");
            fprintf(stderr, "JTAG packet messages are enabled\n");
            fprintf(stderr, "Debug messages are enabled\n");
            break;
        }
        case LogType_Debug: {
            prnt_Debug = true;
            fprintf(stderr, "Debug messages are enabled\n");
            break;
        }
        case LogType_Error:
        case LogType_None:
            break;
        case LogType_MIN:
        case LogType_MAX: {
            fprintf(stderr, "Invalid log setting\n");
            break;
        }
    }
}
