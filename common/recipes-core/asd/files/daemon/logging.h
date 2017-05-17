/*
* Logging functions header
* Copyright (c) 2017, Intel Corporation.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*/

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    LogType_MIN = -1,
    LogType_None,
    LogType_IRDR,
    LogType_NETWORK,
    LogType_JTAG,
    LogType_All,
    LogType_Debug,
    LogType_Error,
    LogType_Syslog,
    LogType_MAX,
} ASD_LogType;

#ifdef __cplusplus
extern "C" {
#endif

void ASD_log(ASD_LogType log_type, const char* format, ...);

void ASD_log_buffer(ASD_LogType log_type, const unsigned char* ptr, size_t len,
                    const char* prefixPtr);

void ASD_initialize_log_settings(ASD_LogType type);

#ifdef __cplusplus
}
#endif

#endif  // _LOGGING_H_
