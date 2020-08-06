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

type PartitionConfigType = string

const (
	IGNORE       PartitionConfigType = "IGNORE"
	LEGACY_UBOOT                     = "LEGACY_UBOOT"
	UBOOT                            = "UBOOT"
	FIT                              = "FIT"
)

// info for each partition in PartitionConfigs
type PartitionConfigInfo struct {
	Name     string
	Offset   uint32
	Size     uint32
	Type     PartitionConfigType
	FitNodes uint32 // applicable only for FIT partitions
}

// taken from fw-util (common/recipes-core/fw-util/files/image_parts.json).
// maps from image format name to a array of partition configurations.
// the idea is that the image is validated against every single format type
// until one succeeds; if all fails, then the image is not valid.
// Differences:
// (1) flashy can support uboot validation, hence it is not ignored
//     (only in certain cases)
// (2) size and offset are in bytes
var ImageFormats = map[string]([]PartitionConfigInfo){
	// "fit": {
	// 	{
	// 		Name:   "u-boot",
	// 		Offset: 0,
	// 		Size:   384 * 1024,
	// 		Type:   UBOOT,
	// 	},
	// 	{
	// 		Name:   "env",
	// 		Offset: 384 * 1024,
	// 		Size:   128 * 1024,
	// 		Type:   IGNORE,
	// 	},
	// 	{
	// 		Name:     "unified-fit",
	// 		Offset:   512 * 1024,
	// 		Size:     27776 * 1024,
	// 		Type:     FIT,
	// 		FitNodes: 2,
	// 	},
	// },
	"legacy-fido": {
		{
			Name:   "u-boot",
			Offset: 0,
			Size:   384 * 1024,
			Type:   IGNORE, // not in known checksums and no appended checksums
		},
		{
			Name:   "env",
			Offset: 384 * 1024,
			Size:   128 * 1024,
			Type:   IGNORE,
		},
		{
			Name:   "kernel",
			Offset: 512 * 1024,
			Size:   2560 * 1024,
			Type:   LEGACY_UBOOT,
		},
		{
			Name:   "rootfs",
			Offset: 3072 * 1024,
			Size:   12288 * 1024,
			Type:   LEGACY_UBOOT,
		},
	},
}
