/*
 * Copyright 2020 Astera Labs, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file astera_log.h
 * @brief Logging module for Aries.
 */

#ifndef ASTERA_LOG_H_
#define ASTERA_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_VERSION "0.2.1"

typedef void (*logLockFn)(void* udata, int lock);

enum
{
    ASTERA_LOG_LEVEL_TRACE,
    ASTERA_LOG_LEVEL_DEBUG,
    ASTERA_LOG_LEVEL_INFO,
    ASTERA_LOG_LEVEL_WARN,
    ASTERA_LOG_LEVEL_ERROR,
    ASTERA_LOG_LEVEL_FATAL
};

#define ASTERA_TRACE(...)                                                      \
    asteraLogMsg(ASTERA_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define ASTERA_DEBUG(...)                                                      \
    asteraLogMsg(ASTERA_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define ASTERA_INFO(...)                                                       \
    asteraLogMsg(ASTERA_LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define ASTERA_WARN(...)                                                       \
    asteraLogMsg(ASTERA_LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define ASTERA_ERROR(...)                                                      \
    asteraLogMsg(ASTERA_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define ASTERA_FATAL(...)                                                      \
    asteraLogMsg(ASTERA_LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void asteraLogSetUdata(void* udata);
void asteraLogSetLock(logLockFn fn);
void asteraLogSetFp(FILE* fp);
void asteraLogSetCallback(void (*ptr)());
void asteraLogSetLevel(int level);
void asteraLogSetQuiet(int enable);

void asteraLogMsg(int level, const char* file, int line, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LOG_H_ */
