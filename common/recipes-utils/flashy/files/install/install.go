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

package install

import (
	"log"
	"os"
	"path/filepath"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"

	// all entry point packages must be included to successfully run install
	_ "github.com/facebook/openbmc/common/recipes-utils/flashy/files/checks_and_remediations/common"
	_ "github.com/facebook/openbmc/common/recipes-utils/flashy/files/checks_and_remediations/wedge100"
	_ "github.com/facebook/openbmc/common/recipes-utils/flashy/files/flash_procedure"
)

// flashy _MUST_ be installed here
// checkInstallPath will confirm and error out if not
const flashyInstallPath = "/opt/flashy/flashy"

func Install() {
	log.Println("Installing flashy...")
	checkInstallPath()
	cleanInstallPath()
	initSymlinks()
	log.Println("Finished installing flashy")
}

// check whether flashy is installed in the right path
func checkInstallPath() {
	exPath := utils.GetExecutablePath()
	if exPath != flashyInstallPath {
		log.Fatalf(`Unable to install flashy. Flashy must be installed in '%v',
currently installed in '%v' instead`, flashyInstallPath, exPath)
	}
}

// make sure the install directory is clean
func cleanInstallPath() {
	exPath := utils.GetExecutablePath()
	exDir := filepath.Dir(exPath)

	d, err := os.Open(exDir)
	defer d.Close()
	if err != nil {
		log.Fatalf("Unable to open installation directory: '%v'", err)
	}
	dirnames, err := d.Readdirnames(-1)
	if err != nil {
		log.Fatalf("Unable to read installation directory: '%v'", err)
	}

	for _, name := range dirnames {
		// don't delete flashy itself!
		if name != "flashy" {
			err := os.RemoveAll(filepath.Join(exDir, name))
			if err != nil {
				log.Fatalf("Unable to clean installation directory, '%v'", err)
			}
		}
	}
}

// init all the busybox-style symlinks
func initSymlinks() {
	// error handler
	errHandler := func(symlinkPath string, err error) {
		if err != nil {
			log.Fatalf("Unable to link '%v': '%v'", symlinkPath, err)
		}
	}

	exPath := utils.GetExecutablePath()
	exDir := filepath.Dir(exPath)

	for _symlinkPath, _ := range utils.EntryPointMap {
		// absolute symlinkPath
		symlinkPath := filepath.Join(exDir, _symlinkPath)

		// make directory up to the parent of the current path
		// this does nothing if the directory already exists
		mkdirErr := os.MkdirAll(filepath.Dir(symlinkPath), 0755)
		errHandler(symlinkPath, mkdirErr)

		// create symlink
		symlinkErr := os.Symlink(exPath, symlinkPath)
		errHandler(symlinkPath, symlinkErr)
	}
}
