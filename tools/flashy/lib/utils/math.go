/**
 * Copyright 2021-present Facebook. All Rights Reserved.
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

import (
	"math"

	"github.com/pkg/errors"
)

// AddU32 returns an error if the result overflowed Uint32 bounds.
func AddU32(x, y uint32) (uint32, error) {
	if x > math.MaxUint32-y {
		return 0, errors.Errorf("Unsigned integer overflow for (%v+%v)", x, y)
	}
	return x + y, nil
}
