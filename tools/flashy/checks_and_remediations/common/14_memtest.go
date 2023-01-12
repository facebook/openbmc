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
	"log"
	"unsafe"
)

func init() {
	// XXX T142265469
	// XXX Disabled for the moment because it causes OOM Killer to
	// XXX intervene on the BMC.  Typically this kills flashy but
	// XXX it may kill other things, too.
	// step.RegisterStep(runMemtestSuite)
	step.RegisterStep(dummyRunMemtestSuite)
}

func dummyRunMemtestSuite(stepParams step.StepParams) step.StepExitError {
	log.Printf("Skipping memtest, see T142265469")
	return nil
}

// Equivalent of std::memset(scratchpadPointer, pattern, memtestSize)
func populateMemtest(pattern byte, memtestSize uint64, wordSize uintptr, scratchpadPointer unsafe.Pointer) {
	for i := uint64(0); i < memtestSize; i++ {
		*(*byte)(unsafe.Pointer(uintptr(scratchpadPointer) + (uintptr(i) * wordSize))) = pattern
	}
}

// Iterate over scratchpad and check if pattern matches every byte of it. Return error if not.
var checkMemtest = func(pattern byte, memtestSize uint64, wordSize uintptr, scratchpadPointer unsafe.Pointer) step.StepExitError {
	for i := uint64(0); i < memtestSize; i++ {
		var value = *(*byte)(unsafe.Pointer(uintptr(scratchpadPointer) + (uintptr(i) * wordSize)))
		if value != pattern {
			errMsg := errors.Errorf("Wrote pattern 0x%x to index %v of test buffer, read value was 0x%x. Possible memory corruption",
				pattern, i, value)
			return step.ExitUnsafeToReboot{Err: errMsg}
		}
	}

	return nil
}

// How many bytes should be spared in the memtest (20M)
const memtestMemoryKeepout = 20 * 1024 * 1024

// How small can the memtest run be to be considered valid (10M)
const memtestMinimumMemoryCoverage = 10 * 1024 * 1024

// Perform a suite of memtest checks.
func runMemtestSuite(stepParams step.StepParams) step.StepExitError {
	memInfo, err := utils.GetMemInfo()
	if err != nil {
		return step.ExitSafeToReboot{Err: err}
	}

	freeMem := memInfo.MemFree
	// since memtestSize is pesimistically negative, convert temporarily to signed ints
	memtestSize := int64(freeMem) - int64(memtestMemoryKeepout)

	if memtestSize < memtestMinimumMemoryCoverage {
		log.Printf("Only %v B memory left for memtest, which is less than the minimum size of %v B needed. Skipping memtest",
			memtestSize, memtestMinimumMemoryCoverage)
		return nil
	}

	// at this point memtestSize must be positive and larger than memtestMinimumMemoryCoverage
	// we are free to convert back to uint64
	uMemtestSize := uint64(memtestSize)
	log.Printf("Running memtest on %v bytes", uMemtestSize)

	// since go could probably optimize both those functions out (assuming perfectly working memory)
	// we need to create unsafe pointers that make sure that the writes are commited to ram
	// more or less C99 volatile
	var scratchpad = make([]byte, memtestSize)
	var wordSize = unsafe.Sizeof(scratchpad[0])
	var scratchpadPointer = unsafe.Pointer(&scratchpad[0])

	var patterns = [4]byte{0x00, 0xff, 0x5a, 0xa5}
	for _, pattern := range patterns {
		populateMemtest(pattern, uMemtestSize, wordSize, scratchpadPointer)
		var err = checkMemtest(pattern, uMemtestSize, wordSize, scratchpadPointer)
		if err != nil {
			return err
		}
	}

	return nil
}
