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

package common

import (
	"os"
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/utils"
	"github.com/pkg/errors"
)

func TestDropCaches(t *testing.T) {
	// mock utils.WriteFile to return nil if the write is correct
	writeFileOrig := utils.WriteFile
	defer func() {
		utils.WriteFile = writeFileOrig
	}()
	wantFilename := "/proc/sys/vm/drop_caches"
	wantDataString := "3"
	utils.WriteFile = func(filename string, data []byte, perm os.FileMode) error {
		if filename != wantFilename {
			return errors.Errorf("filename: want %v got %v", wantFilename, filename)
		}
		if string(data) != wantDataString {
			return errors.Errorf("data: want %v got %v", wantDataString, string(data))
		}
		return nil
	}

	t.Run("Test drop caches", func(t *testing.T) {
		got := dropCaches("x", "x")
		if got != nil {
			t.Errorf("want nil got %v", got)
		}
	})
}
