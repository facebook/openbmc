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

import (
	"strings"
	"testing"
	"time"
)

func TestIsPfrSystem(t *testing.T) {
	runCommandOrig := RunCommand
	defer func() {
		RunCommand = runCommandOrig
	}()

	cases := []struct {
		name     string
		exitCode int
		want     bool
	}{
		{
			name:     "is pfr system: exit 0",
			exitCode: 0,
			want:     true,
		},
		{
			name:     "not pfr system: exit 255",
			exitCode: 255,
			want:     false,
		},
	}

	wantCmd := "i2craw 12 0x38 -r 1 -w 0x0A"
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				gotCmd := strings.Join(cmdArr, " ")
				if wantCmd != gotCmd {
					t.Errorf("cmd: want '%v' got '%v'", wantCmd, gotCmd)
				}
				return tc.exitCode, nil, "", ""
			}
			got := IsPfrSystem()
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}
