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
	"encoding/binary"
	"encoding/json"
	"fmt"
	"log"
	"regexp"
	"time"

	"github.com/pkg/errors"
)

// Takes a string slice and looks if a given item is in it. If found it will
// return its index, otherwise it will return -1
func StringFind(val string, arr []string) int {
	for i, item := range arr {
		if item == val {
			return i
		}
	}
	return -1
}

// StringDifference returns the elements in 'a' not in 'b'
func StringDifference(a, b []string) []string {
	mb := make(map[string]struct{}, len(b))
	for _, x := range b {
		mb[x] = struct{}{}
	}
	var diff []string
	for _, x := range a {
		if _, found := mb[x]; !found {
			diff = append(diff, x)
		}
	}
	return diff
}

// get all the string keys in a string-keyed map.
// the sequence of the keys is not guaranteed.
func GetStringKeys(m map[string]interface{}) []string {
	i := 0
	keys := make([]string, len(m))
	for k := range m {
		keys[i] = k
		i++
	}
	return keys
}

// Takes a uint32 slice and looks if a given item is in it. If found it will
// return its index, otherwise it will return -1
func Uint32Find(val uint32, arr []uint32) int {
	for i, item := range arr {
		if item == val {
			return i
		}
	}
	return -1
}

// helper function for GetRegexSubexpMap & GetAllRegexSubexpMap
// `match` must already have matched with the subexpNames, otherwise an
// error will be returned
// e.g. match: []string{"", "foo", "bar"}
//      subexpNames: []string{"", "F", "B"}
// -> map[string]string { "F": "foo", "B": "bar" }
func regexSubexpMapHelper(match, subexpNames []string) (map[string]string, error) {
	m := map[string]string{}

	// unmatched, this should not happen if this function
	// is called with a valid `match`
	if len(match) != len(subexpNames) {
		return m, errors.Errorf("Incomplete match '%#v' for subexpNames '%#v'",
			match, subexpNames)
	}

	for i, name := range subexpNames {
		// i > 0 to skip the first empty string returned by
		// FindStringSubmatch
		if i > 0 {

			// invalid subexpName with empty strings
			if len(name) == 0 {
				return m, errors.Errorf("Invalid empty subexpName, subexpNames must have " +
					"non-empty strings (except for the 1st entry)")
			}

			// dupe capturing group name
			if _, ok := m[name]; ok {
				return m, errors.Errorf(
					fmt.Sprintf("Duplicate subexpName '%v' found. Make sure ", name) +
						"the regEx capturing group names are unique.",
				)
			}

			m[name] = match[i]
		}
	}

	return m, nil
}

// given a regex with capturing groups and an input string
// return a map of the regex subexpNames to their respective matched
// values
// NOTE: this function is used only for regex with valid capturing groups
// uses beyond well-defined regexes with valid capturing groups will
// result in errors or undefined behavior
// e.g. regex: "(?P<type>[a-z]+):(?P<specifier>.+)"
//      inputString: "mtd:flash0"
// -> map[string]string { "type": "mtd", "specifier": "flash0" }
// return error if match was not successful
func GetRegexSubexpMap(regEx, inputString string) (map[string]string, error) {
	subexpMap := map[string]string{}

	reg, err := regexp.Compile(regEx)
	if err != nil {
		return subexpMap, err
	}

	match := reg.FindStringSubmatch(inputString)
	if match == nil {
		return subexpMap, errors.Errorf("No match for regex '%v' for input '%v'",
			regEx, inputString)
	}

	subexpNames := reg.SubexpNames()
	subexpMap, err = regexSubexpMapHelper(match, subexpNames)
	if err != nil {
		return subexpMap, err
	}

	return subexpMap, nil
}

// 'All' version of GetRegexSubexpMap that tries to match as many strings as possible
// return empty list of subexpMap if no matches found
func GetAllRegexSubexpMap(regEx, inputString string) ([](map[string]string), error) {
	allSubexpMap := [](map[string]string){}

	reg, err := regexp.Compile(regEx)
	if err != nil {
		return allSubexpMap, err
	}

	matches := reg.FindAllStringSubmatch(inputString, -1)

	subexpNames := reg.SubexpNames()
	for _, match := range matches {
		subexpMap, err := regexSubexpMapHelper(match, subexpNames)
		if err != nil {
			return allSubexpMap, nil
		}
		allSubexpMap = append(allSubexpMap, subexpMap)
	}

	return allSubexpMap, nil
}

// bytes version of GetRegexSubexpMap for convenience, image/mtds have long slices of bytes.
// For convenience, map[string]string is returned
func GetBytesRegexSubexpMap(regEx string, buf []byte) (map[string]string, error) {
	subexpMap := map[string]string{}

	reg, err := regexp.Compile(regEx)
	if err != nil {
		return subexpMap, err
	}

	match := reg.FindSubmatch(buf)
	if match == nil {
		return subexpMap, errors.Errorf("No match for regex '%v' for input", regEx)
	}

	// convert the matches into string for convenience
	matchStrings := make([]string, len(match))
	for i, m := range match {
		matchStrings[i] = string(m)
	}

	subexpNames := reg.SubexpNames()
	subexpMap, err = regexSubexpMapHelper(matchStrings, subexpNames)
	if err != nil {
		return subexpMap, err
	}

	return subexpMap, nil
}

// sleep function variable for mocking purposes
var sleepFunc = time.Sleep

// LogAndIgnoreErr logs the error info and ignores the
// error
func LogAndIgnoreErr(err error) {
	if err != nil {
		log.Printf("%v", err)
	}
}

// c := append(a, b...) is unsafe, if a has enough capacity, b will be directly
// appended.
// SafeAppendBytes makes guarantees that a is not changed.
func SafeAppendBytes(a, b []byte) []byte {
	la := len(a)
	c := make([]byte, la, la+len(b))
	copy(c, a)
	c = append(c, b...)
	return c
}

// String version of SafeAppendBytes
func SafeAppendString(a, b []string) []string {
	la := len(a)
	c := make([]string, la, la+len(b))
	copy(c, a)
	c = append(c, b...)
	return c
}

// get a 4 byte uint32 from a byte slice 'data' from offset to offset+4.
// return an error if offset will cause an out of range read
func GetWord(data []byte, offset uint32) (uint32, error) {
	if offset+4 > uint32(len(data)) {
		return 0, errors.Errorf("Required offset %v out of range of data size %v",
			offset, len(data))
	}
	val := binary.BigEndian.Uint32(data[offset : offset+4])
	return val, nil
}

// set a 4 byte uint32 in a byte slice 'data' from offset to offset+4.
// return an error if offset will cause an out of range write.
// Warning: make an explicit copy if the original array is required again after
// SetWord.
func SetWord(data []byte, word, offset uint32) ([]byte, error) {
	if offset+4 > uint32(len(data)) {
		return nil, errors.Errorf("Required offset %v out of range of data size %v",
			offset, len(data))
	}
	for i := 0; i < 4; i++ {
		b := byte(word >> ((3 - i) * 8))
		data[offset+uint32(i)] = b
	}
	return data, nil
}

// given 'data' which contains JSON data, get all the string keys.
// the sequence of the keys is not guaranteed.
func GetStringKeysFromJSONData(data []byte) ([]string, error) {
	var m map[string]interface{}

	err := json.Unmarshal(data, &m)
	if err != nil {
		return nil, errors.Errorf("Unable to unmarshal JSON: %v", err)
	}

	checksums := GetStringKeys(m)
	return checksums, nil
}
