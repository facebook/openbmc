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
	"github.com/pkg/errors"
)

// BytesSliceRange returns s[start:end] but with bounds checks
func BytesSliceRange(s []byte, start, end uint32) ([]byte, error) {
	if start > end {
		return nil, errors.Errorf("Slice start (%v) > end (%v)", start, end)
	}
	if end > uint32(len(s)) {
		return nil, errors.Errorf("Slice end (%v) larger than original slice length (%v)", end, len(s))
	}
	return s[start:end], nil
}
