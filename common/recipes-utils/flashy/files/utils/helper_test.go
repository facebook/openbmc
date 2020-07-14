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
	"reflect"
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

func TestGetRegexMap(t *testing.T) {
	cases := []struct {
		name    string
		regEx   string
		input   string
		want    map[string]string
		wantErr error
	}{
		{
			name:  "basic test",
			regEx: `(?P<type>[a-z]+):(?P<specifier>.+)`,
			input: "mtd:flash0",
			want: map[string]string{
				"type":      "mtd",
				"specifier": "flash0",
			},
			wantErr: nil,
		},
		{
			name:    "no matches",
			regEx:   `(?P<type>[a-z]+)`,
			input:   "123",
			want:    map[string]string{},
			wantErr: errors.Errorf("No match for regex '(?P<type>[a-z]+)' for input '123'"),
		},
		{
			name:  "mulitple groups, no match",
			regEx: `(?P<type>[a-z]+):(?P<specifier>[0-9]+)`,
			input: "123mtdabc:abc",
			want:  map[string]string{},
			wantErr: errors.Errorf("No match for regex '(?P<type>[a-z]+):(?P<specifier>[0-9]+)' " +
				"for input '123mtdabc:abc'"),
		},
		{
			name:  "partially matched front",
			regEx: `(?P<type>[a-z]+)`,
			input: "mtdabc123",
			want: map[string]string{
				"type": "mtdabc",
			},
			wantErr: nil,
		},
		{
			name:  "partially matched back",
			regEx: `(?P<type>[a-z]+)`,
			input: "123mtdabc",
			want: map[string]string{
				"type": "mtdabc",
			},
			wantErr: nil,
		},
		{
			name:  "partially matched middle",
			regEx: `(?P<type>[a-z]+):(?P<specifier>[0-9]+)`,
			input: "123mtdabc:123abc",
			want: map[string]string{
				"type":      "mtdabc",
				"specifier": "123",
			},
			wantErr: nil,
		},
		{
			name:    "malformed regex string",
			regEx:   `(?P<type>[a-z]+`,
			input:   "123mtdabc:123abc",
			want:    map[string]string{},
			wantErr: errors.Errorf("error parsing regexp: missing closing ): `(?P<type>[a-z]+`"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := getRegexSubexpMap(tc.regEx, tc.input)

			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want %v got %v", tc.want, got)
			}
		})
	}
}
