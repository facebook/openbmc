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
	"bytes"
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	registerPartitionFactory(FBMETA_IMAGE, fbmetaImagePartitionFactory)
}

var fbmetaImagePartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &FBMetaImagePartition{
		Name:   args.PInfo.Name,
		Data:   args.Data,
		Offset: args.PInfo.Offset,
	}
}

// supported FBOBMC_IMAGE_META_VER
var fbmetaSupportedVersions = []int{1}

const fbmetaImageMetaPartitionSize = 64 * 1024
const fbmetaImageMetaPartitionOffset = 0x000F0000

// FBMetaInfo contains relevant info in the image-meta partition.
type FBMetaInfo struct {
	FBOBMC_IMAGE_META_VER int              `json:"FBOBMC_IMAGE_META_VER"`
	PartInfos             []FBMetaPartInfo `json:"part_infos"`
}

// FBMetaPartInfoType represents the "Type" defined in MetaPartInfo
type FBMetaPartInfoType = string

const (
	// FBMETA_ROM : the u-boot SPL used in verified boot systems
	FBMETA_ROM FBMetaPartInfoType = "rom"
	// FBMETA_RAW : raw binary, normally U-Boot
	FBMETA_RAW = "raw"
	// FBMETA_FIT : U-Boot defined fit
	FBMETA_FIT = "fit"
	// FBMETA_META : the image meta partition
	FBMETA_META = "meta"
	// FBMETA_DATA : data partition
	FBMETA_DATA = "data"
)

// FBMetaPartInfo is analogous to PartitionConfigInfo, but a shim is required
// to convert it properly to use the correct "type".
type FBMetaPartInfo struct {
	Name   string             `json:"name"`
	Size   uint32             `json:"size"`
	Offset uint32             `json:"offset"`
	Type   FBMetaPartInfoType `json:"type"`
	// For now, the only checksum used is MD5
	Checksum string `json:"md5"`
	// applicable only for FIT partitions
	// this is the minimum number of children nodes of the 'images' node
	FitImageNodes uint32 `json:"num-nodes"`
}

// FBMetaChecksum is a separate JSON that contains the md5 checksum
// of the image-meta partition.
type FBMetaChecksum struct {
	Checksum string `json:"meta_md5"`
}

// FBMetaImagePartition is a full image that contains the image-meta partition.
// The image-meta partition contains the information
// of the partitions and indicates the validation scheme
// required.
type FBMetaImagePartition struct {
	Name     string
	Data     []byte
	Offset   uint32
	metaInfo FBMetaInfo
}

func (p *FBMetaImagePartition) GetName() string {
	return p.Name
}

func (p *FBMetaImagePartition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *FBMetaImagePartition) Validate() error {
	var err error

	p.metaInfo, err = p.getMetaInfo()
	if err != nil {
		return err
	}
	log.Printf("Validating image following image-meta: \n%+v", p.metaInfo)

	partitionConfigs, err := getPartitionConfigsFromFBMetaPartInfos(p.metaInfo.PartInfos)
	if err != nil {
		return err
	}

	err = ValidatePartitionsFromPartitionConfigs(p.Data, partitionConfigs)
	if err != nil {
		return err
	}

	log.Printf("Image-meta partition '%v' passed validation.", p.Name)
	return nil
}

func (p *FBMetaImagePartition) GetType() PartitionConfigType {
	return FBMETA_IMAGE
}

// getMetaInfo gets and validates the image-meta information
// from the meta partition
func (p *FBMetaImagePartition) getMetaInfo() (FBMetaInfo, error) {
	var metaInfo FBMetaInfo
	offsetEnd := fbmetaImageMetaPartitionSize + fbmetaImageMetaPartitionOffset
	if uint32(offsetEnd) > p.GetSize() {
		return metaInfo, errors.Errorf("Image/device size too small (%v B) to contain meta partition region",
			p.GetSize())
	}

	metaPartitionData := p.Data[fbmetaImageMetaPartitionOffset:offsetEnd]

	return parseAndValidateFBImageMetaJSON(metaPartitionData)
}

// parseAndValidateFBImageMetaJSON parses and validates MetaInfo given
// the bytes containing the meta-partition region
var parseAndValidateFBImageMetaJSON = func(data []byte) (FBMetaInfo, error) {
	var metaInfo FBMetaInfo
	var metaChecksum FBMetaChecksum
	// the image-meta partition contains two lines
	// ending with '\n' (0x0A).
	// the first line contains the image-meta JSON (FBMetaInfo)
	// the second line contains the checksum for image-meta (FBMetaChecksum)
	splitData := bytes.Split(data, []byte{'\n'})
	// must have at least 3 elements
	if len(splitData) < 3 {
		return metaInfo, errors.Errorf("Meta partition incomplete: cannot find two lines of " +
			"JSON")
	}

	imageMetaJSONData := splitData[0]
	imageMetaChecksumJSONData := splitData[1]

	// get the checksums first
	err := json.Unmarshal(imageMetaChecksumJSONData, &metaChecksum)
	if err != nil {
		return metaInfo, errors.Errorf("Unable to unmarshal image-meta checksum JSON: %v",
			err)
	}
	checksum := metaChecksum.Checksum

	// calculate md5sum of imageMetaJSONData
	hash := md5.Sum(imageMetaJSONData)
	calcChecksum := hex.EncodeToString(hash[:])

	if calcChecksum != checksum {
		return metaInfo, errors.Errorf("'image-meta' checksum (%v) does not match checksums supplied (%v)",
			calcChecksum, checksum)
	}

	// get metaInfo
	err = json.Unmarshal(imageMetaJSONData, &metaInfo)
	if err != nil {
		return metaInfo, errors.Errorf("Unable to unmarshal image-meta JSON: %v",
			err)
	}

	return metaInfo, nil
}

var getPartitionConfigsFromFBMetaPartInfos = func(metaPartInfos []FBMetaPartInfo) ([]PartitionConfigInfo, error) {
	// get vboot enforcement, required for getPartitionConfigFromFBMetaPartInfo
	vbootEnforcement := utils.GetVbootEnforcement()

	partitionConfigs := make([]PartitionConfigInfo, 0, len(metaPartInfos))
	for _, metaPartInfo := range metaPartInfos {
		partitionConfig, err := getPartitionConfigFromFBMetaPartInfo(
			metaPartInfo,
			vbootEnforcement,
		)
		if err != nil {
			return nil, err
		}
		partitionConfigs = append(partitionConfigs, partitionConfig)
	}
	return partitionConfigs, nil
}

// getPartitionConfigFromFBMetaPartInfo is a shim function to convert MetaPartInfo
// to PartitionConfigInfo
var getPartitionConfigFromFBMetaPartInfo = func(m FBMetaPartInfo, vbootEnforcement utils.VbootEnforcementType) (PartitionConfigInfo, error) {
	var partitionConfig PartitionConfigInfo
	partitionConfigType, err := getPartitionConfigTypeFromFBMetaPartInfoType(m.Type, vbootEnforcement)
	if err != nil {
		return partitionConfig, err
	}
	partitionConfig = PartitionConfigInfo{
		Name:     m.Name,
		Offset:   m.Offset,
		Size:     m.Size,
		Type:     partitionConfigType,
		Checksum: m.Checksum,
	}
	return partitionConfig, nil
}

var getPartitionConfigTypeFromFBMetaPartInfoType = func(t FBMetaPartInfoType, vbootEnforcement utils.VbootEnforcementType) (FBMetaPartInfoType, error) {
	switch t {
	case FBMETA_ROM:
		if vbootEnforcement == utils.VBOOT_HARDWARE_ENFORCE {
			// ignore this, as this ROM part will be Read-Only
			return IGNORE, nil
		}
		return FBMETA_MD5, nil
	case FBMETA_RAW:
		return FBMETA_MD5, nil
	case FBMETA_FIT:
		return FIT, nil
	case FBMETA_META,
		FBMETA_DATA:
		return IGNORE, nil
	default:
		return IGNORE, errors.Errorf("Unknown partition type in image-meta: '%v'", t)
	}
}
