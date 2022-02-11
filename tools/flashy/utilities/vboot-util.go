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

package utilities

import (
	"encoding/json"
	"fmt"
	"strings"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

func init() {
	step.RegisterUtility(vbootUtil)
}

func vbootUtilUsage() {
	fmt.Printf(
		`vboot-util
----------
Gets system vboot information in JSON format.

Usage: vboot-util
`)
}

func prettyPrint(i interface{}) string {
	s, _ := json.MarshalIndent(i, "", "\t")
	return string(s)
}

func vbootUtil(args []string) error {
	if len(args) != 1 || len(args) > 1 && (strings.Contains(args[1], "help") || args[1] == "-h") {
		vbootUtilUsage()
		return nil
	}

	vbs, err := utils.GetVbs()
	if err != nil {
		return err
	}

	fmt.Printf("%v\n", prettyPrint(vbs))
	return nil
}
