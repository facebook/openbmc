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
	"bufio"
	"bytes"
	"context"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path"
	"regexp"
	"strconv"
	"strings"
	"syscall"
	"time"
	"unsafe"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/pkg/errors"
	"github.com/vtolstov/go-ioctl"
)

// MemInfo represents memory information in bytes.
type MemInfo struct {
	MemTotal uint64
	MemFree  uint64
}

type MtdInfo struct {
	Dev       string
	Size      uint32
	Erasesize uint32
}

const ProcMtdFilePath = "/proc/mtd"
const etcIssueFilePath = "/etc/issue"
const procMountsPath = "/proc/mounts"

// other flashers + the "flashy" binary
var otherFlasherBaseNames = []string{
	"cpld_upgrade.sh",
	"flash_eraseall",
	"flashcp",
	"flashrom",
	"fw-util",
	"improve_system.py",
	"jbi",
	"psu-update-bel.py",
	"psu-update-delta.py",
	"flashy",
}

var ownCmdlines = []string{
	"/proc/self/cmdline",
	"/proc/thread-self/cmdline",
	fmt.Sprintf("/proc/%v/cmdline", os.Getpid()),
}

// GetMemInfo gets MemInfo from /proc/meminfo.
// Note that this assumes kB units for MemFree and MemTotal
// and will fail otherwise.
var GetMemInfo = func() (*MemInfo, error) {
	buf, err := fileutils.ReadFile("/proc/meminfo")
	if err != nil {
		return nil, errors.Errorf("Unable to open /proc/meminfo: %v", err)
	}

	memInfoStr := string(buf)

	memFreeRegex := regexp.MustCompile(`(?m)^MemFree: +([0-9]+) kB$`)
	memTotalRegex := regexp.MustCompile(`(?m)^MemTotal: +([0-9]+) kB$`)

	memFreeMatch := memFreeRegex.FindStringSubmatch(memInfoStr)
	if len(memFreeMatch) < 2 {
		return nil, errors.Errorf("Unable to get MemFree in /proc/meminfo")
	}
	memFree, _ := strconv.ParseUint(memFreeMatch[1], 10, 64)
	memFree *= 1024 // convert to B

	memTotalMatch := memTotalRegex.FindStringSubmatch(memInfoStr)
	if len(memTotalMatch) < 2 {
		return nil, errors.Errorf("Unable to get MemTotal in /proc/meminfo")
	}
	memTotal, _ := strconv.ParseUint(memTotalMatch[1], 10, 64)
	memTotal *= 1024 // convert to B

	return &MemInfo{
		memTotal,
		memFree,
	}, nil
}

// dropCR drops a terminal \r from the data.
func dropCR(data []byte) []byte {
	if len(data) > 0 && data[len(data)-1] == '\r' {
		return data[0 : len(data)-1]
	}
	return data
}

// scanLinesRN is an implementation of bufio.ScanLines that treats end of line as
// \r?\n or \r^\n.
func scanLinesRN(data []byte, atEOF bool) (advance int, token []byte, err error) {
	if atEOF && len(data) == 0 {
		return 0, nil, nil
	}

	// \r^\n case
	if i := bytes.IndexByte(data, '\r'); i >= 0 && i != len(data)-1 && data[i+1] != '\n' {
		// We have a \r-terminated line
		return i + 1, data[0:i], nil
	}

	// \r?\n case
	if i := bytes.IndexByte(data, '\n'); i >= 0 {
		// We have a full newline-terminated line.
		return i + 1, dropCR(data[0:i]), nil
	}

	// If we're at EOF, we have a final, non-terminated line. Return it.
	if atEOF {
		return len(data), dropCR(data), nil
	}
	// Request more data.
	return 0, nil, nil
}

// logScanner is a function to aid logging and saving live stdout and stderr output
// from a running command.
// Note that sequential execution is not guaranteed - race conditions
// might still exist.
func logScanner(s *bufio.Scanner, ch chan struct{}, pre string, str *string) {
	s.Split(scanLinesRN)
	for s.Scan() {
		t := fmt.Sprintf("%v\n", s.Text())
		log.Printf("%v%v", pre, t)
		*str = *str + t
	}
	close(ch)
}

// RunCommand runs command and pipes live output.
// Returns exitcode, error, stdout (string), stderr (string) if non-zero/error returned or timed out.
// Returns 0, nil, stdout (string), stderr (string) if successfully run.
var RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
	start := time.Now()
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	// make sure cmdArr has at least 1 item
	if len(cmdArr) == 0 {
		return 1, errors.Errorf("Cannot run empty command"), "", ""
	}

	fullCmdStr := strings.Join(cmdArr[:], " ")
	log.Printf("Running command '%v' with %v timeout", fullCmdStr, timeout)
	cmd := exec.CommandContext(ctx, cmdArr[0], cmdArr[1:]...)

	var stdoutStr, stderrStr string
	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return 1, errors.Errorf("Unable to open stdout pipe: %v", err), "", ""
	}
	stderr, err := cmd.StderrPipe()
	if err != nil {
		return 1, errors.Errorf("Unable to open stderr pipe: %v", err), "", ""
	}
	stdoutScanner := bufio.NewScanner(stdout)
	stderrScanner := bufio.NewScanner(stderr)
	stdoutDone := make(chan struct{})
	stderrDone := make(chan struct{})
	go logScanner(stdoutScanner, stdoutDone, "stdout: ", &stdoutStr)
	go logScanner(stderrScanner, stderrDone, "stderr: ", &stderrStr)

	exitCode := 1 // show 1 if failed by default

	if err := cmd.Start(); err != nil {
		log.Printf("Command '%v' failed to start: %v", fullCmdStr, err)
		// failed to start, exit now
		return exitCode, err, stdoutStr, stderrStr
	}

	<-stdoutDone
	<-stderrDone

	err = cmd.Wait()
	if err != nil {
		if exitErr, ok := err.(*exec.ExitError); ok {
			// The program exited with exit code != 0
			waitStatus := exitErr.Sys().(syscall.WaitStatus)
			exitCode = waitStatus.ExitStatus()
		} else {
			log.Printf("Could not get exit code, defaulting to '1'")
		}
	} else {
		// exit code should be 0
		waitStatus := cmd.ProcessState.Sys().(syscall.WaitStatus)
		exitCode = waitStatus.ExitStatus()
	}

	elapsed := time.Since(start)

	if ctx.Err() == context.DeadlineExceeded {
		log.Printf("Command '%v' timed out after %v", fullCmdStr, timeout)
		// replace the err with the deadline exceeded error
		// instead of just signal: killed
		err = ctx.Err()
	} else {
		log.Printf("Command '%v' exited with code %v after %v", fullCmdStr, exitCode, elapsed)
	}

	return exitCode, err, stdoutStr, stderrStr
}

// RunCommandWithRetries calls RunCommand repeatedly until succeeded or maxAttempts is reached.
// Between attempts, an interval is applied.
// Returns the results from the first succeeding run or last tried run.
var RunCommandWithRetries = func(cmdArr []string, timeout time.Duration, maxAttempts int, interval time.Duration) (int, error, string, string) {
	exitCode, err, stdoutStr, stderrStr := 1, errors.Errorf("Command failed to run"), "", ""

	if maxAttempts < 1 {
		err = errors.Errorf("Command failed to run: maxAttempts must be > 0 (got %v)", maxAttempts)
		return exitCode, err, stdoutStr, stderrStr
	}

	fullCmdStr := strings.Join(cmdArr[:], " ")
	for attempt := 1; attempt <= maxAttempts; attempt++ {
		log.Printf("Attempt %v of %v: Running command '%v' with timeout %v and retry interval %v",
			attempt, maxAttempts, fullCmdStr, timeout, interval)

		exitCode, err, stdoutStr, stderrStr = RunCommand(cmdArr, timeout)
		if err == nil {
			log.Printf("Attempt %v of %v succeeded", attempt, maxAttempts)
			break
		} else {
			log.Printf("Attempt %v of %v failed", attempt, maxAttempts)
			if attempt < maxAttempts {
				log.Printf("Sleeping for %v before retrying", interval)
				Sleep(interval)
			} else {
				log.Printf("Max attempts (%v) reached. Returning with error.", maxAttempts)
			}
		}
	}
	return exitCode, err, stdoutStr, stderrStr
}

// SystemdAvailable checks whether systemd is available.
var SystemdAvailable = func() (bool, error) {
	const cmdlinePath = "/proc/1/cmdline"

	if fileutils.FileExists(cmdlinePath) {
		buf, err := fileutils.ReadFile(cmdlinePath)
		if err != nil {
			return false, errors.Errorf("%v exists but cannot be read: %v",
				cmdlinePath, err)
		}

		contains := strings.Contains(string(buf), "systemd")

		return contains, nil
	}

	return false, nil
}

// GetOpenBMCVersionFromIssueFile gets OpenBMC version from /etc/issue.
// examples: fbtp-v2020.09.1, wedge100-v2020.07.1
// WARNING: There is no guarantee that /etc/issue is well-formed
// in old images.
var GetOpenBMCVersionFromIssueFile = func() (string, error) {
	const rVersion = "version"
	etcIssueVersionRegEx := fmt.Sprintf(`^Open ?BMC Release (?P<%v>[^\s]+)`, rVersion)

	etcIssueBuf, err := fileutils.ReadFile(etcIssueFilePath)
	if err != nil {
		return "", errors.Errorf("Error reading %v: %v",
			etcIssueFilePath, err)
	}
	etcIssueStr := string(etcIssueBuf)

	// handle ancient galaxy100 linecard release with missing version info
	if strings.HasPrefix(etcIssueStr, "OpenBMC Release \n") {
		return "unknown-v1", nil
	}

	etcIssueMap, err := GetRegexSubexpMap(
		etcIssueVersionRegEx, etcIssueStr)

	if err != nil {
		// does not match regex
		return "",
			errors.Errorf("Unable to get version from %v: %v",
				etcIssueFilePath, err)
	}

	// really old releases don't have the build name
	version := etcIssueMap[rVersion]
	if version[0] == 'v' && !strings.Contains(version, "-") {
		version = "unknown-" + version
	}
	return version, nil
}

// IsOpenBMC check whether the system is an OpenBMC
// by checking whether the string "OpenBMC" exists in /etc/issue.
var IsOpenBMC = func() (bool, error) {
	const magic = "OpenBMC"
	const ancient_magic = "Open BMC Release"

	etcIssueBuf, err := fileutils.ReadFile(etcIssueFilePath)
	if err != nil {
		return false, errors.Errorf("Error reading '%v': %v",
			etcIssueFilePath, err)
	}

	isOpenBMC := strings.Contains(string(etcIssueBuf), magic)
	if !isOpenBMC {
		isOpenBMC = strings.Contains(string(etcIssueBuf), ancient_magic)
	}
	return isOpenBMC, nil
}

// CheckOtherFlasherRunning return an error if any other flashers are running.
// It takes in the baseNames of all flashy's steps (e.g. 00_truncate_logs)
// to make sure no other instance of flashy is running.
var CheckOtherFlasherRunning = func(flashyStepBaseNames []string) error {
	allFlasherBaseNames := SafeAppendString(otherFlasherBaseNames, flashyStepBaseNames)

	otherProcCmdlinePaths := getOtherProcCmdlinePaths()

	err := checkNoBaseNameExistsInProcCmdlinePaths(allFlasherBaseNames,
		otherProcCmdlinePaths)
	if err != nil {
		return errors.Errorf("Another flasher running: %v", err)
	}
	return nil
}

var getOtherProcCmdlinePaths = func() []string {
	allCmdlines, _ := fileutils.Glob("/proc/*/cmdline")
	// ignore err as this Glob only returns bas pattern error

	otherCmdlines := StringDifference(allCmdlines, ownCmdlines)
	return otherCmdlines
}

// Examine arguments for well known base names and refine the match.
var refineBaseNameMatch = func(baseName string, params []string) bool {
	if baseName == "fw-util" {
		// ignore fw-util unless doing an update.
		return StringFind("--update", params) >= 0
	} else if baseName == "flashrom" {
		// ignore flashrom if it's just reading.
		return StringFind("-r", params) == -1
	} else {
		// default: finding the name is enough
		return true
	}
}

// checkNoBaseNameExistsInProcCmdlinePaths returns error if a basename is found running in a proc/*/cmdline file
var checkNoBaseNameExistsInProcCmdlinePaths = func(baseNames, procCmdlinePaths []string) error {
	for _, procCmdlinePath := range procCmdlinePaths {
		cmdlineBuf, err := fileutils.ReadFile(procCmdlinePath)
		if err != nil {
			// processes and their respective files in procfs naturally come and go
			continue
		}

		// Consider all command line parameters so `python improve_system.py` and
		// `improve_system.py` are both detected.
		params := strings.Split(string(cmdlineBuf), "\x00")
		for _, param := range params {
			baseName := path.Base(param)
			if StringFind(baseName, baseNames) >= 0 && refineBaseNameMatch(baseName, params) {
				return errors.Errorf("'%v' found in cmdline '%v'",
					baseName, strings.Join(params, " "))
			}
		}

	}
	return nil
}

// GetMtdInfoFromSpecifier MtdInfo for the mtd device specifier. Information is obtained from /proc/mtd.
var GetMTDInfoFromSpecifier = func(deviceSpecifier string) (MtdInfo, error) {
	var mtdInfo MtdInfo
	// read from /proc/mtd
	procMTDBuf, err := fileutils.ReadFile(ProcMtdFilePath)
	if err != nil {
		return mtdInfo, errors.Errorf("Unable to read from /proc/mtd: %v", err)
	}
	procMTD := string(procMTDBuf)

	const rDev = "dev"
	const rSize = "size"
	const rErasesize = "erasesize"

	regEx := fmt.Sprintf("(?m)^(?P<%v>mtd[0-9a-f]+): (?P<%v>[0-9a-f]+) (?P<%v>[0-9a-f]+) \"%v\"$",
		rDev, rSize, rErasesize, deviceSpecifier)

	mtdMap, err := GetRegexSubexpMap(regEx, procMTD)
	if err != nil {
		return mtdInfo, errors.Errorf("Error finding MTD entry in /proc/mtd for flash device 'mtd:%v'",
			deviceSpecifier)
	}

	size, err := strconv.ParseUint(mtdMap[rSize], 16, 32)
	if err != nil {
		return mtdInfo, errors.Errorf("Failed to parse size: %v", err)
	}
	erasesize, err := strconv.ParseUint(mtdMap[rErasesize], 16, 32)
	if err != nil {
		return mtdInfo, errors.Errorf("Failed to parse erasesize: %v", err)
	}
	return MtdInfo{
		Dev:       mtdMap[rDev],
		Size:      uint32(size),
		Erasesize: uint32(erasesize),
	}, nil
}

// IsDataPartitionMounted checks if /mnt/data is mounted
var IsDataPartitionMounted = func() (bool, error) {
	procMountsDat, err := fileutils.ReadFile(procMountsPath)
	if err != nil {
		return false, errors.Errorf("Cannot read /proc/mounts: %v", err)
	}

	regEx := `(?m)^[^ ]+ /mnt/data [^ ]+ [^ ]+ [0-9]+ [0-9]+$`
	regExMap, err := GetAllRegexSubexpMap(regEx, string(procMountsDat))
	if err != nil {
		return false, errors.Errorf("regex error: %v", err)
	}

	return len(regExMap) != 0, nil
}

func tryPetWatchdog() bool {
	var WDIOC_KEEPALIVE = ioctl.IOR('W', 5, 4)
	var WDIOC_SETTIMEOUT = ioctl.IOWR('W', 6, 4)

	f, err := os.OpenFile("/dev/watchdog", os.O_RDWR, 0)
	if err != nil {
		log.Printf("Not petting watchdog: unable to open /dev/watchdog: %v", err)
		return false
	}
	defer f.Close()

	// Extend the timeout.  The kernel may adjust the timeout downward
	// if a hardware watchdog is in use.  In any case, don't arrange to
	// restore the original timeout value because the current design for
	// flashy is that the system will reboot afterwards.
	timo := int32(300)
	err2 := ioctl.IOCTL(f.Fd(), WDIOC_SETTIMEOUT, uintptr(unsafe.Pointer(&timo)))
	if err2 != nil {
		log.Printf("ioctl WDIOC_SETTIMEOUT failed: %v", err2)
	}

	err3 := ioctl.IOCTL(f.Fd(), WDIOC_KEEPALIVE, uintptr(0))
	if err3 != nil {
		log.Printf("ioctl WDIOC_KEEPALIVE failed: %v", err3)
	}
	return err2 == nil && err3 == nil
}

// Try to pet the watchdog and increase its timeout.  This works in two ways:
//
// - When /dev/watchdog is busy because it's held open by healthd, the delay
//   here will hopefully allow healthd's watchdog thread to get some CPU
//   time.
//
// - When /dev/watchdog it NOT busy because healthd is not running and there
//   are no concurrent instances of wdtcli, the watchdog timeout will be
//   extended and the watchdog petted.
var PetWatchdog = func() {
	for i := 0; i < 10; i++ {
		if tryPetWatchdog() {
			log.Printf("Watchdog petted")
			return
		}
		time.Sleep(1 * time.Second)
	}
	log.Printf("Watchdog not petted; yielded CPU instead")
}
