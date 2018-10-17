/*
 * Copyright 2015-present Facebook. All Rights Reserved.
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

/*
 * Utility library to handle the aspeed watchdog. Only one watchdog
 * should be in use at any time throughout the entire system.
 */

#ifndef _OBMC_WATCHDOG_H_
#define _OBMC_WATCHDOG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * open_watchdog() opens watchdog device file, which also starts watchdog
 * timer automatically.
 * The watchdog can be opened in either manual or auto-kick mode:
 *   - manual mode (<auto_kick> == 0):
 *     The caller needs to kick the watchdog itself (by kick_watchdog()),
 *     and the second parameter <auto_kick_interval> is ignored.
 *   - auto-kick mode (<auto_kick> != 0):
 *     open_watchdog() function will create a separate thread which kicks
 *     watchdog peridically, and kick interval is specified by the second
 *     parameter <auto_kick_interval>.
 * The function returns 0 for success, and -1 on failures.
 * NOTES:
 *   - In linux 4.17, open_watchdog() can be called only once, and
 *     subsequent calls would fail (no matter it's called from current
 *     or different processes).
 */
int open_watchdog(int auto_kick, int auto_kick_interval);

/*
 * release_watchdog() frees all the resources allocated by open_watchdog().
 * NOTES:
 *   - release_watchdog() does not stop the watchdog by default. In order
 *     to stop watchdog, people have 2 options:
 *       * call stop_watchdog() explicitly
 *       * call watchdog_enable_magic_close() JUST before calling
 *         release_watchdog().
 */
void release_watchdog(void);

/*
 * kick_watchdog() kicks/feeds the watchdog by resetting watchdog timer.
 * The function returns 0 for success, and -1 on failures.
 */
int kick_watchdog(void);

/*
 * start|stop_watchdog() starts and stops watchdog explicitly.
 * Both functions return 0 for success, and -1 on failures.
 * NOTES:
 *   - Think it over before calling stop_watchdog(), because if system
 *      gets stuck after the call, the watchdog won't help to reboot the
 *      system.
 */
int start_watchdog(void);
int stop_watchdog(void);

/*
 * Functions to enable/disable magic close feature. Both functions return
 * 0 for success, and -1 on failures.
 * NOTES:
 *   - Magic close feature is disabled by default, which means closing
 *     watchdog device file doesn't stop watchdog timer.
 *   - Use watchdog_enable_magic_close() with caution, because if the
 *     process who kicks watchdog crashes right after enabling magic
 *     close feature, the watchdog will be stopped.
 */
int watchdog_enable_magic_close(void);
int watchdog_disable_magic_close(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _OBMC_WATCHDOG_H_ */
