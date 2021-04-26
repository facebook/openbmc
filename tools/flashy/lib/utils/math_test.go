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
	"testing"

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestAddU32(t *testing.T) {
	cases := []struct {
		name    string
		x       uint32
		y       uint32
		want    uint32
		wantErr error
	}{
		{
			name:    "OK",
			x:       32,
			y:       42,
			want:    32 + 42,
			wantErr: nil,
		},
		{
			name:    "Overflowed",
			x:       math.MaxUint32 - 1,
			y:       2,
			want:    0,
			wantErr: errors.Errorf("Unsigned integer overflow for (4294967294+2)"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := AddU32(tc.x, tc.y)
			if err != nil {
				tests.CompareTestErrors(tc.wantErr, err, t)
			} else if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}
