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
	"crypto/sha256"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
	"github.com/u-root/u-root/pkg/dt"
)

func init() {
	registerPartitionFactory(FIT, fitPartitionFactory)
}

var fitPartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &FitPartition{
		Name:          args.PInfo.Name,
		Data:          args.Data,
		Offset:        args.PInfo.Offset,
		FitImageNodes: args.PInfo.FitImageNodes,
	}
}

// FitPartition represents a partition using the FIT format.
// Validation works by parsing a device tree / flattened image tree header.
// Both the beginning and end of the data region are determined from fields in
// the header. Used for "kernel" and "rootfs" partitions.
type FitPartition struct {
	Name string
	Data []byte
	// offset in the image file
	Offset uint32
	// number of children nodes within the 'images' node
	FitImageNodes uint32
}

func (p *FitPartition) GetName() string {
	return p.Name
}

func (p *FitPartition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *FitPartition) Validate() error {
	r := bytes.NewReader(p.Data)
	// ReadFDT performs basic validations, including checking for magic
	// and overlaps
	fdt, err := dt.ReadFDT(r)
	if err != nil {
		return errors.Errorf("Unable to read FIT: %v", err)
	}

	// we recognize two nodes, both of which must exist:
	// 'images' and 'configurations'
	// N.B. some images (e.g. tiogapass1) have three nodes, the third being 'keys';
	// this is ignored.
	imagesNode, err := getNodeFromChildren(fdt.RootNode.Children, "images")
	if err != nil {
		return errors.Errorf("Unable to get 'images' node: %v", err)
	}
	err = p.validateImagesNode(imagesNode)
	if err != nil {
		return errors.Errorf("'images' FIT node failed validation: %v", err)
	}

	// existence check is sufficient, no validation is done to this node
	_, err = getNodeFromChildren(fdt.RootNode.Children, "configurations")
	if err != nil {
		return errors.Errorf("Unable to get 'configurations' node: %v", err)
	}

	log.Printf("FIT partition '%v' passed validation", p.GetName())
	return nil
}

func (p *FitPartition) GetType() PartitionConfigType {
	return FIT
}

// validate images node
func (p *FitPartition) validateImagesNode(imagesNode *dt.Node) error {
	numValidImageNodes := uint32(0)
	for _, imageNode := range imagesNode.Children {
		err := p.validateImageNode(imageNode)
		if err != nil {
			return errors.Errorf("Image node '%v' failed validation: %v",
				imageNode.Name, err)
		}
		log.Printf("FIT image node '%v' passed validation", imageNode.Name)
		numValidImageNodes++
	}
	// check that sufficient image nodes are valid
	if numValidImageNodes < p.FitImageNodes {
		return errors.Errorf("Not enough image nodes validated: want '%v' got '%v'",
			p.FitImageNodes, numValidImageNodes)
	}
	return nil
}

// for an image node, compute the sha256 digest and compare
// it against the one stored in the FDT node
func (p *FitPartition) validateImageNode(imageNode *dt.Node) error {
	data, err := p.getDataFromImageNode(imageNode)
	if err != nil {
		return err
	}

	checksum, err := p.getSHA256ChecksumFromImageNode(imageNode)
	if err != nil {
		return errors.Errorf("Error getting sha256 checksum: %v", err)
	}

	calcChecksumDat := sha256.Sum256(data)
	calcChecksum := calcChecksumDat[:]
	if !bytes.Equal(checksum, calcChecksum) {
		return errors.Errorf("Calculated sha256 (0x%X) does not match that in FIT (0x%X)",
			calcChecksum, checksum)
	}
	return nil
}

// get data from the image via 2 known methods.
// (1) getDataFromImageNodeViaDataProp (e.g. wedge100)
//     Use the 'data' prop which in its value stores the data required.
// (2) getDataFromImageNodeViaDataLink (e.g. fbtp)
//     Use the 'data-position' and 'data-size' prop to determine which part
//     in p.Data the data is.
func (p *FitPartition) getDataFromImageNode(imageNode *dt.Node) ([]byte, error) {
	var data []byte
	var err error
	data, err = p.getDataFromImageNodeViaDataProp(imageNode)
	if err == nil {
		// no error
		return data, nil
	}

	log.Printf("Unable to get data using 'data' property': %v. "+
		"Attempting to use links to image in properties.", err)

	data, err = p.getDataFromImageNodeViaDataLink(imageNode)
	if err != nil {
		return nil, errors.Errorf("Unable to get data: %v. "+
			"Attempted using (1) 'data' property and (2) data links.", err)
	}
	return data, nil
}

// use the 'data' property in the image node to get the data
func (p *FitPartition) getDataFromImageNodeViaDataProp(imageNode *dt.Node) ([]byte, error) {
	dataProp, err := getPropertyFromNode(imageNode, "data")
	if err != nil {
		return nil, err
	}
	data := dataProp.Value

	dataSize := uint32(len(data))
	if dataSize > p.GetSize() {
		return nil, errors.Errorf("Partition too small for data size defined " +
			"in image FDT")
	}

	return data, nil
}

// alternative to getDataFromImageNodeViaDataProp.
// used when 'data' property does not exist in the image node
// this uses a link to get the data (via props 'data-size' and 'data-position)
func (p *FitPartition) getDataFromImageNodeViaDataLink(imageNode *dt.Node) ([]byte, error) {
	dataSizeProp, err := getPropertyFromNode(imageNode, "data-size")
	if err != nil {
		return nil, err
	}
	dataSize, err := dataSizeProp.AsU32()
	if err != nil {
		return nil, err
	}

	dataPosProp, err := getPropertyFromNode(imageNode, "data-position")
	if err != nil {
		return nil, err
	}
	dataPos, err := dataPosProp.AsU32()
	if err != nil {
		return nil, err
	}

	endOffset, err := utils.AddU32(dataPos, dataSize)
	if err != nil {
		return nil, errors.Errorf("End offset overflowed: %v", err)
	}
	if endOffset > p.GetSize() {
		return nil, errors.Errorf("End offset required (%v) by 'data-size' (%v) and 'data-position' (%v) "+
			"too large for partition size (%v)", endOffset, dataSize, dataPos, p.GetSize())
	}

	return utils.BytesSliceRange(p.Data, dataPos, endOffset)
}

// given the image node, get the sha256 checksum.
// return error if the node is corrupt or the algo does not match sha256
func (p *FitPartition) getSHA256ChecksumFromImageNode(imageNode *dt.Node) ([]byte, error) {
	// get the sha256 digest stored in the image
	// info is stored in a hashNode
	// fb-openbmc uses 'hash@1', lf-openbmc uses 'hash-1'
	hashNode, err := getNodeFromChildren(imageNode.Children, "hash@1")
	if err != nil {
		hashNode, err = getNodeFromChildren(imageNode.Children, "hash-1")
		if err != nil {
			return nil, err
		}
	}
	// make sure the algo in the hashNode is sha256
	algoProp, err := getPropertyFromNode(hashNode, "algo")
	if err != nil {
		return nil, err
	}
	algo, err := algoProp.AsString()
	if err != nil {
		return nil, err
	} else if algo != "sha256" {
		return nil, errors.Errorf("Algo in image hash node is not sha256, got '%v'",
			algo)
	}

	checksumProp, err := getPropertyFromNode(hashNode, "value")
	if err != nil {
		return nil, err
	}
	checksum := checksumProp.Value
	if len(checksum) != sha256.Size {
		return nil, errors.Errorf("sha256 checksum size (%v) incorrect (should be %v)",
			len(checksum), sha256.Size)
	}
	return checksum, nil
}

// Helper function for dt.
// walk through all the properties and return the property matching the name,
// if not found, return an error
func getPropertyFromNode(n *dt.Node, name string) (dt.Property, error) {
	for _, p := range n.Properties {
		if p.Name == name {
			return p, nil
		}
	}
	return dt.Property{},
		errors.Errorf("Property with name '%v' not found in node", name)
}

// Helper function for dt.
// walk through all the children and return the node matching the name,
// if not found, return an error
func getNodeFromChildren(children []*dt.Node, name string) (*dt.Node, error) {
	for _, child := range children {
		if child.Name == name {
			return child, nil
		}
	}
	return nil, errors.Errorf("Child node with name '%v' not found in children",
		name)
}
