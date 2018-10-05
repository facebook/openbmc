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
 *
 * Utility library to handle the aspeed watchdog. Only one watchdog
 * should be in use at any time throughout the entire system.
 */

#ifndef _WDT_CTRL_H_
#define _WDT_CTRL_H_

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
 * NOTE:
 *   - open_watchdog() can be called only once, and subsequent calls would
 *     fail (no matter it's called from current or different processes).
 */
int open_watchdog(int auto_kick, int auto_kick_interval);

/*
 * release_watchdog() frees all the resources allocated by open_watchdog().
 * NOTE:
 *   - release_watchdog() does not stop the watchdog by default. In order
 *     to stop watchdog, you will need to either call stop_watchdog() or
 *     watchdog_enable_magic_close() before calling release_watchdog().
 */
void release_watchdog(void);

/*
 * kick_watchdog() kicks/feeds the watchdog by resetting watchdog timer.
 */
int kick_watchdog(void);

/*
 * start|stop_watchdog() starts/stops watchdog explicitly. Most times,
 * these functions are not needed because open|feed|release_watchdog()
 * can take care of the watchdog very well.
 * Think it over before calling stop_watchdog(), because if system gets
 * stuck after the call, watchdog won't help us reboot the system.
 */
int start_watchdog(void);
int stop_watchdog(void);

/*
 * Magic close feature.
 * By default, magic close feature is disabled, which means closing
 * watchdog device file (for example, by calling release_watchdog())
 * doesn't stop watchdog timer. To turn on magic close feature, you
 * need to call watchdog_enable_magic_close() explicitly.
 * Similar to stop_watchdog(), use watchdog_enable_magic_close() with
 * causion: if the process who kicks watchdog crashes, the watchdog
 * will be stopped automatically.
 */
int watchdog_enable_magic_close(void);
int watchdog_disable_magic_close(void);

/*
 * set_persistent_watchdog() is defined in old libwatchdog-0.1, and
 * it's no-op in this version (0.2).
 */
#define set_persistent_watchdog(p)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* _WDT_CTRL_H_ */
