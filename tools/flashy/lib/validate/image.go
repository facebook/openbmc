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

package validate

import (
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// get OpenBMC version from /etc/issue
// examples: fbtp-v2020.09.1, wedge100-v2020.07.1
// WARNING: There is no guarantee that /etc/issue is well-formed
// in old images
var getOpenBMCVersionFromIssueFile = func() (string, error) {
	versionRegEx := `^OpenBMC Release (?P<version>[^\s]+)`
	version := ""

	etcIssueBuf, err := utils.ReadFile("/etc/issue")
	if err != nil {
		return version, errors.Errorf("Error reading /etc/issue: %v", err)
	}

	etcIssueMap, err := utils.GetRegexSubexpMap(
		versionRegEx, string(etcIssueBuf))

	if err != nil {
		// does not match regex
		return version,
			errors.Errorf("Unable to get version from /etc/issue: %v", err)
	}

	version = etcIssueMap["version"]

	return version, nil
}
