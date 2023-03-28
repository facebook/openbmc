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
	"crypto/rand"
	"crypto/sha256"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
	"github.com/u-root/u-root/pkg/dt"
)

// the other header values are set accordingly in fdt.Write
var exampleFDTHeader = dt.Header{
	Magic:   0xd00dfeed,
	Version: 17,
}
var exampleRootProperties = []dt.Property{
	{Name: "foo"},
}

// convenience function to convert fdt to []byte for testing
func fdtToBytes(fdt *dt.FDT) []byte {
	var b bytes.Buffer
	_, err := fdt.Write(&b)
	if err != nil {
		panic(err)
	}
	return b.Bytes()
}

// convenience function to insert image nodes into a valid fdt for testing
func insertImageNodesIntoValidFDT(n []*dt.Node) *dt.FDT {
	return &dt.FDT{
		Header: exampleFDTHeader,
		RootNode: &dt.Node{
			Children: []*dt.Node{
				{
					Name:     "images",
					Children: n, // inserted here
				},
				{Name: "configurations"},
			},
			Properties: exampleRootProperties,
		},
	}
}

func TestFitPartition(t *testing.T) {
	// make 4kB random bytes
	randData := make([]byte, 4*1024)
	rand.Read(randData)

	args := PartitionFactoryArgs{
		Data: randData,
		PInfo: PartitionConfigInfo{
			Name:          "foo",
			Offset:        1024,
			Size:          4096,
			Type:          FIT,
			FitImageNodes: 2,
		},
	}
	wantP := &FitPartition{
		Name:          "foo",
		Data:          randData,
		Offset:        1024,
		FitImageNodes: 2,
	}
	p := fitPartitionFactory(args)
	if !reflect.DeepEqual(wantP, p) {
		t.Errorf("partition: want '%v' got '%v'", wantP, p)
	}
	gotName := p.GetName()
	if "foo" != gotName {
		t.Errorf("name: want '%v' got '%v'", "foo", gotName)
	}
	gotSize := p.GetSize()
	if uint32(4096) != gotSize {
		t.Errorf("size: want '%v' got '%v'", 4096, gotSize)
	}
	gotValidatorType := p.GetType()
	if gotValidatorType != FIT {
		t.Errorf("validator type: want '%v' got '%v'", FIT, gotValidatorType)
	}
}

// generic end-to-end tests for Validate
func TestFitValidate(t *testing.T) {
	// make 4kB random bytes
	randData := make([]byte, 4*1024)
	rand.Read(randData)

	calcRandChecksum := sha256.Sum256(randData)

	exampleValidImageNode := &dt.Node{
		Name: "imageNode1",
		Properties: []dt.Property{
			{
				Name:  "data",
				Value: randData,
			},
		},
		Children: []*dt.Node{
			{
				Name: "hash@1",
				Properties: []dt.Property{
					{
						Name:  "algo",
						Value: []byte("sha256\x00"),
					},
					{
						Name:  "value",
						Value: calcRandChecksum[:],
					},
				},
			},
		},
	}

	cases := []struct {
		name string
		data []byte
		want error
	}{
		{
			// intricacies on FDT parsing is left to the fdt pkg,
			// will not be tested here
			name: "FDT parse error (empty bytes)",
			data: []byte{},
			want: errors.Errorf("Unable to read FIT: EOF"),
		},
		{
			name: "no images node",
			data: fdtToBytes(&dt.FDT{
				Header: exampleFDTHeader,
				RootNode: &dt.Node{
					Children: []*dt.Node{
						{Name: "UNKNOWN_NAME"},
						{Name: "configurations"},
					},
					Properties: exampleRootProperties,
				},
			}),
			want: errors.Errorf("Unable to get 'images' node: " +
				"Child node with name 'images' not found in children"),
		},
		{
			name: "no configurations node",
			data: fdtToBytes(&dt.FDT{
				Header: exampleFDTHeader,
				RootNode: &dt.Node{
					Children: []*dt.Node{
						{Name: "images"},
						{Name: "UNKNOWN_NAME"},
					},
					Properties: exampleRootProperties,
				},
			}),
			want: errors.Errorf("Unable to get 'configurations' node: " +
				"Child node with name 'configurations' not found in children"),
		},
		{
			name: "valid children node names, no images to validate",
			data: fdtToBytes(&dt.FDT{
				Header: exampleFDTHeader,
				RootNode: &dt.Node{
					Children: []*dt.Node{
						{Name: "images"},
						{Name: "configurations"},
					},
					Properties: exampleRootProperties,
				},
			}),
			want: nil,
		},
		{
			name: "has data, passed validation",
			data: fdtToBytes(insertImageNodesIntoValidFDT([]*dt.Node{
				exampleValidImageNode,
			})),
			want: nil,
		},
		{
			name: "images failed validation: can't get image data, no 'data' & 'data-size'",
			data: fdtToBytes(insertImageNodesIntoValidFDT([]*dt.Node{
				{
					Name: "imageNode1",
					// no 'data' and 'data-size' property
				},
			})),
			want: errors.Errorf("'images' FIT node failed validation: " +
				"Image node 'imageNode1' failed validation: " +
				"Unable to get data: " +
				"Property with name 'data-size' not found in node. " +
				"Attempted using (1) 'data' property and (2) data links."),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			p := &FitPartition{
				FitImageNodes: 0,
				Data:          tc.data,
			}

			got := p.Validate()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestValidateImagesNode(t *testing.T) {
	// make 4kB random bytes
	randData := make([]byte, 4*1024)
	rand.Read(randData)

	calcRandChecksum := sha256.Sum256(randData)

	exampleValidImageNode := &dt.Node{
		Name: "validImageNode",
		Properties: []dt.Property{
			{
				Name:  "data",
				Value: randData,
			},
		},
		Children: []*dt.Node{
			{
				Name: "hash@1",
				Properties: []dt.Property{
					{
						Name:  "algo",
						Value: []byte("sha256\x00"),
					},
					{
						Name:  "value",
						Value: calcRandChecksum[:],
					},
				},
			},
		},
	}

	// no data
	exampleInvalidImageNode := &dt.Node{
		Name: "invalidImageNode",
	}

	cases := []struct {
		name          string
		fitImageNodes uint32
		imagesNode    *dt.Node
		want          error
	}{
		{
			name:          "two successful nodes",
			fitImageNodes: 2,
			imagesNode: &dt.Node{
				Name: "imagesNode",
				Children: []*dt.Node{
					exampleValidImageNode,
					exampleValidImageNode,
				},
			},
			want: nil,
		},
		{
			name:          "second node failed",
			fitImageNodes: 2,
			imagesNode: &dt.Node{
				Name: "imagesNode",
				Children: []*dt.Node{
					exampleValidImageNode,
					exampleInvalidImageNode,
				},
			},
			want: errors.Errorf("Image node 'invalidImageNode' failed validation: " +
				"Unable to get data: Property with name 'data-size' not found in node. " +
				"Attempted using (1) 'data' property and (2) data links."),
		},
		{
			name:          "not enough successful nodes",
			fitImageNodes: 2,
			imagesNode: &dt.Node{
				Name: "imagesNode",
				Children: []*dt.Node{
					exampleValidImageNode,
				},
			},
			want: errors.Errorf("Not enough image nodes validated: want '2' got '1'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			p := &FitPartition{
				Data:          randData,
				FitImageNodes: tc.fitImageNodes,
			}
			got := p.validateImagesNode(tc.imagesNode)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestValidateImageNode(t *testing.T) {
	// make 4kB random bytes
	randData := make([]byte, 4*1024)
	rand.Read(randData)

	calcRandChecksum := sha256.Sum256(randData)

	p := &FitPartition{
		Data: randData,
	}

	// has data property
	exampleValidImageNode := &dt.Node{
		Name: "imageNode1",
		Properties: []dt.Property{
			{
				Name:  "data",
				Value: randData,
			},
		},
		Children: []*dt.Node{
			{
				Name: "hash@1",
				Properties: []dt.Property{
					{
						Name:  "algo",
						Value: []byte("sha256\x00"),
					},
					{
						Name:  "value",
						Value: calcRandChecksum[:],
					},
				},
			},
		},
	}

	cases := []struct {
		name      string
		imageNode *dt.Node
		want      error
	}{
		{
			name:      "validation successful",
			imageNode: exampleValidImageNode,
			want:      nil,
		},
		{
			name: "no 'data' nor data link, failed",
			imageNode: &dt.Node{
				Name:       "imageNode1",
				Properties: []dt.Property{},
			},
			want: errors.Errorf("Unable to get data: Property with name 'data-size' " +
				"not found in node. Attempted using (1) 'data' property and (2) data links."),
		},
		{
			name: "error getting sha256 checksum",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data",
						Value: randData,
					},
				},
				Children: []*dt.Node{},
			},
			want: errors.Errorf("Error getting sha256 checksum: " +
				"Child node with name 'hash-1' not found in children"),
		},
		{
			name: "checksums do not match",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data",
						Value: randData,
					},
				},
				Children: []*dt.Node{
					{
						Name: "hash@1",
						Properties: []dt.Property{
							{
								Name:  "algo",
								Value: []byte("sha256\x00"),
							},
							{
								Name:  "value",
								Value: []byte("abcdabcdabcdabcdabcdabcdabcdabcd"),
							},
						},
					},
				},
			},
			want: errors.Errorf("Calculated sha256 "+
				"(0x%X) "+
				"does not match that in FIT "+
				"(0x6162636461626364616263646162636461626364616263646162636461626364)",
				calcRandChecksum[:]),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			err := p.validateImageNode(tc.imageNode)
			tests.CompareTestErrors(tc.want, err, t)
		})
	}
}

func TestGetDataFromImageNode(t *testing.T) {
	// make 1kB random bytes
	randData := make([]byte, 1024)
	rand.Read(randData)

	p := &FitPartition{
		Data: randData,
	}

	// has data property
	exampleValidImageNodeDataProp := &dt.Node{
		Name: "imageNode1",
		Properties: []dt.Property{
			{
				Name:  "data",
				Value: randData,
			},
		},
	}

	// uses data-size and data-position
	// depends on a FitPartition initialized with partition data 'randData'
	exampleValidImageNodeDataLink := &dt.Node{
		Name: "imageNode1",
		Properties: []dt.Property{
			{
				Name:  "data-size",
				Value: []byte{0x00, 0x00, 0x04, 0x00},
			},
			{
				Name:  "data-position",
				Value: []byte{0x00, 0x00, 0x00, 0x00},
			},
		},
	}
	cases := []struct {
		name      string
		imageNode *dt.Node
		want      []byte
		wantErr   error
	}{
		{
			name:      "success: use 'data' prop",
			imageNode: exampleValidImageNodeDataProp,
			want:      randData,
			wantErr:   nil,
		},
		{
			name:      "success: use data link",
			imageNode: exampleValidImageNodeDataLink,
			want:      randData,
			wantErr:   nil,
		},
		{
			name: "failed, no data prop nor link",
			imageNode: &dt.Node{
				Name:       "imageNode1",
				Properties: []dt.Property{},
			},
			want: nil,
			wantErr: errors.Errorf("Unable to get data: " +
				"Property with name 'data-size' not found in node. " +
				"Attempted using (1) 'data' property and (2) data links."),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := p.getDataFromImageNode(tc.imageNode)
			if !bytes.Equal(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestGetDataFromImageNodeViaDataProp(t *testing.T) {
	// make 1kB rand bytes
	randData := make([]byte, 1024)
	rand.Read(randData)

	p := &FitPartition{
		Data: randData,
	}

	cases := []struct {
		name      string
		imageNode *dt.Node
		want      []byte
		wantErr   error
	}{
		{
			name: "successfully gotten data",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data",
						Value: randData,
					},
				},
			},
			want:    randData,
			wantErr: nil,
		},
		{
			name: "no 'data' property",
			imageNode: &dt.Node{
				Name:       "imageNode1",
				Properties: []dt.Property{},
			},
			want:    nil,
			wantErr: errors.Errorf("Property with name 'data' not found in node"),
		},
		{
			name: "data longer than partition data",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name: "data",
						// repeated
						Value: bytes.Repeat(randData, 2),
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("Partition too small for data size defined in image FDT"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := p.getDataFromImageNodeViaDataProp(tc.imageNode)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !bytes.Equal(tc.want, got) {
				t.Errorf("want '%v', got '%v'", tc.want, got)
			}
		})
	}
}

func TestGetDataFromImageNodeViaDataLink(t *testing.T) {
	// make 1kB rand bytes
	randData := make([]byte, 1024)
	rand.Read(randData)

	// initialize a fit partition with 1kb rand data
	p := &FitPartition{
		Data: randData,
	}

	cases := []struct {
		name      string
		imageNode *dt.Node
		want      []byte
		wantErr   error
	}{
		{
			name: "successfully gotten",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data-size",
						Value: []byte{0x00, 0x00, 0x00, 0x08},
					},
					{
						Name:  "data-position",
						Value: []byte{0x00, 0x00, 0x00, 0x04},
					},
				},
			},
			want:    randData[4 : 4+8],
			wantErr: nil,
		},
		{
			name: "no 'data-size' prop",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data-position",
						Value: []byte{0x00, 0x00, 0x00, 0x04},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("Property with name 'data-size' not found in node"),
		},
		{
			name: "no 'data-position' prop",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data-size",
						Value: []byte{0x00, 0x00, 0x00, 0x04},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("Property with name 'data-position' not found in node"),
		},
		{
			name: "data-size can't be casted to uint32 type",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data-size",
						Value: []byte{0x08},
					},
					{
						Name:  "data-position",
						Value: []byte{0x00, 0x00, 0x00, 0x04},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("property \"data-size\" is not <u32>"),
		},
		{
			name: "data-position can't be casted to uint32 type",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data-size",
						Value: []byte{0x00, 0x00, 0x00, 0x08},
					},
					{
						Name:  "data-position",
						Value: []byte{0x04},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("property \"data-position\" is not <u32>"),
		},
		{
			name: "end offset too large",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data-size",
						Value: []byte{0x04, 0x00, 0x00, 0x00},
					},
					{
						Name:  "data-position",
						Value: []byte{0x00, 0x00, 0x00, 0x04},
					},
				},
			},
			want: nil,
			wantErr: errors.Errorf("End offset required (67108868) by 'data-size' " +
				"(67108864) and 'data-position' (4) too large for partition size (1024)"),
		},
		{
			name: "end offset overflowed",
			imageNode: &dt.Node{
				Name: "imageNode1",
				Properties: []dt.Property{
					{
						Name:  "data-size",
						Value: []byte{0xFF, 0xFF, 0xFF, 0xFF},
					},
					{
						Name:  "data-position",
						Value: []byte{0x00, 0x00, 0x00, 0x04},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("End offset overflowed: Unsigned integer overflow for (4+4294967295)"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := p.getDataFromImageNodeViaDataLink(tc.imageNode)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !bytes.Equal(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

func TestFitGetSHA256ChecksumFromImageNode(t *testing.T) {
	cases := []struct {
		name      string
		imageNode *dt.Node
		want      []byte
		wantErr   error
	}{
		{
			name: "successfully gotten (hash@1)",
			imageNode: &dt.Node{
				Children: []*dt.Node{
					{
						Name: "hash@1",
						Properties: []dt.Property{
							{
								Name:  "algo",
								Value: []byte("sha256\x00"),
							},
							{
								Name:  "value",
								Value: []byte("abcdabcdabcdabcdabcdabcdabcdabcd"),
							},
						},
					},
				},
			},
			want:    []byte("abcdabcdabcdabcdabcdabcdabcdabcd"),
			wantErr: nil,
		},
		{
			name: "no hash@1 node",
			imageNode: &dt.Node{
				Children: []*dt.Node{},
			},
			want:    nil,
			wantErr: errors.Errorf("Child node with name 'hash-1' not found in children"),
		},
		{
			name: "successfully gotten (hash-1)",
			imageNode: &dt.Node{
				Children: []*dt.Node{
					{
						Name: "hash-1",
						Properties: []dt.Property{
							{
								Name:  "algo",
								Value: []byte("sha256\x00"),
							},
							{
								Name:  "value",
								Value: []byte("abcdabcdabcdabcdabcdabcdabcdabcd"),
							},
						},
					},
				},
			},
			want:    []byte("abcdabcdabcdabcdabcdabcdabcdabcd"),
			wantErr: nil,
		},
		{
			name: "no algo node in hash@1",
			imageNode: &dt.Node{
				Children: []*dt.Node{
					{
						Name: "hash@1",
						Properties: []dt.Property{
							{
								Name:  "value",
								Value: []byte("abcdabcdabcdabcdabcdabcdabcdabcd"),
							},
						},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("Property with name 'algo' not found in node"),
		},
		{
			name: "no value node",
			imageNode: &dt.Node{
				Children: []*dt.Node{
					{
						Name: "hash@1",
						Properties: []dt.Property{
							{
								Name:  "algo",
								Value: []byte("sha256\x00"),
							},
						},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("Property with name 'value' not found in node"),
		},
		{
			name: "wrong algo",
			imageNode: &dt.Node{
				Children: []*dt.Node{
					{
						Name: "hash@1",
						Properties: []dt.Property{
							{
								Name:  "algo",
								Value: []byte("crc32\x00"),
							},
						},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("Algo in image hash node is not sha256, got 'crc32'"),
		},
		{
			name: "wrong algo, can't get as string",
			imageNode: &dt.Node{
				Children: []*dt.Node{
					{
						Name: "hash@1",
						Properties: []dt.Property{
							{
								Name:  "algo",
								Value: []byte{42},
							},
						},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("property \"algo\" is not <string>"),
		},
		{
			name: "checksum size incorrect",
			imageNode: &dt.Node{
				Children: []*dt.Node{
					{
						Name: "hash@1",
						Properties: []dt.Property{
							{
								Name:  "algo",
								Value: []byte("sha256\x00"),
							},
							{
								Name:  "value",
								Value: []byte("tooshort!"),
							},
						},
					},
				},
			},
			want:    nil,
			wantErr: errors.Errorf("sha256 checksum size (9) incorrect (should be 32)"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			p := &FitPartition{}
			got, err := p.getSHA256ChecksumFromImageNode(tc.imageNode)
			if !bytes.Equal(tc.want, got) {
				t.Errorf("want '%x' got '%x'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestGetPropertyFromNode(t *testing.T) {
	node := &dt.Node{
		Properties: []dt.Property{
			{
				Name: "a",
			},
		},
	}

	_, err := getPropertyFromNode(node, "a")
	tests.CompareTestErrors(nil, err, t)

	_, err = getPropertyFromNode(node, "b")
	tests.CompareTestErrors(
		errors.Errorf("Property with name '%v' not found in node", "b"),
		err,
		t,
	)
}

func TestGetNodeFromChildren(t *testing.T) {
	children := []*dt.Node{
		{
			Name: "a",
		},
	}

	_, err := getNodeFromChildren(children, "a")
	tests.CompareTestErrors(nil, err, t)

	_, err = getNodeFromChildren(children, "b")
	tests.CompareTestErrors(
		errors.Errorf("Child node with name '%v' not found in children", "b"),
		err,
		t,
	)

}
