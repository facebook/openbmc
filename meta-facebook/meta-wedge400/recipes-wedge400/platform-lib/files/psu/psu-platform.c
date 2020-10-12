/*
 * Copyright 2020-present Facebook. All Rights Reserved.
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
 * This file is intended to contain platform specific definitions, and
 * it's set to empty intentionally. Each platform needs to override the
 * file if platform specific settings are required.
 */
#include "psu.h"

void sensord_operation(uint8_t num, uint8_t action)
{
  if (action == STOP) {
    syslog(LOG_WARNING, "Stop monitor PSU%d sensor to update", num + 1);
    run_command("sv stop sensord > /dev/null");
    switch (num) {
      case 0:
        run_command("/usr/local/bin/sensord scm smb psu2 > /dev/null 2>&1 &");
        break;
      case 1:
        run_command("/usr/local/bin/sensord scm smb psu1 > /dev/null 2>&1 &");
        break;
    }
  } else if (action == START) {
    run_command("killall sensord");
    run_command("sv start sensord > /dev/nul");
    syslog(LOG_WARNING, "Start monitor PSU%d sensor", num + 1);
  }
}
