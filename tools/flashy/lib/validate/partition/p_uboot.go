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
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	registerPartitionFactory(UBOOT, ubootPartitionFactory)
}

var ubootPartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &UBootPartition{
		Name:      args.PInfo.Name,
		Data:      args.Data,
		Offset:    args.PInfo.Offset,
		checksums: ubootKnownChecksums,
	}
}

const ubootMagicSize = 4

var ubootMagics = []uint32{
	0x130000EA, 0xB80000EA, 0xBE0000EA, 0xF0000EA,
}

const ubootChecksumsPartitionSize = 1024 * 4

// known U-Boot checksums to validate U-Boot partitions that do not have
// appended checksums.
// This method is deprecated in Jun 2019, and all images built after have
// appended checksums. This list is used to support validating older
// flash devices.
var ubootKnownChecksums = []string{
	"4379ef3b08ae05c16a4de050b957b3c1",
	"26a4790a534b9fa84af5391ae8fa2b77",
	"6607bb28b23cab38f74bdf74667ed742",
	"70a40d90662d1e3d2d76fec1257fa944",
	"8a5e6f6ebf88e19f8b2670bc9a0a9cb6",
	"1fc8847fb4603bbc194d2b4a5fd4f1fc",
	"350b45e04babe4877fc92c995f048ca2",
	"aaabd327a9b7e85b1677606588321a00",
	"d96d2785aa8d9ff76ec2a4bbb2ac6c99",
	"8a41f4f2afda666c0da68fe85204a2fa",
	"2420e3eae3b3ad6d7e37c7646a3fffef",
	"2573bd1ad9537c4946e3c37e2dfb965a",
	"3c7b3c03c5ee2924a003b82b45233514",
	"064168447b860c7ce1bfcfd4fbbfdc40",
	"d4f98c28967837266c66d168c92bf10d",
	"69cbb5d67be491abe694b39944e14593",
	"094053297574eb1c0e067a420cc8ad89",
	"096640fe93a4797d61d1731a59f6eb41",
	"dec6d86016023b4b1a165ab71a7d3700",
	"0239145068bd3eae2455e29e62e82490",
	"25ed736a1a713fc335b37683ed4f79c1",
	"08f5d435a5b8944a76cb000609f9aa75",
	"de6663591b0873a69016874be4ffba80",
	"a700e250280c2a53c5199f4c3d4e4e29",
	"9d0d690db353c5e34abbf557078b20f0",
	"15280c29d888ec5a6c75b81386b39de7",
	"e0d27bab1cd990f9b9fde7e8f7e5f977",
	"e816edddc9ca673db5343fc551f7cc34",
	"53f32ead965c566d77c57b7eb65df790",
	"801b495e4ec3c3080c13c08d293ab400",
	"2be6219085cf5624ba53ae513e667ad3",
	"730270b9324ebf384e4b6d8a7a5d1f88",
	"22c8545cf9848578618da16a6ebddf29",
	"39a014b1618f6247de285f6e844ecdf1",
	"cb8bd8d2c3a4ca16b737f0af6bcf403c",
	"b05b6ebbf58a2f3cfcff3b4cfe81a440",
	"9352f0fa025a37b7fe87c40336a05730",
	"cb90fd29731c223b02cc6e3913859226",
	"9ca73e24223f853c0bad20004f0fdf3f",
	"a3a8f9d90d0c769e41e34379ed810ef1",
	"e1fe6a966562fab1da88ba7cf4e7db75",
	"58b18e55feef6c4a5ae1d0edf6086a1c",
	"936ee0ac311e9f7b8794f19d220d4d28",
	"0262ae17afe82dc65474a09d5934e9b7",
	"3029f15e214d9382c50e0889e55ba72d",
	"ab7428d76398c3ccacc0481d72a48474",
	"646763914069b3c0baf600af8da868e0",
	"165010b512c51079ea56eec0704de2a9",
	"a940170d999db44dd096e71d3e225346",
	"8f858c5c43695a8e68eeb0112b9ace75",
	"4ffb53046219ccc107fd1a7b4918c05a",
	"13406bb6a50550e86c82f603c8650e97",
	"3042eba38c58e9404c6f2d8c2ef0ae8e",
	"385ae3a590b40c8129d05b2d1364676e",
	"ec8726a0357004a7f809ffff4f263394",
	"5cf3280f245dd6e7866ba95630baaf53",
	"51f97d436259673c2b77c21919a9ea78",
	"a4e5a294cf559c5a7e02f3200f161e0e",
	"c8542af8dbc1f51253cc94e4b4c16d09",
	"db1671487bd3c46b23a8cf02d72d4f41",
	"bd73df9b7d8bef4dd8e52487437f52b1",
	"3aaa4ea8e3dc1b9784bfb453f5a890fd",
	"9fcce96a8074e42b71213f8c2ed2d1e0",
	"e926e3cb3a907b8a45f03c532f617605",
	"57dfae46af27a0f27daabca859720be1",
	"80bb1d8f5a2dd500daf399e8a9ae1bd7",
	"4544594bc3f6287f5bfb01861a974cc6",
	"913f98972f60f0a45549aa9b7da0c5c3",
	"7af0761eeb620a6c0ba723259ef82cb1",
	"bd6cd40b991c8a5203ac3c92408bff0a",
	"b8f650a43799d40f750804e6ed491a74",
	"32c6193841ca20dfbd50736e16beed0b",
	"58630c07b65f4704523e0d49743ef10d",
	"c6d6c828e27d50c91f79609d0d4abdf3",
	"96223c502298a0bc51a62dc0b291b6df",
	"7c342e175cc1dcb2bedc61d686c13c2c",
	"d9dadc633cbeee4edac58ede7c754240",
	"8b18cac028900da86113c56668118601",
	"0316015c35058d0b93c29956b46d9960",
	"15ba86517603d040b4d5ac7fc3de716a",
	"75c797545cece3c0fc01ee3c49c84e57",
	"7e5127d695db6b93237d84a34a069a94",
	"f7ef1af2820046d6eeea51bc1c6cc06d",
	"84c3d182b74f957c243a49c53f5955fd",
	"1cebd7c4f5614af6f5022f098a4c362c",
	"8a2cf2d75b7d89cd5a36c4875285fd3c",
	"3dcc5446be3ec19df16ccd5fb6bee249",
	"211f061c433152909a182bd2b013e3b2",
	"f84873123f24bae0790320b5df69d0d7",
	"57e9d18e6be493633e9dd66a58e03687",
	"d10479be5aff6d94cf05b2374dce89f9",
	"a16b8487fb60f45f76143c165622f734",
	"b8fb4b1d19712d20308c608409f706ff",
	"815a878f229fa7e615d84b1c9418df51",
	"693f2b0a92aeeca0e056ba970cc862c3",
	"a59dc7bdd0f8ee0a36160c9d4802ef44",
	"fc9adfb0d23e53300f0359f0575b393e",
	"356ef7f83a823b62b8dd9f44e269605a",
	"9df2b43b330047deabcdfda76092302c",
	"8571cb4cb46f3984879ae3e323e7e30a",
	"80b43b76c99019527a7636a66a19a663",
	"5e0bcf9a5ebc33976449d183edd54db1",
	"3bae25120694e6b6243bec7deeb5697c",
	"99d2788600c922a4059dbe728c6f55d0",
	"7cb96d97477e4e059ede5e0405255c55",
	"3cba8ee495407b1426acc12ac2e6457e",
	"006bd2a536bc371eb5ad4d79a8438d48",
	"e78ce9fc39ef6590b47ef5afc3f6685e",
	"336e446f4c4443cfdeeee84fe2a79806",
	"f8990ceb888a2d2288870b38038b338e",
	"51340fcbc4074c49df9749c643b272a5",
	"5d145e64c3b47dc0f87d0201d0facb63",
	"3c17743f6076f8fddedf004a6c6cedb4",
	"46a21d12da4721c4d8e7aa9718fdbc2b",
	"c51b9316ffb1cc12c16048c2b976f511",
	"15fb7cdae7d5a42ce0c5096bb6b18c51",
	"38b0c13833ba4237fabeacdcef3e59c5",
	"c237ba48299052b7f304d21cdd55a4ba",
	"a7d2e38a841fb955afde3901087a8dcf",
	"5c48467d1f528e9636d62df68affb633",
	"5a20c237c5a67f5f8d485da400fddf40",
	"d648d0b03bf2dbae6131ec5cb9ea1b68",
	"007dd5a6b6e025b765b91d4a924cf273",
	"3945625420f72ff14d168a6efa72f49b",
	"3d7e25b8ba741f79c71396bdc2b3a27b",
	"a0f44a77a939fb62afb7be408a0447a7",
	"fed1cf3767aceff08a1296e96e27cdda",
	"2e0204e726bf574bc8202c6440e53449",
	"8171fbbee9f5dcee6016a4c307a1c429",
	"f85a6b3ef8dc11b59db2f409ce840355",
	"f155504032a3d33601d04a83159fe366",
	"bc78d12b54d58a2b26044c6d330f1fdc",
	"97f348780a45d8559b7a6a2ec3e9725d",
	"60827e8615b8f031433663947d5fce62",
	"b80dcdf778b9281c245ca8e0b6850c1c",
	"b47112b23015ed1d859c0602b4adef04",
	"a5bc8f2d98cdb5ef362e0f6a0ffdf8f5",
	"bf174ed0feb922fd355a7e8cd06af661",
	"4364cadda5758f605e8ba3c29cfcb384",
	"e6d5be677a64af0ba69e517ceee45b6c",
	"782b3c8c5ce7d54e458de2f25a6bce65",
	"bd43d2ac309d9c9f26f952a2e95ea67c",
	"401001c0949c3dd87833c7e7b0547d70",
	"da47af59aeeea3123deec243792c4679",
	"bf90861b7b451eab95a89c2174506aa3",
	"0a6bd972453c799c535d132b7027faee",
	"b02e0f3ce73c7404596771a639b0838c",
	"d574846573993f60c004e7d69bf8c81e",
	"4e23e03de68780ec3c5aff460f7c643f",
	"d4e8abec17088c6d3e59f45108464780",
	"abd1754f6d5bb9fe518b9d23fac89fbc",
	"bbd44cf1c38edcd4b5acd8a3db44a999",
	"27fba94139f3c85834b9f2190c7f3855",
	"6aefa7848b51a31e904ddd3e637f05d9",
	"64000dd8f2c839f6cfdd5d28dac1909a",
	"5e1f3a0fe86a6cc27bc7a36bf1eb5e4d",
	"d3acb2af5cd47a00f838bd36ceef9ebf",
	"aff5afc1c77b73fe762e6ed2d7657d23",
	"1fa2dc2a35e8b5d6709703784a4c3bcd",
	"ff6633e68272cb99f116206d95fb043f",
	"eed0fcc3e814133d8365bc3210bc3b40",
	"b9735ec8f73e4eb0ecd5736161c1fcf9",
	"394109ff7ca241567b0d3456ee38fee3",
	"03c21c4c066864afe5298bebeab1f9fc",
	"5c0e995f68d773ca7bcf2bc8d29b72e0",
	"4d2181f1e1de31753d862f2af666f7b1",
}

// UBootPartition represents U-Boot partitions.
// Validation works by calculating an md5sum for the whole partition and considers it valid if the
// computed checksum matches any of the appended checksums/known checksums.
// The checksum is stored as json (appended, 4kB).
type UBootPartition struct {
	Name string
	Data []byte
	// offset in the image file
	Offset               uint32
	checksums            []string
	areChecksumsAppended bool
}

func (p *UBootPartition) GetName() string {
	return p.Name
}

func (p *UBootPartition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *UBootPartition) Validate() error {
	// check vboot enforcement. There is no point validating RO parts of
	// vboot U-Boot (which in actuality is spl+recovery U-Boot)
	vbootEnforcement := utils.GetVbootEnforcement()

	if vbootEnforcement == utils.VBOOT_HARDWARE_ENFORCE {
		log.Printf("U-Boot partition '%v' validation bypassed: this is a VBoot system",
			p.Name)
		return nil
	}

	// check magic
	err := p.checkMagic()
	if err != nil {
		return err
	}

	p.tryGetAppendedChecksums()

	// calculate the actual checksum without the checksum region
	calcChecksum, err := p.getChecksum()
	if err != nil {
		return errors.Errorf("Failed to calculate checksum: %v", err)
	}

	if utils.StringFind(calcChecksum, p.checksums) == -1 {
		errMsg := fmt.Sprintf("'%v' partition md5sum '%v' unrecognized",
			p.Name, calcChecksum)
		log.Printf("%v", errMsg)
		return errors.Errorf(errMsg)
	}
	log.Printf("'%v' partition md5sum '%v' OK",
		p.Name, calcChecksum)

	log.Printf("U-Boot partition '%v' passed validation", p.Name)
	return nil
}

func (p *UBootPartition) GetType() PartitionConfigType {
	return UBOOT
}

// check magic
func (p *UBootPartition) checkMagic() error {
	// check that p.Data is larger than 4 bytes
	if len(p.Data) < ubootMagicSize {
		return errors.Errorf("'%v' partition too small (%v) to contain U-Boot magic",
			p.Name, len(p.Data))
	}

	magic := binary.BigEndian.Uint32(p.Data[:ubootMagicSize])
	if utils.Uint32Find(magic, ubootMagics) == -1 {
		return errors.Errorf("Magic '0x%X' does not match any U-Boot magic", magic)
	}
	return nil
}

// try to get the appended checksums
func (p *UBootPartition) tryGetAppendedChecksums() {
	// helper function to indicate that checksums are not appended
	var logNotAppended = func(err error) {
		log.Printf("'%v' partition has no appended checksums: %v\n"+
			"Using known checksums", p.GetName(), err)
	}

	// check that image is large enough to contain appended checksums
	if len(p.Data) < ubootChecksumsPartitionSize {
		logNotAppended(errors.Errorf("Too small (%v) to contain "+
			"appended checksums", len(p.Data)))
		return
	}
	checksumsDatRegionPadded, err := utils.BytesSliceRange(p.Data, uint32(len(p.Data)-ubootChecksumsPartitionSize), uint32(len(p.Data)))
	if err != nil {
		logNotAppended(errors.Errorf("Unable to get checksums region: %v", err))
		return
	}

	// trim null chars, as unused space is padded by them
	checksumsDatRegion := bytes.Trim(checksumsDatRegionPadded,
		"\x00")

	appendedChecksums, err := utils.GetStringKeysFromJSONData(checksumsDatRegion)
	if err != nil {
		logNotAppended(err)
		return
	}

	log.Printf("'%v' partition has appended checksum(s) %v",
		p.Name, appendedChecksums)

	// append to back of checksums
	p.areChecksumsAppended = true
	p.checksums = append(p.checksums, appendedChecksums...)
}

// calculate md5sum of region excluding the appended checksums region
func (p *UBootPartition) getChecksum() (string, error) {
	var checksum string
	// if checksums are appended, then don't hash the checksum region
	if p.areChecksumsAppended {
		dataWithoutChecksumRegion, err := utils.BytesSliceRange(p.Data, 0, uint32(len(p.Data)-ubootChecksumsPartitionSize))
		if err != nil {
			return "", errors.Errorf("Unable to calculate checksums: %v", err)
		}
		hash := md5.Sum(dataWithoutChecksumRegion)
		checksum = hex.EncodeToString(hash[:])
	} else {
		hash := md5.Sum(p.Data)
		checksum = hex.EncodeToString(hash[:])
	}
	return checksum, nil
}
