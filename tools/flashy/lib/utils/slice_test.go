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
	"bytes"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestByteSliceRange(t *testing.T) {
	sampleS := []byte{1, 2, 3, 4, 5, 6, 7}
	cases := []struct {
		name    string
		s       []byte
		start   uint32
		end     uint32
		want    []byte
		wantErr error
	}{
		{
			name:    "OK",
			s:       sampleS,
			start:   3,
			end:     5,
			want:    sampleS[3:5],
			wantErr: nil,
		},
		{
			name:    "start > end",
			s:       sampleS,
			start:   3,
			end:     1,
			want:    nil,
			wantErr: errors.Errorf("Slice start (3) > end (1)"),
		},
		{
			name:    "end too large",
			s:       sampleS,
			start:   3,
			end:     10000,
			want:    nil,
			wantErr: errors.Errorf("Slice end (10000) larger than original slice length (7)"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := BytesSliceRange(tc.s, tc.start, tc.end)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !bytes.Equal(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}
