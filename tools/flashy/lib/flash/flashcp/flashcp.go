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

// Package flashcp contains a reimplementation of busybox flashcp.
// https://git.busybox.net/busybox/tree/miscutils/flashcp.c
// 1ca9d158da7e2fefc910ff41fa88f8c35afa99da
// A difference is that an RO roOffset argument is provided to skip parts in both
// files, intended for devices with RO blocks.
// N.B.: We use the block device (/dev/mtdblock[0-9]+) for RO operations
// throughout flashy. There is a mysterious edge case in which if you keep
// the non-block device (/dev/mtd[0-9]+) open, 0x00 blocks don't get sync-ed
// properly to erased (0xFF) blocks.
// This can be worked-around by making sure that no instances of the
// non-block device is open, i.e. they MUST be explicitly closed.
// This is why the non-block device is explicitly closed in the steps here,
// as verification is done via the block device.
// In addition, we are not using mmap to write to mtd, as this is not advisable.
// http://www.linux-mtd.infradead.org/faq/general.html
package flashcp

import (
	"bytes"
	"fmt"
	"log"
	"os"
	"regexp"
	"syscall"
	"unsafe"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
	"github.com/vtolstov/go-ioctl"
)

// flashDeviceFile is an interface for the used flash device file functions, this is implemented by
// os.File and is intended to make testing easier.
type flashDeviceFile interface {
	Fd() uintptr
	Close() error
	Name() string
}

// imageFile is a struct containing the imageFileName and the mmap-ed data
type imageFile struct {
	name string
	data []byte
}

// linux/include/uapi/mtd/mtd-abi.h
type erase_info_user struct {
	start  uint32
	length uint32
}

// linux/include/uapi/mtd/mtd-abi.h
type mtd_info_user struct {
	_type     uint8
	flags     uint32
	size      uint32 /* Total size of the MTD */
	erasesize uint32
	writesize uint32
	oobsize   uint32 /* Amount of OOB data per block (e.g. 16) */
	padding   uint64 /* Old obsolete field; do not use */
}

const sizeof_mtd_info_user = 32
const sizeof_erase_user_info = 8

// linux/include/uapi/mtd/mtd-abi.h
var MEMGETINFO = ioctl.IOR('M', 1, sizeof_mtd_info_user)
var MEMERASE = ioctl.IOW('M', 2, sizeof_erase_user_info)

var IOCTL = ioctl.IOCTL

// FlashCp copies an image file into a device file.
// roOffset is the beginning RO region in the MTD. roOffset bytes will be skipped
// in both the image and the flash device. (In `dd` terms, this roOffset would be supplied
// to both skip= and seek=).
// Note that erase will still erase complete blocks, so if the roOffset is within an
// erase block, the whole erase block is erased. Since the region is RO, there is no effect.
// roOffset example case:
// roOffset = 2
// image contents = IIII
// flash contents = OOOO
// result         = OOII
var FlashCp = func(imageFilePath, deviceFilePath string, roOffset uint32) error {
	// open flash device
	deviceFile, err := openFlashDeviceFile(deviceFilePath)
	if err != nil {
		return errors.Errorf("Unable to open flash device file '%v': %v",
			deviceFilePath, err)
	}
	// get mtd_info_user
	m, err := getMtdInfoUser(deviceFile.Fd())
	if err != nil {
		return errors.Errorf("Can't get mtd_info_user for '%v', "+
			"this may not be a MTD flash device: %v",
			deviceFilePath, err)
	}
	// close device file here explicitly
	err = closeFlashDeviceFile(deviceFile)
	if err != nil {
		return errors.Errorf("Unable to close flash device file '%v': %v",
			deviceFilePath, err)
	}

	// log for clarity
	if roOffset != 0 {
		log.Printf("Flashcp RO roOffset mode: skipping %vB RO region", roOffset)
	}

	// read image data
	imageData, err := fileutils.MmapFile(
		imageFilePath, syscall.PROT_READ, syscall.MAP_SHARED,
	)
	if err != nil {
		return errors.Errorf("Can't mmap image file '%v': %v",
			imageFilePath, err)
	}
	defer fileutils.Munmap(imageData)

	imFile := imageFile{
		name: imageFilePath,
		data: imageData,
	}

	return runFlashProcess(deviceFilePath, m, imFile, roOffset)
}

// openFlashDeviceFile is a wrapper around OpenFileWithLock intended to return
// an os.File which implements flashDeviceFile.
// remember to call closeFlashDeviceFile to close the flash device file.
var openFlashDeviceFile = func(deviceFilePath string) (flashDeviceFile, error) {
	return fileutils.OpenFileWithLock(deviceFilePath, os.O_SYNC|os.O_RDWR, syscall.LOCK_EX)
}

var closeFlashDeviceFile = func(f flashDeviceFile) error {
	return fileutils.CloseFileWithUnlock(f.(*os.File))
}

// runFlashProcess performs a simple health check then performs flashing in 3 steps:
// 1) erase, 2) copy, and 3) verify.
var runFlashProcess = func(
	deviceFilePath string,
	m mtd_info_user,
	imFile imageFile,
	roOffset uint32) error {

	deviceFile, err := openFlashDeviceFile(deviceFilePath)
	if err != nil {
		return errors.Errorf("Unable to open flash device file '%v': %v",
			deviceFilePath, err)
	}

	err = healthCheck(deviceFile, m, imFile, roOffset)
	if err != nil {
		return err
	}

	utils.PetWatchdog()
	err = eraseFlashDevice(deviceFile, m, imFile, roOffset)
	if err != nil {
		return err
	}

	utils.PetWatchdog()
	err = flashImage(deviceFile, m, imFile, roOffset)
	if err != nil {
		return err
	}

	// make sure non-block device is closed before using block device
	err = closeFlashDeviceFile(deviceFile)
	if err != nil {
		return errors.Errorf("Unable to close flash device file '%v': %v",
			deviceFilePath, err)
	}

	utils.PetWatchdog()
	err = verifyFlash(deviceFilePath, m, imFile, roOffset)
	if err != nil {
		return err
	}

	return nil
}

var getMtdInfoUser = func(fd uintptr) (mtd_info_user, error) {
	var m mtd_info_user

	err := IOCTL(fd, MEMGETINFO, uintptr(unsafe.Pointer(&m)))
	if err != nil {
		return m, errors.Errorf("Can't get mtd_info_user: %v", err)
	}
	log.Printf("Got mtd_info_user: %#v", m)

	return m, nil
}

// healthCheck makes sure that the device file path of the mtd matches /dev/mtd[0-9]+,
// and the imageData is smaller than the device size and roOffset.
var healthCheck = func(
	deviceFile flashDeviceFile,
	m mtd_info_user,
	imFile imageFile,
	roOffset uint32) error {
	const mtdFilePathRegEx = "^/dev/mtd[0-9]+$"
	regEx := regexp.MustCompile(mtdFilePathRegEx)
	matched := regEx.MatchString(deviceFile.Name())
	if !matched {
		return errors.Errorf("Device file path '%v' does not match required pattern '%v'",
			deviceFile.Name(), mtdFilePathRegEx)
	}

	if uint32(len(imFile.data)) > m.size {
		return errors.Errorf("Image size (%vB) larger than flash device size (%vB)",
			len(imFile.data), m.size)
	}

	if uint32(len(imFile.data)) < roOffset {
		return errors.Errorf("Image size (%vB) smaller than RO offset %v",
			len(imFile.data), roOffset)
	}

	return nil
}

// eraseFlashDevice erases the flash device up to the block larger than the
// image file size. If roOffset is non-zero and within an eraseblock, the whole
// block is erased. This is be a no-op for the hardware-enforced RO region,
// and only the RW part is actually erased. We don't erase starting from the middle
// of the block as this is bad MTD practice.
var eraseFlashDevice = func(
	deviceFile flashDeviceFile,
	m mtd_info_user,
	imFile imageFile,
	roOffset uint32,
) error {
	log.Printf("Erasing flash device '%v'...", deviceFile.Name())

	if m.erasesize == 0 {
		// make sure first m.erasesize != 0
		return errors.Errorf("invalid mtd device erasesize: 0")
	}

	// make sure we erase from a complete erasesize block
	eraseStart := uint32(roOffset/m.erasesize) * m.erasesize

	// make sure we erase up to a complete erasesize block
	imageSize := uint32(len(imFile.data))

	// check for overflow
	imageAndEraseSize, err := utils.AddU32(imageSize, m.erasesize)
	if err != nil {
		return errors.Errorf("Failed to get erase length: %v", err)
	}
	// length if erasesize is 0 (won't over/under-flow here due to m.erasesize > 0)
	eraseEnd := uint32((imageAndEraseSize-1)/m.erasesize) * m.erasesize

	// erase the flash one block at a time, like flash_eraseall does.
	// this used to wipe the entire device with a single ioctl, but
	// that could lock up the system in kernel mode for the duration.
	log.Printf("Erasing flash device: start: %v, length: %v (end: %v)",
		eraseStart, eraseEnd-eraseStart, eraseEnd)
	for off := uint32(eraseStart); off < eraseEnd; off += m.erasesize {
		e := erase_info_user{
			start:  off,
			length: m.erasesize,
		}

		err = IOCTL(deviceFile.Fd(), MEMERASE, uintptr(unsafe.Pointer(&e)))
		if err != nil {
			errMsg := fmt.Sprintf("Flash device '%v' erase failed: %v",
				deviceFile.Name(), err)
			log.Print(errMsg)
			return errors.Errorf("%v", errMsg)
		}
	}

	log.Printf("Finished erasing flash device '%v'", deviceFile.Name())
	return nil
}

// flashImage copies the image file into the device.
var flashImage = func(
	deviceFile flashDeviceFile,
	m mtd_info_user,
	imFile imageFile,
	roOffset uint32,
) error {
	const chunkSize = uint32(1048576)
	fileSize := uint32(len(imFile.data))

	log.Printf("Flashing image '%v' on to flash device '%v'", imFile.name, deviceFile.Name())

	if roOffset >= fileSize {
		return errors.Errorf("roOffset (%v) >= file size (%v)", roOffset, fileSize)
	}

	// flash the device in many large chunks rather than attempting to
	// do it all with a single system call.  prevents us from locking
	// up the system in kernel mode in the event of a kernel bug.
	for off := roOffset; off < fileSize; off += chunkSize {
		size := uint32(fileSize - off)
		if size > chunkSize {
			size = chunkSize
		}
		activeImageData, err := utils.BytesSliceRange(imFile.data, off, off + size)
		if err != nil {
			return errors.Errorf("Unable to get image data after roOffset (%v): %v", roOffset, err)
		}

		// use Pwrite, WriteAt may call Pwrite multiple times under the hood
		n, err := fileutils.Pwrite(int(deviceFile.Fd()), activeImageData, int64(off))
		if err != nil {
			return errors.Errorf("Failed to flash image '%v' on to flash device '%v': "+
				"%vB flashed: %v",
				imFile.name, deviceFile.Name(), n, err,
			)
		}
	}

	log.Printf("Finished flashing image '%v' on to flash device '%v'",
		imFile.name, deviceFile.Name())
	return nil
}

// verifyFlash compares the image file with the device data block by block.
var verifyFlash = func(
	deviceFilePath string,
	m mtd_info_user,
	imFile imageFile,
	roOffset uint32,
) error {
	log.Printf("Verifying copy on flash device '%v' with image file '%v'",
		deviceFilePath, imFile.name)

	imageSize := uint32(len(imFile.data))

	// use mmap here
	mtdBlockFilePath, err := devices.GetMTDBlockFilePath(deviceFilePath)
	if err != nil {
		return err
	}

	flashData, err := fileutils.MmapFileRange(
		mtdBlockFilePath, 0, int(imageSize), syscall.PROT_READ, syscall.MAP_SHARED,
	)
	if err != nil {
		return errors.Errorf("Unable to mmap flash device '%v': %v",
			deviceFilePath, err)
	}
	defer fileutils.Munmap(flashData)

	activeImageData, err := utils.BytesSliceRange(imFile.data, roOffset, uint32(len(imFile.data)))
	if err != nil {
		return errors.Errorf("Unable to get image data after roOffset (%v): %v", roOffset, err)
	}

	activeFlashData, err := utils.BytesSliceRange(flashData, roOffset, uint32(len(flashData)))
	if err != nil {
		return errors.Errorf("Unable to get flash data after roOffset (%v): %v", roOffset, err)
	}

	if !bytes.Equal(activeFlashData, activeImageData) {
		// slow scan to identify first differing byte
		var off int
		var v byte
		for off, v = range activeFlashData {
			if v != activeImageData[off] {
				break
			}
		}
		errMsg := fmt.Sprintf("Verification failed: flash and image data " +
			"mismatch beginning at offset %v + roOffset %v.", off, roOffset)
		log.Print(errMsg)
		return errors.Errorf("%v", errMsg)
	}

	log.Printf("Finished verifying copy on flash device '%v' with image file '%v'",
		deviceFilePath, imFile.name)
	return nil
}
