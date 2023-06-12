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

#include "include/astera_log.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct
{
    void* udata;
    logLockFn lock;
    FILE* fp;
    void (*ptr)();
    int level;
    int quiet;
    bool traceEn;
} AsteraLogger;

static const char* logLevelNames[] = {"TRACE", "DEBUG", "INFO",
                                      "WARN",  "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static const char* logLevelColors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m",
                                       "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

static void lock(void)
{
    if (AsteraLogger.lock)
    {
        AsteraLogger.lock(AsteraLogger.udata, 1);
    }
}

static void unlock(void)
{
    if (AsteraLogger.lock)
    {
        AsteraLogger.lock(AsteraLogger.udata, 0);
    }
}

void asteraLogSetUdata(void* udata)
{
    AsteraLogger.udata = udata;
}

void asteraLogSetLock(logLockFn fn)
{
    AsteraLogger.lock = fn;
}

void asteraLogSetFp(FILE* fp)
{
    AsteraLogger.fp = fp;
}

void asteraLogSetCallback(void (*ptr)())
{
    AsteraLogger.ptr = ptr;
}

void asteraLogSetLevel(int level)
{
    AsteraLogger.level = level;
    if (level == 0)
    {
        AsteraLogger.traceEn = true;
    }
}

void asteraLogSetQuiet(int enable)
{
    AsteraLogger.quiet = enable ? 1 : 0;
}

void asteraLogMsg(int level, const char* file, int line, const char* fmt, ...)
{
    if (level == 0 && !(AsteraLogger.traceEn))
    {
        return;
    }
    else if (level < AsteraLogger.level)
    {
        return;
    }

    /* Acquire lock */
    lock();

    /* Get current time */
    time_t t = time(NULL);
    struct tm* lt = localtime(&t);

    /* Log to stderr */
    if (!AsteraLogger.quiet)
    {
        va_list args;
        char buf[16];
        buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';
#ifdef LOG_USE_COLOR
        fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
                logLevelColors[level], logLevelNames[level], file, line);
#else
        fprintf(stderr, "%s %-5s %s:%d: ", buf, logLevelNames[level], file,
                line);
#endif
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    /* Log to file */
    if (AsteraLogger.fp)
    {
        va_list args;
        char buf[32];
        buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
        fprintf(AsteraLogger.fp, "%s %-5s %s:%d: ", buf, logLevelNames[level],
                file, line);
        va_start(args, fmt);
        vfprintf(AsteraLogger.fp, fmt, args);
        va_end(args);
        fprintf(AsteraLogger.fp, "\n");
        fflush(AsteraLogger.fp);
    }

    /* Call funtion callback */
    if (AsteraLogger.ptr)
    {
        va_list args;
        char buf[32];
        char string[256];
        buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt)] = '\0';
        sprintf(string, "%s %-5s %s:%d: ", buf, logLevelNames[level], file,
                line);
        AsteraLogger.ptr(string);
        va_start(args, fmt);
        vsprintf(string, fmt, args);
        AsteraLogger.ptr(string);
        va_end(args);
        sprintf(string, "\n");
        AsteraLogger.ptr(string);
    }

    /* Release lock */
    unlock();
}

#ifdef __cplusplus
}
#endif
