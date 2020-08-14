/**
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

package utils

// IsPfrSystem checks whether the system is a PFR-enabled system or not.
// This is just a temporary method, due to be replaced by pfr-util.
var IsPfrSystem = func() bool {
	// i2craw 12 0x38 -r 1 -w "0x0A"
	exitCode, _, _, _ := RunCommand(
		[]string{"i2craw", "12", "0x38", "-r", "1", "-w", "0x0A"},
		30,
	)
	return exitCode == 0
}
