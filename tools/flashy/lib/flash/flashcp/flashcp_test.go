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

package flashcp

import (
	"bytes"
	"testing"
	"unsafe"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

type mockFlashDeviceFile struct {
	writeErr       error
	seekErr        error
	deviceFilePath string
	closed         bool
	seekedAddr     int64
	writtenBytes   []byte
	writesCalled   int
}

func (f *mockFlashDeviceFile) Fd() uintptr {
	return 42
}

func (f *mockFlashDeviceFile) Write(a []byte) (int, error) {
	f.writtenBytes = append(f.writtenBytes, a...)
	f.writesCalled++
	return 0, f.writeErr
}

func (f *mockFlashDeviceFile) Seek(addr int64, whence int) (int64, error) {
	f.seekedAddr = addr
	return addr, f.seekErr
}

func (f *mockFlashDeviceFile) Close() error {
	f.closed = true
	return nil
}

func (f *mockFlashDeviceFile) Name() string {
	return f.deviceFilePath
}

func TestFlashCp(t *testing.T) {
	// mock and defer restore openFlashDeviceFile, getMtdInfoUser, MmapFile, Munmap,
	// runFlashProcess & closeFlashDeviceFile
	openFileOrig := openFlashDeviceFile
	getMtdInfoUserOrig := getMtdInfoUser
	mmapFileOrig := fileutils.MmapFile
	munmapOrig := fileutils.Munmap
	runFlashProcessOrig := runFlashProcess
	closeFileOrig := closeFlashDeviceFile
	defer func() {
		openFlashDeviceFile = openFileOrig
		getMtdInfoUser = getMtdInfoUserOrig
		fileutils.MmapFile = mmapFileOrig
		fileutils.Munmap = munmapOrig
		runFlashProcess = runFlashProcessOrig
		closeFlashDeviceFile = closeFileOrig
	}()

	cases := []struct {
		name               string
		openFileErr        error
		closeFileErr       error
		getMtdInfoError    error
		mmapFileErr        error
		runFlashProcessErr error
		want               error
	}{
		{
			name:               "success",
			openFileErr:        nil,
			closeFileErr:       nil,
			getMtdInfoError:    nil,
			mmapFileErr:        nil,
			runFlashProcessErr: nil,
			want:               nil,
		},
		{
			name:               "open file failed",
			openFileErr:        errors.Errorf("open file failed"),
			closeFileErr:       nil,
			getMtdInfoError:    nil,
			mmapFileErr:        nil,
			runFlashProcessErr: nil,
			want: errors.Errorf("Unable to open flash device file " +
				"'/dev/mtd42': open file failed"),
		},
		{
			name:               "close file failed",
			openFileErr:        nil,
			closeFileErr:       errors.Errorf("close file failed"),
			getMtdInfoError:    nil,
			mmapFileErr:        nil,
			runFlashProcessErr: nil,
			want: errors.Errorf("Unable to close flash device file " +
				"'/dev/mtd42': close file failed"),
		},
		{
			name:               "get mtd info failed",
			openFileErr:        nil,
			closeFileErr:       nil,
			getMtdInfoError:    errors.Errorf("get mtd info failed"),
			mmapFileErr:        nil,
			runFlashProcessErr: nil,
			want: errors.Errorf("Can't get mtd_info_user for '/dev/mtd42', " +
				"this may not be a MTD flash device: get mtd info failed"),
		},
		{
			name:               "mmap failed",
			openFileErr:        nil,
			closeFileErr:       nil,
			getMtdInfoError:    nil,
			mmapFileErr:        errors.Errorf("mmap failed"),
			runFlashProcessErr: nil,
			want: errors.Errorf("Can't mmap image file " +
				"'/opt/upgrade/image': mmap failed"),
		},
		{
			name:               "flash process failed",
			openFileErr:        nil,
			closeFileErr:       nil,
			getMtdInfoError:    nil,
			mmapFileErr:        nil,
			runFlashProcessErr: errors.Errorf("flash process failed"),
			want:               errors.Errorf("flash process failed"),
		},
	}

	exampleDeviceFilePath := "/dev/mtd42"
	exampleImageFilePath := "/opt/upgrade/image"
	mtdinfouser := mtd_info_user{}
	exampleMockFile := &mockFlashDeviceFile{
		deviceFilePath: exampleDeviceFilePath,
	}
	exampleImageData := []byte("foobar")

	fileutils.Munmap = func(data []byte) error {
		return nil
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			openFlashDeviceFile = func(name string) (flashDeviceFile, error) {
				if name != exampleDeviceFilePath {
					t.Errorf("device file name: want '%v' got '%v'",
						exampleDeviceFilePath, name)
				}
				return exampleMockFile, tc.openFileErr
			}
			closeFlashDeviceFile = func(f flashDeviceFile) error {
				f.Close()
				return tc.closeFileErr
			}
			getMtdInfoUser = func(fd uintptr) (mtd_info_user, error) {
				if fd != 42 {
					t.Errorf("fd: want '%v' got '%v'", 42, fd)
				}
				return mtdinfouser, tc.getMtdInfoError
			}
			fileutils.MmapFile = func(filename string, prot, flags int) ([]byte, error) {
				if filename != exampleImageFilePath {
					t.Errorf("image file path: want '%v' got '%v'",
						exampleImageFilePath, filename)
				}
				return exampleImageData, tc.mmapFileErr
			}
			runFlashProcess = func(
				deviceFilePath string,
				m mtd_info_user,
				imFile imageFile) error {
				// make sure flashDeviceFile is closed
				if !exampleMockFile.closed {
					t.Errorf("flash device file should be closed before running " +
						"flash process!")
				}

				return tc.runFlashProcessErr
			}

			err := FlashCp(exampleImageFilePath, exampleDeviceFilePath)
			tests.CompareTestErrors(tc.want, err, t)
		})
	}
}

func TestRunFlashProcess(t *testing.T) {
	openFlashDeviceFileOrig := openFlashDeviceFile
	closeFlashDeviceFileOrig := closeFlashDeviceFile
	healthCheckOrig := healthCheck
	eraseFlashDeviceOrig := eraseFlashDevice
	flashImageOrig := flashImage
	verifyFlashOrig := verifyFlash
	defer func() {
		openFlashDeviceFile = openFlashDeviceFileOrig
		closeFlashDeviceFile = closeFlashDeviceFileOrig
		healthCheck = healthCheckOrig
		eraseFlashDevice = eraseFlashDeviceOrig
		flashImage = flashImageOrig
		verifyFlash = verifyFlashOrig
	}()

	cases := []struct {
		name string
		// errors for the mocked functions
		oErr error
		cErr error
		hErr error
		eErr error
		fErr error
		vErr error
		want error
	}{
		{
			name: "success",
		},
		{
			name: "openFlashDeviceFile error",
			oErr: errors.Errorf("openFlashDeviceFile error"),
			want: errors.Errorf("Unable to open flash device file '/dev/mtd42': " +
				"openFlashDeviceFile error"),
		},
		{
			name: "healthCheck error",
			hErr: errors.Errorf("healthCheck error"),
			want: errors.Errorf("healthCheck error"),
		},
		{
			name: "eraseFlash error",
			eErr: errors.Errorf("eraseFlash error"),
			want: errors.Errorf("eraseFlash error"),
		},
		{
			name: "flashImage error",
			fErr: errors.Errorf("flashImage error"),
			want: errors.Errorf("flashImage error"),
		},
		{
			name: "verifyFlash error",
			vErr: errors.Errorf("verifyFlash error"),
			want: errors.Errorf("verifyFlash error"),
		},
		{
			name: "closeFlashDevice error",
			cErr: errors.Errorf("closeFlashDevice error"),
			want: errors.Errorf("Unable to close flash device file '/dev/mtd42': " +
				"closeFlashDevice error"),
		},
	}

	exampleDeviceFilePath := "/dev/mtd42"
	exampleImageFilePath := "/opt/upgrade/image"
	exampleImageData := []byte("foobar")
	exampleImFile := imageFile{
		exampleImageFilePath,
		exampleImageData,
	}
	mtdinfouser := mtd_info_user{}
	exampleMockFile := &mockFlashDeviceFile{
		deviceFilePath: exampleDeviceFilePath,
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			openFlashDeviceFile = func(deviceFilePath string) (flashDeviceFile, error) {
				if deviceFilePath != exampleDeviceFilePath {
					t.Errorf("device file path: want '%v' got '%v'",
						exampleDeviceFilePath, deviceFilePath)
				}
				return exampleMockFile, tc.oErr
			}
			closeFlashDeviceFile = func(f flashDeviceFile) error {
				f.Close()
				return tc.cErr
			}
			healthCheck = func(deviceFile flashDeviceFile, m mtd_info_user, imFile imageFile) error {
				return tc.hErr
			}
			eraseFlashDevice = func(deviceFile flashDeviceFile, m mtd_info_user, imFile imageFile) error {
				return tc.eErr
			}
			flashImage = func(deviceFile flashDeviceFile, m mtd_info_user, imFile imageFile) error {
				return tc.fErr
			}
			verifyFlash = func(deviceFilePath string, m mtd_info_user, imFile imageFile) error {
				if !exampleMockFile.closed {
					t.Errorf("flash device file must be closed before calling verifyFlash!")
				}
				return tc.vErr
			}
			err := runFlashProcess(exampleDeviceFilePath, mtdinfouser, exampleImFile)
			tests.CompareTestErrors(tc.want, err, t)
		})
	}
}

func TestGetMtdInfoUser(t *testing.T) {
	IOCTLOrig := IOCTL
	defer func() {
		IOCTL = IOCTLOrig
	}()

	cases := []struct {
		name     string
		ioctlErr error
		wantErr  error
	}{
		{
			name:     "success",
			ioctlErr: nil,
			wantErr:  nil,
		},
		{
			name:     "failed",
			ioctlErr: errors.Errorf("ioctl failed"),
			wantErr: errors.Errorf("Can't get mtd_info_user: %v",
				"ioctl failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			IOCTL = func(fd, name, data uintptr) error {
				if fd != 42 {
					t.Errorf("fd: want '42' got '%v'", fd)
				}
				if name != MEMGETINFO {
					t.Errorf("name: want '%v' got '%v'", MEMGETINFO, name)
				}
				return tc.ioctlErr
			}
			_, err := getMtdInfoUser(42)
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestHealthCheck(t *testing.T) {
	imData := []byte("foobar")
	imFile := imageFile{
		data: imData,
	}

	cases := []struct {
		name           string
		deviceFilePath string
		deviceFileSize uint32
		want           error
	}{
		{
			name:           "success",
			deviceFilePath: "/dev/mtd42",
			deviceFileSize: 42,
			want:           nil,
		},
		{
			name:           "deviceFilePath pattern wrong",
			deviceFilePath: "/dev/mtd42a",
			deviceFileSize: 42,
			want: errors.Errorf("Device file path '/dev/mtd42a' does not " +
				"match required pattern '^/dev/mtd[0-9]+$'"),
		},
		{
			name:           "device too small for image",
			deviceFilePath: "/dev/mtd42",
			deviceFileSize: 1,
			want:           errors.Errorf("Image size (6B) larger than flash device size (1B)"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			deviceFile := &mockFlashDeviceFile{
				deviceFilePath: tc.deviceFilePath,
			}
			m := mtd_info_user{
				size: tc.deviceFileSize,
			}
			err := healthCheck(deviceFile, m, imFile)
			tests.CompareTestErrors(tc.want, err, t)
		})
	}
}

func TestEraseFlashDevice(t *testing.T) {
	IOCTLOrig := IOCTL
	defer func() {
		IOCTL = IOCTLOrig
	}()

	imData := []byte("foobar")
	imFile := imageFile{
		data: imData,
	}
	deviceFile := &mockFlashDeviceFile{
		deviceFilePath: "/dev/mtd5",
	}
	cases := []struct {
		name           string
		mtdErasesize   uint32
		wantEraseLen   uint32
		wantIoctlCalls int
		ioctlErr       error
		want           error
	}{
		{
			name:           "success",
			mtdErasesize:   4,
			wantEraseLen:   4,
			wantIoctlCalls: 2,
			ioctlErr:       nil,
			want:           nil,
		},
		{
			name:           "invalid erasesize 0",
			mtdErasesize:   0,
			wantEraseLen:   0,
			wantIoctlCalls: 0,
			ioctlErr:       nil,
			want:           errors.Errorf("invalid mtd device erasesize: 0"),
		},
		{
			name:           "erase failed",
			mtdErasesize:   4,
			wantEraseLen:   4,
			wantIoctlCalls: 1,
			ioctlErr:       errors.Errorf("erase failed"),
			want:           errors.Errorf("Flash device '/dev/mtd5' erase failed at 0x0: erase failed"),
		},
	}

	fileutils.Munmap = func(data []byte) error {
		return nil
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			gotIoctlCalls := 0
			m := mtd_info_user{
				erasesize: tc.mtdErasesize,
			}
			IOCTL = func(fd, name, data uintptr) error {
				gotIoctlCalls++
				if fd != 42 {
					t.Errorf("fd: want '42' got '%v'", fd)
				}
				if name != MEMERASE {
					t.Errorf("name: want '%v' got '%v'", MEMERASE, name)
				}
				gotM := (*erase_info_user)(unsafe.Pointer(data))
				if gotM.length != tc.wantEraseLen {
					t.Errorf("erase length: want '%v' got '%v'",
						tc.wantEraseLen, gotM.length)
				}
				return tc.ioctlErr
			}
			err := eraseFlashDevice(deviceFile, m, imFile)
			tests.CompareTestErrors(tc.want, err, t)
			if tc.wantIoctlCalls != gotIoctlCalls {
				t.Errorf("ioctl calls: want '%v' got '%v'",
					tc.wantIoctlCalls, gotIoctlCalls)
			}
		})
	}
}

func TestFlashImage(t *testing.T) {
	imData := []byte("foobar")
	imFile := imageFile{
		name: "/opt/upgrade/image",
		data: imData,
	}
	m := mtd_info_user{
		erasesize: 4,
	}
	cases := []struct {
		name         string
		seekErr      error
		writeErr     error
		writesCalled int
		want         error
	}{
		{
			name:         "success",
			seekErr:      nil,
			writeErr:     nil,
			writesCalled: 2,
			want:         nil,
		},
		{
			name:         "seek failed",
			seekErr:      errors.Errorf("seek failed"),
			writeErr:     nil,
			writesCalled: 0,
			want: errors.Errorf("Unable to seek to beginning of flash device " +
				"'/dev/mtd5': seek failed"),
		},
		{
			name:         "write failed",
			seekErr:      nil,
			writeErr:     errors.Errorf("write failed"),
			writesCalled: 1,
			want: errors.Errorf("Flashing image '/opt/upgrade/image' on " +
				"to flash device '/dev/mtd5' failed at 0x0: write failed"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			deviceFile := &mockFlashDeviceFile{
				deviceFilePath: "/dev/mtd5",
				writeErr:       tc.writeErr,
				seekErr:        tc.seekErr,
			}
			err := flashImage(deviceFile, m, imFile)
			tests.CompareTestErrors(tc.want, err, t)
			if tc.writesCalled != deviceFile.writesCalled {
				t.Errorf("writes called: want '%v' got '%v'",
					tc.writesCalled, deviceFile.writesCalled)
			}
			if err == nil {
				if bytes.Compare(imData, deviceFile.writtenBytes) != 0 {
					t.Errorf("written bytes: want '%v' got '%v'",
						imData, deviceFile.writtenBytes)
				}
				if deviceFile.seekedAddr != 0 {
					t.Errorf("want seek to 0, got to '%v'", deviceFile.seekedAddr)
				}
			}
		})
	}
}

func TestVerifyFlash(t *testing.T) {
	mmapFileRangeOrig := fileutils.MmapFileRange
	munmapOrig := fileutils.Munmap
	defer func() {
		fileutils.MmapFileRange = mmapFileRangeOrig
		fileutils.Munmap = munmapOrig
	}()
	fileutils.Munmap = func(b []byte) error {
		return nil
	}

	imData := []byte("foobar")
	imFile := imageFile{
		name: "/opt/upgrade/image",
		data: imData,
	}
	m := mtd_info_user{
		erasesize: 4,
	}

	cases := []struct {
		name           string
		deviceFileName string
		deviceData     []byte
		mmapErr        error
		want           error
	}{
		{
			name:           "success",
			deviceFileName: "/dev/mtd42",
			deviceData:     imData,
			mmapErr:        nil,
			want:           nil,
		},
		{
			name:           "invalid device file name, get block file path failed",
			deviceFileName: "/dev/mtd42a",
			deviceData:     imData,
			mmapErr:        nil,
			want: errors.Errorf("Unable to get block file path for '/dev/mtd42a': " +
				"No match for regex '^(?P<devmtdpath>/dev/mtd)(?P<mtdnum>[0-9]+)$' " +
				"for input '/dev/mtd42a'"),
		},
		{
			name:           "mmap failed",
			deviceFileName: "/dev/mtd42",
			deviceData:     imData,
			mmapErr:        errors.Errorf("mmap failed"),
			want:           errors.Errorf("Unable to mmap flash device '/dev/mtd42': mmap failed"),
		},
		{
			name:           "device data does not match",
			deviceFileName: "/dev/mtd42",
			deviceData:     []byte("foobrr"),
			mmapErr:        nil,
			want:           errors.Errorf("Verification failed: flash and image data mismatch at 0x4"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {

			fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
				if length != 6 {
					t.Errorf("length: want '6' got '%v'", length)
				}
				if offset != 0 {
					t.Errorf("offset: want '0' got '%v'", offset)
				}
				return tc.deviceData, tc.mmapErr
			}
			err := verifyFlash(tc.deviceFileName, m, imFile)
			tests.CompareTestErrors(tc.want, err, t)
		})
	}
}
