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
	"io"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"runtime"

	"github.com/pkg/errors"
)

var SourceRootDir string

func init() {
	// get the source root directory
	_, filename, _, ok := runtime.Caller(0)
	if !ok {
		panic("No caller information")
	}
	// go up three paths, as this file is three directories deep
	SourceRootDir = filepath.Dir(filepath.Dir(filepath.Dir(filename)))
}

func GetExecutablePath() string {
	// get the executable's (flashy's) path
	exPath, err := os.Executable()
	if err != nil {
		panic(err)
	}

	return exPath
}

// sanitize os.Args[0] to get exactly the binary name required
// e.g. if flashy was placed as /tmp/flashy
// and a call was made inside /var for ../tmp/abc/def
// this should return abc/def
func SanitizeBinaryName(arg string) string {
	// full path of the binary requested
	fullPath, err := filepath.Abs(arg)
	if err != nil {
		panic(err)
	}
	exPath := GetExecutablePath()
	exDir := filepath.Dir(exPath)

	// now we truncate fullPath to get the filepath required
	// e.g. fullPath: /tmp/abc/def
	// &    exDir	: /tmp
	// =>   binName : abc/def
	binName := fullPath[len(exDir)+1:]

	return binName
}

// from the absolute path of a file, get the intended symlink path
// e.g. /files/checks/abc.go => checks/abc
func GetSymlinkPathForSourceFile(path string) string {
	// truncate to get the symlink path
	// also truncate ".go"
	symlinkPath := path[len(SourceRootDir)+1 : len(path)-3]
	return symlinkPath
}

// basic file utilities as function variables for mocking purposes
var osStat = os.Stat
var RemoveFile = os.Remove
var TruncateFile = os.Truncate
var WriteFile = ioutil.WriteFile
var ReadFile = ioutil.ReadFile
var RenameFile = os.Rename
var CreateFile = os.Create

// PathExists returns true when the path exists
// (can be file/directory)
// defaults to `false` if os.Stat returns any other non-nil error
var PathExists = func(path string) bool {
	_, err := osStat(path)
	if err == nil {
		// exists for sure
		return true
	} else if os.IsNotExist(err) {
		// does not exist for sure
		return false
	} else {
		// may or may not exist
		log.Printf("Existence check of path '%v' returned error '%v', defaulting to false",
			path, err)
		return false
	}
}

// FileExists returns true when the file exists
// also checks that the file is not a directory
// defaults to `false` if os.Stat returns any other non-nil error
var FileExists = func(filename string) bool {
	if PathExists(filename) {
		// err guaranteed to be nil by PathExists
		info, _ := osStat(filename)
		// confirm is not a directory
		return !info.IsDir()
	}
	return false
}

// append to end of file
var AppendFile = func(filename, data string) error {
	// create if non-existent
	f, err := os.OpenFile(filename, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return err
	}
	_, err = f.Write([]byte(data))
	if err != nil {
		return err
	}
	err = f.Close()
	if err != nil {
		return err
	}
	return nil
}

// read first n bytes in a file in path
// N.B.: Use mmap if possible for large n
var ReadFirstNBytes = func(path string, n int) ([]byte, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, errors.Errorf("Unable to open file '%v': %v", path, err)
	}
	defer f.Close()

	// read first n bytes into memory
	// number of bytes read is guaranteed by non-nil err
	buf := make([]byte, n)
	_, err = io.ReadFull(f, buf)
	if err != nil {
		return nil, errors.Errorf("Error reading from file '%v': %v", path, err)
	}

	return buf, nil
}

// replace first n bytes in dstPath with first n bytes in srcPath
// WARNING: n bytes will be stored in memory, do not use for large n
// if large n required, make a buffer and loop to replace block by block
// or, use an mmap method. A non-mmap method is provided here because
// it is not possible to mmap /dev/mtd* files
var ReplaceFirstNBytes = func(dstPath, srcPath string, n int) error {
	srcBuf, err := ReadFirstNBytes(srcPath, n)
	if err != nil {
		return errors.Errorf("Unable to read from src file '%v': %v", srcPath, err)
	}

	dst, err := os.OpenFile(dstPath, os.O_WRONLY, 0644)
	if err != nil {
		return errors.Errorf("Unable to open dst file '%v': %v", dstPath, err)
	}
	defer dst.Close()

	// number of bytes wrote is guaranteed by non-nil err
	// Write defaults to beginning of file
	_, err = dst.Write(srcBuf)
	if err != nil {
		return errors.Errorf("Error writing to dst file '%v': %v", dstPath, err)
	}

	return nil
}

// read the 4 bytes in the header and check it against the ELF magic number
// 0x7F 'E', 'L', 'F'
// default to false for all other errors (e.g. file not found, no permission etc)
var IsELFFile = func(filename string) bool {
	elfMagicNumber := []byte{
		0x7F, 'E', 'L', 'F',
	}
	buf, err := ReadFirstNBytes(filename, 4)
	if err != nil {
		log.Printf("Is ELF File Check: Unable to read from file '%v': %v, "+
			"assuming that it is not an ELF file", filename, err)
		return false
	}
	return bytes.Compare(buf, elfMagicNumber) == 0
}
