/*
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef HR_NANOSLEEP_H
#define HR_NANOSLEEP_H

#include <errno.h>
#include <stdint.h>
#include <time.h>

#define NANOSEC_IN_SEC (1000 * 1000 * 1000)
#define HR_NANOSLEEP_THRESHOLD_DEFAULT (1 * 1000 * 1000) // 1ms

/*
 * Before adding the high resolution timer support, either spin or nanosleep()
 * will not bring the process wakeup within 10ms. It turns out the system time
 * update is also controlled by HZ (100).
 *
 * After the high resolution timer is supported, the spin works as the
 * system time is updated more frequently. However, nanosleep() solution is
 * still noticeable slower comparing with spin. There could be some kernel
 * scheduling delay.
 *
 * hr_nanosleep() spins on CPU in the case if the 'ns' is smaller than
 * the threshold.
 */
static int hr_nanosleep_threshold(uint64_t ns, uint64_t threshold_ns) {
  struct timespec req, rem;
  int rc = 0;
  if (ns < threshold_ns) {
    uint64_t slept = 0;
    rc = clock_gettime(CLOCK_MONOTONIC, &req);
    while (!rc && slept < ns ) {
      rc = clock_gettime(CLOCK_MONOTONIC, &rem);
      slept = (rem.tv_sec - req.tv_sec) * NANOSEC_IN_SEC;
      if (rem.tv_nsec >= req.tv_nsec) {
        slept += rem.tv_nsec - req.tv_nsec;
      } else {
        slept -= req.tv_nsec - rem.tv_nsec;
      }
    }
  } else {
    req.tv_sec = 0;
    req.tv_nsec = ns;
    while ((rc = nanosleep(&req, &rem)) == -1 && errno == EINTR) {
      req = rem;
    }
  }
  return rc;
}

static int hr_nanosleep(uint64_t ns) {
  return hr_nanosleep_threshold(ns, HR_NANOSLEEP_THRESHOLD_DEFAULT);
}

#endif
