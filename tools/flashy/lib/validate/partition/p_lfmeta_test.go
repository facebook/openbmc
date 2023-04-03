package partition

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"log"
	"reflect"
	"testing"

	//"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestLFMetaImagePartition(t *testing.T) {
	exampleData := []byte("abcd")
	args := PartitionFactoryArgs{
		Data: exampleData,
		PInfo: PartitionConfigInfo{
			Name:   "foo",
			Offset: 1024,
			Size:   4,
			Type:   LFMETA_IMAGE,
		},
	}
	wantP := &LFMetaImagePartition{
		Name:   "foo",
		Offset: 1024,
		Data:   exampleData,
	}
	p := lfmetaImagePartitionFactory(args)
	if !reflect.DeepEqual(wantP, p) {
		t.Errorf("partition: want '%v' got '%v'", wantP, p)
	}
	gotName := p.GetName()
	if "foo" != gotName {
		t.Errorf("name: want '%v' got '%v'", "foo", gotName)
	}
	gotSize := p.GetSize()
	if uint32(4) != gotSize {
		t.Errorf("size: want '%v' got '%v'", 4, gotSize)
	}
	gotValidatorType := p.GetType()
	if gotValidatorType != LFMETA_IMAGE {
		t.Errorf("validator type: want '%v' got '%v'", LFMETA_IMAGE, gotValidatorType)
	}
}

var TestLFMakeManifest = func(init func(*LFMetaManifest), checksum bool) []byte {
	var partitionData [32 * 1024 * 1024]byte
	defaultJson := LFMetaManifest{
		Type:    "phosphor-image-manifest",
		Version: 1,
		Info: LFMetaManifestInfo{
			Purpose:         "unknown",
			Machine:         "test",
			Version:         "1.2.3",
			BuildId:         "v1.2.3",
			ExtendedVersion: "v1.2.3-p0",
			CompatibleNames: []string{},
		},
		Parts:     []LFMetaManifestPart{},
		Sha256Sum: "0",
	}

	if init != nil {
		init(&defaultJson)
	}

	blob, _ := json.MarshalIndent(defaultJson, "", "    ")
	if checksum {
		hash := sha256.Sum256(
			blob[:bytes.Index(blob, []byte("manifest-sha256"))],
		)
		defaultJson.Sha256Sum = hex.EncodeToString(hash[:])
		blob, _ = json.MarshalIndent(defaultJson, "", "    ")
	}

	log.Printf("JSON: %v", string(blob))
	copy(partitionData[380*1024:], blob)

	return partitionData[:]
}

func TestLFMetaImageValidate(t *testing.T) {
	cases := []struct {
		name      string
		partition LFMetaImagePartition
		want      error
	}{
		{
			name: "Empty partition",
			partition: LFMetaImagePartition{
				Name:   "foo",
				Data:   []byte(""),
				Offset: 0,
			},
			want: errors.Errorf("Image/device size too small (0 B) to contain meta partition region"),
		},
		{
			name: "Missing phosphor-image-manifest",
			partition: LFMetaImagePartition{
				Name: "foo",
				Data: TestLFMakeManifest(func(m *LFMetaManifest) {
					m.Type = "missing-the-manifest"
				}, false),
				Offset: 0,
			},
			want: errors.Errorf("Did not find 'phosphor-image-manifest' keyword."),
		},
		{
			name: "Manifest with bad checksum",
			partition: LFMetaImagePartition{
				Name: "foo",
				Data: TestLFMakeManifest(func(m *LFMetaManifest) {
					m.Sha256Sum = "abcdefg"
				},
					false),
				Offset: 0,
			},
			want: errors.Errorf("Validation failed: Partition 'manifest' failed validation: 'manifest' partition SHA256 (382ceed4fa5fccc968adc2bc62823e7539c1b484ad9af1d434abc7dba80a90c5) does not match expected (abcdefg)"),
		},
		{
			name: "Manifest with good checksum but no partitions",
			partition: LFMetaImagePartition{
				Name:   "foo",
				Data:   TestLFMakeManifest(nil, true),
				Offset: 0,
			},
			want: errors.Errorf("Empty partition table found in metadata."),
		},
		{
			name: "Good manifest with just the manifest partition",
			partition: LFMetaImagePartition{
				Name: "foo",
				Data: TestLFMakeManifest(func(m *LFMetaManifest) {
					m.Parts = append(m.Parts, LFMetaManifestPart{
						Name:   "manifest",
						Type:   LF_PART_JSON,
						Offset: 380 * 1024,
						Size:   4 * 1024,
					})
				}, true),
				Offset: 0,
			},
			want: nil,
		},
		{
			name: "Bad u-boot checksum",
			partition: LFMetaImagePartition{
				Name: "foo",
				Data: TestLFMakeManifest(func(m *LFMetaManifest) {
					m.Parts = append(m.Parts, LFMetaManifestPart{
						Name:      "spl",
						Type:      LF_PART_UBOOT,
						Offset:    0,
						Size:      16,
						Sha256Sum: "1234",
					})
				}, true),
				Offset: 0,
			},
			want: errors.Errorf("Validation failed: Partition 'spl' failed validation: 'spl' partition SHA256 (374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb) does not match expected (1234)"),
		},
		{
			name: "Good u-boot checksum",
			partition: LFMetaImagePartition{
				Name: "foo",
				Data: TestLFMakeManifest(func(m *LFMetaManifest) {
					m.Parts = append(m.Parts, LFMetaManifestPart{
						Name:      "spl",
						Type:      LF_PART_UBOOT,
						Offset:    0,
						Size:      16,
						Sha256Sum: "374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb",
					})
				}, true),
				Offset: 0,
			},
			want: nil,
		},
		{
			name: "Bad partition type",
			partition: LFMetaImagePartition{
				Name: "foo",
				Data: TestLFMakeManifest(func(m *LFMetaManifest) {
					m.Parts = append(m.Parts, LFMetaManifestPart{
						Name:   "test",
						Type:   LF_PART_TEST_ONLY,
						Offset: 0,
						Size:   16,
					})
				}, true),
				Offset: 0,
			},
			want: errors.Errorf("Unknown partition type: 'don't-use--testing-only'"),
		},
		{
			name: "Invalid JSON",
			partition: LFMetaImagePartition{
				Name: "foo",
				Data: func() []byte {
					blob := TestLFMakeManifest(nil, true)
					copy(blob[380*1024:], []byte("{ phosphor-image-manifest"))
					return blob
				}(),
			},
			want: errors.Errorf("invalid character 'p' looking for beginning of object key string"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got := tc.partition.Validate()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestLFMetaConvertPartType(t *testing.T) {
	cases := []struct {
		name    string
		intype  LFMetaPartType
		outtype PartitionConfigType
	}{
		{
			name:    "data",
			intype:  LF_PART_DATA,
			outtype: IGNORE,
		},
		{
			name:    "fit",
			intype:  LF_PART_FIT,
			outtype: LFMETA_SHA256,
		},
		{
			name:    "u-boot",
			intype:  LF_PART_UBOOT,
			outtype: LFMETA_SHA256,
		},
		{
			name:    "json",
			intype:  LF_PART_JSON,
			outtype: IGNORE,
		},
		{
			name:    "bad-type",
			intype: LF_PART_TEST_ONLY,
			outtype: IGNORE,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var p LFMetaImagePartition
			got, _ := p.ConvertPartType(LFMetaManifestPart{Type: tc.intype})
			if got != tc.outtype {
				t.Errorf("want '%v' got '%v'", tc.outtype, got)
			}
		})
	}
}
