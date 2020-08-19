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
	"bytes"
	"reflect"
	"sort"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestStringFind(t *testing.T) {
	cases := []struct {
		name string
		val  string
		arr  []string
		want int
	}{
		{
			name: "found in array (index returned)",
			val:  "foo",
			arr:  []string{"bar", "foo", "baz"},
			want: 1,
		},
		{
			name: "not found in array (-1 returned)",
			val:  "foo",
			arr:  []string{"bar", "baz"},
			want: -1,
		},
	}
	for _, tc := range cases {
		got := StringFind(tc.val, tc.arr)
		if tc.want != got {
			t.Errorf("want '%v' got '%v'", tc.want, got)
		}
	}
}

func TestStringDifference(t *testing.T) {
	a := []string{"a", "a", "b", "c", "d"}
	b := []string{"b", "b", "c", "d", "e"}
	want := []string{"a", "a"}
	got := StringDifference(a, b)
	if !reflect.DeepEqual(want, got) {
		t.Errorf("want '%v' got '%v'", want, got)
	}
}

func TestGetStringKeys(t *testing.T) {
	input := map[string]interface{}{
		"a": 1,
		"b": 2,
		"c": 3,
	}
	want := []string{"a", "b", "c"}

	got := GetStringKeys(input)

	// order is not guaranteed in a map, so we sort first
	sort.Strings(want)
	sort.Strings(got)

	if !reflect.DeepEqual(want, got) {
		t.Errorf("want '%v' got '%v'", want, got)
	}
}

func TestUint32Find(t *testing.T) {
	cases := []struct {
		name string
		val  uint32
		arr  []uint32
		want int
	}{
		{
			name: "found in array (index returned)",
			val:  42,
			arr:  []uint32{4, 42, 420},
			want: 1,
		},
		{
			name: "not found in array (-1 returned)",
			val:  42,
			arr:  []uint32{4, 420},
			want: -1,
		},
	}
	for _, tc := range cases {
		got := Uint32Find(tc.val, tc.arr)
		if tc.want != got {
			t.Errorf("want '%v' got '%v'", tc.want, got)
		}
	}
}

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

func TestSafeAppendBytes(t *testing.T) {
	a := make([]byte, 1, 2)
	a[0] = 'x'
	b := make([]byte, 1, 1)
	b[0] = 'y'
	c := SafeAppendBytes(a, b)

	if bytes.Compare(a, []byte{'x'}) != 0 {
		t.Errorf("a: want '%v' got '%v'", []byte{'x'}, a)
	}
	if bytes.Compare(c, []byte{'x', 'y'}) != 0 {
		t.Errorf("c: want '%v' got '%v'", []byte{'x', 'y'}, c)
	}
}

func TestSafeAppendString(t *testing.T) {
	a := make([]string, 1, 2)
	a[0] = "x"
	b := make([]string, 1, 1)
	b[0] = "y"
	c := SafeAppendString(a, b)

	if !reflect.DeepEqual(a, []string{"x"}) {
		t.Errorf("a: want '%v' got '%v'", []string{"x"}, a)
	}
	if !reflect.DeepEqual(c, []string{"x", "y"}) {
		t.Errorf("c: want '%v' got '%v'", []string{"x", "y"}, c)
	}
}

func TestGetWord(t *testing.T) {
	cases := []struct {
		name    string
		data    []byte
		offset  uint32
		want    uint32
		wantErr error
	}{
		{
			name:    "normal operation",
			data:    []byte{0x01, 0x02, 0x03, 0x04},
			offset:  0,
			want:    0x01020304,
			wantErr: nil,
		},
		{
			name:   "out of range",
			data:   []byte{0x01, 0x02, 0x03, 0x04},
			offset: 2,
			want:   0,
			wantErr: errors.Errorf("Required offset %v out of range of data size %v",
				2, 4),
		},
	}
	for _, tc := range cases {
		got, err := GetWord(tc.data, tc.offset)
		if tc.want != got {
			t.Errorf("want '%v' got '%v'", tc.want, got)
		}
		tests.CompareTestErrors(tc.wantErr, err, t)
	}
}

func TestSetWord(t *testing.T) {
	cases := []struct {
		name    string
		data    []byte
		word    uint32
		offset  uint32
		want    []byte
		wantErr error
	}{
		{
			name:    "normal operation",
			data:    []byte{0x01, 0x02, 0x03, 0x04},
			word:    0x12345678,
			offset:  0,
			want:    []byte{0x12, 0x34, 0x56, 0x78},
			wantErr: nil,
		},
		{
			name:   "out of range",
			data:   []byte{0x01, 0x02, 0x03, 0x04},
			word:   0x12345678,
			offset: 2,
			want:   nil,
			wantErr: errors.Errorf("Required offset %v out of range of data size %v",
				2, 4),
		},
	}
	for _, tc := range cases {
		got, err := SetWord(tc.data, tc.word, tc.offset)
		if bytes.Compare(tc.want, got) != 0 {
			t.Errorf("want '%v' got '%v'", tc.want, got)
		}
		tests.CompareTestErrors(tc.wantErr, err, t)
	}
}

func TestGetStringKeysFromJSONData(t *testing.T) {
	cases := []struct {
		name    string
		data    []byte
		want    []string
		wantErr error
	}{
		{
			name: "normal operation 1 entry",
			data: []byte("{\"a5ad8133574ceb63d492c5e7a75feb71\": " +
				"\"Signed: Wednesday Jul 17 13:44:40  2017\"}",
			),
			want:    []string{"a5ad8133574ceb63d492c5e7a75feb71"},
			wantErr: nil,
		},
		{
			name: "normal operation 2 entries, complex objects",
			data: []byte(`
{
	"hello": {
		"world": 42,
		"bigObject": {
			"hello": "world",
			"testing": {
				"testing42": false
			}
		},
		"thisIsNull": null
	},
	"testing123": 42
}`),
			want:    []string{"hello", "testing123"},
			wantErr: nil,
		},
		{
			name: "gibberish, no JSON",
			data: []byte("1254yewruifhqreifriqfhru43r4"),
			want: nil,
			wantErr: errors.Errorf("Unable to unmarshal JSON: " +
				"invalid character 'y' after top-level value"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := GetStringKeysFromJSONData(tc.data)
			tests.CompareTestErrors(tc.wantErr, err, t)

			// order is not guaranteed in a map, so we sort first
			sort.Strings(tc.want)
			sort.Strings(got)

			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

func TestCRC16(t *testing.T) {
	want := uint16(46124)
	got := CRC16([]byte{0x12, 0x34, 0x56, 0x78})
	if want != got {
		t.Errorf("want '%v' got '%v'", want, got)
	}
}
