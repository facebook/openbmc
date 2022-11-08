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

package common

import (
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
	"testing"
	"unsafe"
)

var mockMemInfo = &utils.MemInfo{
	MemTotal: 120 * 1024 * 1024,
	MemFree:  50 * 1024 * 1024,
}

func TestMemtestGoodHW(t *testing.T) {
	memInfoOrig := utils.GetMemInfo
	memchekOrig := checkMemtest

	utils.GetMemInfo = func() (*utils.MemInfo, error) {
		return mockMemInfo, nil
	}

	got := runMemtestSuite(step.StepParams{})
	step.CompareTestExitErrors(nil, got, t)

	utils.GetMemInfo = memInfoOrig
	checkMemtest = memchekOrig
}

func TestMemtestBadHW(t *testing.T) {
	memInfoOrig := utils.GetMemInfo
	memchekOrig := checkMemtest

	utils.GetMemInfo = func() (*utils.MemInfo, error) {
		return mockMemInfo, nil
	}

	checkMemtest = func(pattern byte, memtestSize uint64, wordSize uintptr, scratchpadPointer unsafe.Pointer) step.StepExitError {
		*(*byte)(unsafe.Pointer(uintptr(scratchpadPointer) + (uintptr(1234) * wordSize))) = 0xbb
		return memchekOrig(pattern, memtestSize, wordSize, scratchpadPointer)
	}

	got := runMemtestSuite(step.StepParams{})
	wantMsg := errors.Errorf("Wrote pattern 0x0 to index 1234 of test buffer, read value was 0xbb. Possible memory corruption")
	want := step.ExitUnsafeToReboot{Err: wantMsg}
	step.CompareTestExitErrors(want, got, t)

	utils.GetMemInfo = memInfoOrig
	checkMemtest = memchekOrig
}

func TestMemtestBadHWOnLastLoop(t *testing.T) {
	memInfoOrig := utils.GetMemInfo
	memchekOrig := checkMemtest

	utils.GetMemInfo = func() (*utils.MemInfo, error) {
		return mockMemInfo, nil
	}

	checkMemtest = func(pattern byte, memtestSize uint64, wordSize uintptr, scratchpadPointer unsafe.Pointer) step.StepExitError {
		if pattern == 0xa5 {
			*(*byte)(unsafe.Pointer(uintptr(scratchpadPointer) + (uintptr(1234) * wordSize))) = 0xbb
		}
		return memchekOrig(pattern, memtestSize, wordSize, scratchpadPointer)
	}

	got := runMemtestSuite(step.StepParams{})
	wantMsg := errors.Errorf("Wrote pattern 0xa5 to index 1234 of test buffer, read value was 0xbb. Possible memory corruption")
	want := step.ExitUnsafeToReboot{Err: wantMsg}
	step.CompareTestExitErrors(want, got, t)

	utils.GetMemInfo = memInfoOrig
	checkMemtest = memchekOrig
}

func TestMemtestNoMemory(t *testing.T) {
	memInfoOrig := utils.GetMemInfo
	memchekOrig := checkMemtest

	utils.GetMemInfo = func() (*utils.MemInfo, error) {
		return &utils.MemInfo{
			MemTotal: 120 * 1024 * 1024,
			MemFree:  2 * 1024 * 1024,
		}, nil
	}

	got := runMemtestSuite(step.StepParams{})
	step.CompareTestExitErrors(nil, got, t)

	utils.GetMemInfo = memInfoOrig
	checkMemtest = memchekOrig
}
