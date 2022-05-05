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

package utils

import (
	"bytes"
	"encoding/binary"
	"log"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/pkg/errors"
)

// AST_SRAM_VBS_BASE is the location in SRAM used for verified boot content/flags.
const AST_SRAM_VBS_BASE = 0x1E720200 // ast2400, ast2500
const AST_SRAM_VBS_BASE_G6 = 0x10015800 // ast2600

// AST_SRAM_VBS_SIZE is the size of verified boot content/flags.
const AST_SRAM_VBS_SIZE = 56

// VbootEnforcementType is an enum representing the vboot type of the system
type VbootEnforcementType = string

const (
	VBOOT_NONE             VbootEnforcementType = "NONE"
	VBOOT_SOFTWARE_ENFORCE                      = "SOFTWARE_ENFORCE"
	VBOOT_HARDWARE_ENFORCE                      = "HARDWARE_ENFORCE"
)

// Vbs represents the verified content flags. This is taken from
// common/recipes-utils/vboot-utils/files/vbs.
type Vbs struct {
	/* 00 */ Uboot_exec_address uint32 /* Location in MMIO where U-Boot/Recovery U-Boot is execution */
	/* 04 */ Rom_exec_address uint32 /* Location in MMIO where ROM is executing from */
	/* 08 */ Rom_keys uint32 /* Location in MMIO where the ROM FDT is located */
	/* 0C */ Subordinate_keys uint32 /* Location in MMIO where subordinate FDT is located */
	/* 10 */ Rom_handoff uint32 /* Marker set when ROM is handing execution to U-Boot. */
	/* 14 */ Force_recovery uint8 /* Set by ROM when recovery is requested */
	/* 15 */ Hardware_enforce uint8 /* Set by ROM when WP pin of SPI0.0 is active low */
	/* 16 */ Software_enforce uint8 /* Set by ROM when RW environment uses verify=yes */
	/* 17 */ Recovery_boot uint8 /* Set by ROM when recovery is used */
	/* 18 */ Recovery_retries uint8 /* Number of attempts to recovery from verification failure */
	/* 19 */ Error_type uint8 /* Type of error, or 0 for success */
	/* 1A */ Error_code uint8 /* Unique error code, or 0 for success */
	/* 1B */ Error_tpm uint8 /* The last-most-recent error from the TPM. */
	/* 1C */ Crc uint16 /* A CRC of the vbs structure */
	/* 1E */ Error_tpm2 uint16 /* The last-most-recent error from the TPM2. */
	/* 20 */ Subordinate_last uint32 /* Status reporting only: the last booted subordinate. */
	/* 24 */ Uboot_last uint32 /* Status reporting only: the last booted U-Boot. */
	/* 28 */ Kernel_last uint32 /* Status reporting only: the last booted kernel. */
	/* 2C */ Subordinate_current uint32 /* Status reporting only: the current booted subordinate. */
	/* 30 */ Uboot_current uint32 /* Status reporting only: the current booted U-Boot. */
	/* 34 */ Kernel_current uint32 /* Status reporting only: the current booted kernel. */
}

func (v *Vbs) validate() error {
	// make a copy of v
	vCopy := *v

	// reference crc
	crc16 := vCopy.Crc

	// set CRC to 0
	vCopy.Crc = 0
	// not set when SPL computes CRC
	vCopy.Uboot_exec_address = 0
	// SPL clears this before computing CRC
	vCopy.Rom_handoff = 0

	dat, err := vCopy.encodeVbs()
	if err != nil {
		return errors.Errorf("Error validating vboot content: %v", err)
	}

	calcCrc16 := CRC16(dat)
	if crc16 != calcCrc16 {
		return errors.Errorf("CRC16 of vboot data (%v) does not match reference (%v)",
			calcCrc16, crc16)
	}

	return nil
}

func (v *Vbs) encodeVbs() ([]byte, error) {
	var b bytes.Buffer
	err := binary.Write(&b, binary.LittleEndian, v)
	if err != nil {
		return nil, err
	}
	return b.Bytes(), nil
}

// decodeVbs takes in the section of bytes containing vbs contents
// and returns the Vbs struct after validating it.
func decodeVbs(vbsData []byte) (Vbs, error) {
	var vbs Vbs
	err := binary.Read(bytes.NewBuffer(vbsData[:]), binary.LittleEndian, &vbs)
	if err != nil {
		return vbs, errors.Errorf("Unable to decode vbs data into struct: %v", err)
	}
	err = vbs.validate()
	if err != nil {
		return vbs, err
	}
	return vbs, nil
}

var vbootPartitionExists = func() bool {
	// check whether the "rom" partition exists
	_, err := GetMTDInfoFromSpecifier("rom")
	return err == nil
}

var getMachine = func() (string, error) {
	var uts syscall.Utsname

	err := syscall.Uname(&uts)
	if err != nil {
		return "unknown", err
	}

	machine := make([]byte, 0, len(uts.Machine))
	for _, v := range uts.Machine {
		if v == 0 {
			break
		}
		machine = append(machine, byte(v))
	}
	return string(machine), nil
}

// GetVbs tries to get the Vbs struct by reading off /dev/mem.
// This errors out if the vboot partition ("rom") does not exist.
var GetVbs = func() (Vbs, error) {
	var vbs Vbs
	var baseaddr uint32

	if !vbootPartitionExists() {
		return vbs, errors.Errorf("Not a Vboot system: vboot partition (rom) does not exist.")
	}

	machine, err := getMachine()
	if err != nil {
		return vbs, errors.Errorf("Unable to fetch utsinfo: %v", err)
	}

	if machine == "armv7l" {
		baseaddr = AST_SRAM_VBS_BASE_G6
	} else {
		baseaddr = AST_SRAM_VBS_BASE
	}
	log.Printf("Machine type from utsinfo: %v, base address: 0x%x", machine, baseaddr)

	mmapOffset := fileutils.GetPageOffsettedBase(baseaddr)
	length := fileutils.Pagesize // surely larger than AST_SRAM_VBS_SIZE

	pageData, err := fileutils.MmapFileRange("/dev/mem", int64(mmapOffset),
		length, syscall.PROT_READ, syscall.MAP_SHARED)
	if err != nil {
		return vbs, errors.Errorf("Unable to mmap /dev/mem: %v", err)
	}
	defer fileutils.Munmap(pageData)

	dataOffset := fileutils.GetPageOffsettedOffset(baseaddr)
	vbsEndOffset, err := AddU32(dataOffset, AST_SRAM_VBS_SIZE)
	if err != nil {
		return vbs, errors.Errorf("VBS end offset overflowed: %v", err)
	}
	vbsData, err := BytesSliceRange(pageData, dataOffset, vbsEndOffset)
	if err != nil {
		return vbs, errors.Errorf("Failed to get vbs data: %v", err)
	}

	return decodeVbs(vbsData)
}

// GetVbootEnforcement gets the vboot enforcement type of the system.
var GetVbootEnforcement = func() VbootEnforcementType {
	vbs, err := GetVbs()
	if err != nil {
		log.Printf("Assuming this is not a vboot system because: %v", err)
		return VBOOT_NONE
	}

	if vbs.Hardware_enforce == 1 {
		return VBOOT_HARDWARE_ENFORCE
	} else if vbs.Software_enforce == 1 {
		return VBOOT_SOFTWARE_ENFORCE
	}
	return VBOOT_NONE
}
