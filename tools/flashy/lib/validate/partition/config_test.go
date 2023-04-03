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

package partition

import (
	"testing"
)

// Checks to make sure ImageFormat is properly entered
// (1) Make sure two adjacent partitions don't overlap
// (2) Make sure Offset and Size are multiples of 1024 (prevent accidental kB entry)
//     As of now we don't have any sizes/offsets requiring a higher precision.
// (3) Make sure FIT partitions specify FitImageNodes (fw-util assumes 1 if not specified)
// (4) Make sure final Size + Offset <= 128 * 1024 * 1024 (128MB)
func TestImageFormats(t *testing.T) {
	for _, format := range ImageFormats {
		lastEndOffset := uint32(0)
		for _, config := range format.PartitionConfigs {
			if config.Offset%1024 != 0 {
				t.Errorf("'%v' format '%v' partition offset is not a multiple of 1024, "+
					"did you forget to enter it in bytes?",
					format.Name, config.Name)
			}
			if config.Size%1024 != 0 {
				t.Errorf("'%v' format '%v' partition size is not a multiple of 1024, "+
					"did you forget to enter it in bytes?",
					format.Name, config.Name)
			}
			if config.Offset < lastEndOffset {
				t.Errorf("'%v' format '%v' partition overlapped with previous partition region.",
					format.Name, config.Name)
			}
			if config.Type == FIT {
				if config.FitImageNodes == 0 {
					t.Errorf("'%v' format '%v' partition is FIT but FitImageNodes is not specified",
						format.Name, config.Name)
				}
			}

			lastEndOffset = config.Size + config.Offset
		}
		if lastEndOffset > 128*1024*1024 {
			t.Errorf("Image format %v' more than 128MB!", format)
		}
	}
}
