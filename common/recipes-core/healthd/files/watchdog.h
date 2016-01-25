/*
 * watchdog
 *
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
 *
 * Utility library to handle the aspeed watchdog. Only one watchdog
 * should be in use at any time throughout the entire system (multiple users
 * will not cause adverse effects but the behavior of the watchdog becomes
 * undefined).
 *
 * The watchdog can be started in either manual or automatic mode. In manual
 * mode, the watchdog has to be constantly reset by the user via the
 * kick_watchdog() function. Under automatic mode, the watchdog will
 * run in a separate thread and reset the timer on its own so no intervention
 * is required from the user.
 *
 * In both modes, the watchdog timer will not stop when the process is
 * terminated, unless a call to stop_watchdog() has been made beforehand, or
 * if the user runs in manual mode and uses a non persistent watchdog kick.
 *
 * The default timeout for the watchdog is 11 seconds. When this time period
 * elapses, the ARM chip is restarted and the kernel is rebooted. Other
 * hardware state is not reset, so this may introduce strange behavior on
 * reboot (example: an I2C bus may be left in the open state, triggering
 * constant interrupts). In rare cases, this could result in the kernel
 * failing to fully restart itself and thus preclude the possibility of
 * reinitializing the watchdog timer. Someone will then have to go over and
 * physically restart the machine.
 *
 * The alternative to the soft reset is to request the watchdog device driver
 * for a hard reset on timeout. However this will stop the fans. If the
 * kernel fails to fully boot and restart the fan daemon, the system could
 * overheat. For this reason, we've chosen to take the risk of a stuck soft
 * reset instead.
 *
 */

/* Forward declarations. */
int start_watchdog(const int auto_mode);
enum watchdog_persistent_en {
  WATCHDOG_SET_PERSISTENT,
  WATCHDOG_SET_NONPERSISTENT,
};
void set_persistent_watchdog(enum watchdog_persistent_en persistent);
int kick_watchdog();
void stop_watchdog();
