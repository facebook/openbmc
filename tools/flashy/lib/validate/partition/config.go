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

// PartitionConfigType is the type for the PartitionConfigType enum.
type PartitionConfigType = string

const (
	IGNORE       PartitionConfigType = "IGNORE"
	LEGACY_UBOOT                     = "LEGACY_UBOOT"
	UBOOT                            = "UBOOT"
	FIT                              = "FIT"
	FBMETA_MD5                       = "FBMETA_MD5"
	FBMETA_IMAGE                     = "FBMETA_IMAGE"
)

// ImageFormat represents a layout format for images.
type ImageFormat struct {
	Name             string
	PartitionConfigs []PartitionConfigInfo
}

// PartitionConfigInfo contains info for each partition in PartitionConfigs.
type PartitionConfigInfo struct {
	Name   string
	Offset uint32
	Size   uint32
	Type   PartitionConfigType
	// applicable only for FIT partitions
	// this is the minimum number of children nodes of the 'images' node
	FitImageNodes uint32
	// applicable only for partitions defined in image-meta
	Checksum string
}

// MetaPartitionImageFormat covers both vboot- and fit-meta image formats.
// This image format is exported to allow platform-specific image validations for
// retrofitting purposes (therefore disallowing upgrades to now-unsupported image formats).
var MetaPartitionImageFormat = ImageFormat{
	Name: "meta",
	PartitionConfigs: []PartitionConfigInfo{
		{
			// pass in the whole image and let MetaImagePartition deal with
			// finding the partitionConfigs and checksums
			Name:   "fbmeta-image",
			Offset: 0,
			Size:   32 * 1024 * 1024,
			Type:   FBMETA_IMAGE,
		},
	},
}

// ImageFormats is taken from fw-util (common/recipes-core/fw-util/files/image_parts.json).
// It maps from image format name to a array of partition configurations.
// The idea is that the image is validated against every single format type
// until one succeeds; if all fails, then the image is not valid.
// Differences:
// (1) flashy can support uboot validation, hence it is not ignored
//     (only in certain cases)
// (2) size and offset are in bytes
// (3) FitImageNodes (named num-nodes in fw-util) are explicitly 1 if not
//     specified in fw-util.
// (4) vboot spl+recovery can be treated as UBOOT. It will be ignored if the flash1
//     header is RO (e.g. fbtp)
var ImageFormats = []ImageFormat{
	MetaPartitionImageFormat,
	{
		Name: "vboot",
		PartitionConfigs: []PartitionConfigInfo{
			{
				Name:   "spl+recovery",
				Offset: 0,
				Size:   384 * 1024,
				Type:   UBOOT,
			},
			{
				Name:   "env",
				Offset: 384 * 1024,
				Size:   128 * 1024,
				Type:   IGNORE,
			},
			{
				Name:          "uboot",
				Offset:        512 * 1024,
				Size:          384 * 1024,
				Type:          FIT,
				FitImageNodes: 1,
			},
			{
				Name:          "unified-fit",
				Offset:        896 * 1024,
				Size:          27776 * 1024,
				Type:          FIT,
				FitImageNodes: 2,
			},
		},
	},
	{
		Name: "fit",
		PartitionConfigs: []PartitionConfigInfo{
			{
				Name:   "u-boot",
				Offset: 0,
				Size:   384 * 1024,
				Type:   UBOOT,
			},
			{
				Name:   "env",
				Offset: 384 * 1024,
				Size:   128 * 1024,
				Type:   IGNORE,
			},
			{
				Name:          "unified-fit",
				Offset:        512 * 1024,
				Size:          27776 * 1024,
				Type:          FIT,
				FitImageNodes: 2,
			},
		},
	},
	{
		Name: "legacy",
		PartitionConfigs: []PartitionConfigInfo{
			{
				Name:   "u-boot",
				Offset: 0,
				Size:   384 * 1024,
				Type:   UBOOT,
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
				Size:   4096 * 1024,
				Type:   LEGACY_UBOOT,
			},
			{
				Name:   "rootfs",
				Offset: 4608 * 1024,
				Size:   24064 * 1024,
				Type:   LEGACY_UBOOT,
			},
		},
	},
	{
		Name: "legacy-fido",
		PartitionConfigs: []PartitionConfigInfo{
			{
				Name:   "u-boot",
				Offset: 0,
				Size:   384 * 1024,
				Type:   UBOOT,
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
	},
}
