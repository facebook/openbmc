package partition

import (
	"bytes"
	"encoding/json"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

type LFMetaLocations struct {
	Offset uint32
	Size   uint32
}

// Layouts are from:
//
//	https://github.com/openbmc/openbmc/blob/2d43f028cd98e622d53365b83c39cd120cac5cc2/meta-phosphor/classes/image_types_phosphor.bbclass#L58
var lfmetaImageMetaLocations = []LFMetaLocations{
	{380 * 1024, 4 * 1024}, // default 32 MB layout
	{888 * 1024, 8 * 1024}, // default 64 MB and 128 MB layout
}

func init() {
	registerPartitionFactory(LFMETA_IMAGE, lfmetaImagePartitionFactory)
}

var lfmetaImagePartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &LFMetaImagePartition{
		Name:   args.PInfo.Name,
		Data:   args.Data,
		Offset: args.PInfo.Offset,
	}
}

type LFMetaManifestInfo struct {
	Purpose         string   `json:"purpose"`
	Machine         string   `json:"machine"`
	Version         string   `json:"version"`
	BuildId         string   `json:"build-id"`
	ExtendedVersion string   `json:"extended-version"`
	CompatibleNames []string `json:"compatible-names"`
}

type LFMetaPartType = string

const (
	LF_PART_DATA      LFMetaPartType = "data"
	LF_PART_FIT                      = "fit"
	LF_PART_JSON                     = "json"
	LF_PART_UBOOT                    = "u-boot"
	LF_PART_TEST_ONLY                = "don't-use--testing-only"
)

type LFMetaManifestPart struct {
	Name      string         `json:"name"`
	Type      LFMetaPartType `json:"type"`
	Offset    uint32         `json:"offset"`
	Size      uint32         `json:"size"`
	NumNodes  uint           `json:"num-nodes,omitempty"`
	Sha256Sum string         `json:"sha256,omitempty"`
}

type LFMetaManifest struct {
	Type      string               `json:"type"`
	Version   int                  `json:"version"`
	Info      LFMetaManifestInfo   `json:"info"`
	Parts     []LFMetaManifestPart `json:"partitions"`
	Sha256Sum string               `json:"manifest-sha256"`
}

type LFMetaImagePartition struct {
	Name   string
	Data   []byte
	Offset uint32
}

func (p *LFMetaImagePartition) GetName() string {
	return p.Name
}

func (p *LFMetaImagePartition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *LFMetaImagePartition) GetType() PartitionConfigType {
	return LFMETA_IMAGE
}

func (p *LFMetaImagePartition) Validate() error {
	var err error

	for _, location := range lfmetaImageMetaLocations {
		l_err := p.CheckLocation(location)
		if l_err == nil {
			return nil
		}

		if err == nil {
			err = l_err
		} else {
			log.Printf("Ignoring secondary error: %s", l_err.Error())
		}
	}

	return err
}

func (p *LFMetaImagePartition) CheckLocation(location LFMetaLocations) error {
	log.Printf("Checking location: offset=%d B, size=%d B", location.Offset, location.Size)

	offsetEnd := location.Offset + location.Size
	if offsetEnd > p.GetSize() {
		return errors.Errorf("Image/device size too small (%v B) to contain meta partition region",
			p.GetSize())
	}

	// Extract the metadata partition into an LFMetaManifest.
	metaInfo, err := p.ExtractMetadata(location.Offset, offsetEnd)
	if err != nil {
		return err
	}

	if len(metaInfo.Parts) == 0 {
		return errors.Errorf("Empty partition table found in metadata.")
	}

	// Transform the partitions into a PartitionConfigInfo for further
	// validation.
	var partConfigs []PartitionConfigInfo
	for _, part := range metaInfo.Parts {

		partConfig, err := p.ConvertToPartitionConfig(part)
		if err != nil {
			return err
		}

		partConfigs = append(partConfigs, partConfig)
	}

	// Check the partitions for validity.
	err = ValidatePartitionsFromPartitionConfigs(p.Data, partConfigs)
	if err != nil {
		return err
	}

	return nil
}

func (p *LFMetaImagePartition) ExtractMetadata(start uint32, end uint32) (LFMetaManifest, error) {
	var metaInfo LFMetaManifest

	metaPartitionData, err := utils.BytesSliceRange(p.Data, start, end)
	if err != nil {
		return metaInfo, err
	}

	// Look for 'phosphor-image-manifest' magic before attempting to parse JSON.
	if !bytes.Contains(metaPartitionData, []byte("phosphor-image-manifest")) {
		return metaInfo, errors.New("Did not find 'phosphor-image-manifest' keyword.")
	}

	// Strip off everything after the first NUL character.
	metaPartitionData, _, _ = bytes.Cut(metaPartitionData, []byte("\000"))
	// Find the "manifest-sha256" JSON tag and pick everything before it as the
	// part which is checksum protected.
	checksumProtectedBlob, _, _ := bytes.Cut(metaPartitionData, []byte("manifest-sha256"))

	// Attempt to parse JSON.
	err = json.Unmarshal(metaPartitionData, &metaInfo)
	if err != nil {
		return metaInfo, err
	}

	// Verify JSON checksum.
	manifestPart := []PartitionConfigInfo{{
		Name:     "manifest",
		Offset:   start,
		Size:     uint32(len(checksumProtectedBlob)),
		Type:     LFMETA_SHA256,
		Checksum: metaInfo.Sha256Sum,
	}}
	err = ValidatePartitionsFromPartitionConfigs(p.Data, manifestPart)
	if err != nil {
		return metaInfo, err
	}

	return metaInfo, nil
}

func (p *LFMetaImagePartition) ConvertToPartitionConfig(part LFMetaManifestPart) (PartitionConfigInfo, error) {
	var partConfig PartitionConfigInfo

	partType, err := p.ConvertPartType(part)
	if err != nil {
		return partConfig, err
	}

	partConfig = PartitionConfigInfo{
		Name:     part.Name,
		Offset:   part.Offset,
		Size:     part.Size,
		Type:     partType,
		Checksum: part.Sha256Sum,
	}

	return partConfig, nil
}

func (p *LFMetaImagePartition) ConvertPartType(part LFMetaManifestPart) (PartitionConfigType, error) {
	switch part.Type {
	case LF_PART_DATA:
		return IGNORE, nil
	case LF_PART_FIT:
		return LFMETA_SHA256, nil
		// TODO: FIT magic doesn't seem to match on LF code?
		// return FIT, nil
	case LF_PART_UBOOT:
		return LFMETA_SHA256, nil
	case LF_PART_JSON:
		return IGNORE, nil
	default:
		return IGNORE, errors.Errorf("Unknown partition type: '%v'", part.Type)
	}
}
