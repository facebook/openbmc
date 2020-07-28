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

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestRegexSubexpMapHelper(t *testing.T) {
	cases := []struct {
		name        string
		match       []string
		subexpNames []string
		want        map[string]string
		wantErr     error
	}{
		{
			name:        "basic passing",
			match:       []string{"", "foo", "bar"},
			subexpNames: []string{"", "F", "B"},
			want: map[string]string{
				"F": "foo",
				"B": "bar",
			},
			wantErr: nil,
		},
		{
			name:        "Incomplete match",
			match:       []string{"", "foo"},
			subexpNames: []string{"", "F", "B"},
			want:        map[string]string{},
			wantErr: errors.Errorf("Incomplete match '%#v' for subexpNames '%#v'",
				[]string{"", "foo"}, []string{"", "F", "B"}),
		},
		{
			name:        "empty match and subexpNames",
			match:       []string{},
			subexpNames: []string{},
			want:        map[string]string{},
			wantErr:     nil,
		},
		{
			name:        "duplicate subexpName",
			match:       []string{"", "foo", "bar"},
			subexpNames: []string{"", "f", "f"},
			want: map[string]string{
				"f": "foo",
			},
			wantErr: errors.Errorf("Duplicate subexpName 'f' found. Make sure " +
				"the regEx capturing group names are unique."),
		},
		{
			name:        "empty subexpName",
			match:       []string{"", "foo"},
			subexpNames: []string{"", ""},
			want:        map[string]string{},
			wantErr: errors.Errorf("Invalid empty subexpName, subexpNames must have " +
				"non-empty strings (except for the 1st entry)"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := regexSubexpMapHelper(tc.match, tc.subexpNames)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want '%v', got '%v'", tc.want, got)
			}
		})
	}
}

func TestGetRegexSubexpMap(t *testing.T) {
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
			name:    "malformed regex string",
			regEx:   `(?P<type>[a-z]+`,
			input:   "123mtdabc:123abc",
			want:    map[string]string{},
			wantErr: errors.Errorf("error parsing regexp: missing closing ): `(?P<type>[a-z]+`"),
		},
		{
			name:    "Invalid empty pattern",
			regEx:   `(?P<type>[a-z]+):(?P<>[0-9]+)`,
			input:   "123mtdabc:123abc",
			want:    map[string]string{},
			wantErr: errors.Errorf("error parsing regexp: invalid named capture: `(?P<>`"),
		},
		{
			name:  "Duplicate capturing group name",
			regEx: `(?P<type>[a-z]+):(?P<type>[0-9]+)`,
			input: "123mtdabc:123abc",
			want: map[string]string{
				"type": "mtdabc",
			},
			wantErr: errors.Errorf("Duplicate subexpName 'type' found. " +
				"Make sure the regEx capturing group names are unique."),
		},
		{
			name:  "Called with regex with unnamed parenthesized group",
			regEx: `a(x*)b`,
			input: "-ab-",
			want:  map[string]string{},
			wantErr: errors.Errorf("Invalid empty subexpName, subexpNames must have non-empty strings " +
				"(except for the 1st entry)"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := GetRegexSubexpMap(tc.regEx, tc.input)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want %v got %v", tc.want, got)
			}
		})
	}
}

func TestGetBytesRegexSubexpMap(t *testing.T) {
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
			wantErr: errors.Errorf("No match for regex '(?P<type>[a-z]+)' for input"),
		},
		{
			name:  "mulitple groups, no match",
			regEx: `(?P<type>[a-z]+):(?P<specifier>[0-9]+)`,
			input: "123mtdabc:abc",
			want:  map[string]string{},
			wantErr: errors.Errorf("No match for regex '(?P<type>[a-z]+):(?P<specifier>[0-9]+)' " +
				"for input"),
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
			name:    "malformed regex string",
			regEx:   `(?P<type>[a-z]+`,
			input:   "123mtdabc:123abc",
			want:    map[string]string{},
			wantErr: errors.Errorf("error parsing regexp: missing closing ): `(?P<type>[a-z]+`"),
		},
		{
			name:    "Invalid empty pattern",
			regEx:   `(?P<type>[a-z]+):(?P<>[0-9]+)`,
			input:   "123mtdabc:123abc",
			want:    map[string]string{},
			wantErr: errors.Errorf("error parsing regexp: invalid named capture: `(?P<>`"),
		},
		{
			name:  "Duplicate capturing group name",
			regEx: `(?P<type>[a-z]+):(?P<type>[0-9]+)`,
			input: "123mtdabc:123abc",
			want: map[string]string{
				"type": "mtdabc",
			},
			wantErr: errors.Errorf("Duplicate subexpName 'type' found. " +
				"Make sure the regEx capturing group names are unique."),
		},
		{
			name:  "Called with regex with unnamed parenthesized group",
			regEx: `a(x*)b`,
			input: "-ab-",
			want:  map[string]string{},
			wantErr: errors.Errorf("Invalid empty subexpName, subexpNames must have non-empty strings " +
				"(except for the 1st entry)"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := GetBytesRegexSubexpMap(tc.regEx, []byte(tc.input))
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want %v got %v", tc.want, got)
			}
		})
	}
}

func TestGetAllRegexSubexpMap(t *testing.T) {
	cases := []struct {
		name    string
		regEx   string
		input   string
		want    [](map[string]string)
		wantErr error
	}{
		{
			name:  "basic one match test",
			regEx: `(?P<type>[a-z]+):(?P<specifier>.+)`,
			input: "mtd:flash0",
			want: [](map[string]string){
				map[string]string{
					"type":      "mtd",
					"specifier": "flash0",
				},
			},
			wantErr: nil,
		},
		{
			name:  "basic multiple matches test",
			regEx: `(?P<type>[a-z]+):(?P<specifier>.+)`,
			input: `mtd:flash0
 mtd:flash1
 mtd:flash2`,
			want: [](map[string]string){
				map[string]string{
					"type":      "mtd",
					"specifier": "flash0",
				},
				map[string]string{
					"type":      "mtd",
					"specifier": "flash1",
				},
				map[string]string{
					"type":      "mtd",
					"specifier": "flash2",
				},
			},
			wantErr: nil,
		},
		{
			name:    "no matches found",
			regEx:   `(?P<type>[a-z]+):(?P<specifier>.+)`,
			input:   `mtd-flash0`,
			want:    [](map[string]string){},
			wantErr: nil,
		},
		{
			name:    "malformed regex",
			regEx:   `(?P<type>[a-z]+`,
			input:   `mtd:flash0`,
			want:    [](map[string]string){},
			wantErr: errors.Errorf("error parsing regexp: missing closing ): `(?P<type>[a-z]+`"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := GetAllRegexSubexpMap(tc.regEx, tc.input)

			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want %v got %v", tc.want, got)
			}
		})
	}
}
