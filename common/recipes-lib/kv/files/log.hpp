#pragma once

/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright 2015-present Facebook. All Rights Reserved.
 */

#ifdef DEBUG
#ifdef __TEST__
#define KV_DEBUG(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__)
#else
#define KV_DEBUG(fmt, ...) syslog(LOG_WARNING, fmt, ## __VA_ARGS__)
#endif
#else
#define KV_DEBUG(fmt, ...)
#endif

#ifdef __TEST__
#define KV_WARN(fmt, ...) fprintf(stderr, "WARNING: " fmt "\n", ## __VA_ARGS__)
#else
#define KV_WARN(fmt, ...) syslog(LOG_WARNING, fmt, ## __VA_ARGS__)
#endif
