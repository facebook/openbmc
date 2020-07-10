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

package main

import (
	"log"
	"os"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/install"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/utils"
)

func main() {
	binName := utils.SanitizeBinaryName(os.Args[0])

	// initiation step
	if len(os.Args) == 2 && binName == "flashy" && os.Args[1] == "--install" {
		install.Install()
		return
	}

	if len(os.Args) < 3 {
		log.Fatalf("Not enough arguments. Require `<step-name> <imageFilePath> <deviceID>`")
	}
	if _, exists := utils.EntryPointMap[binName]; !exists {
		log.Fatalf("Unknown binary '%v'", binName)
	}

	imageFilePath := os.Args[1]
	deviceID := os.Args[2]

	log.Printf("Starting: %v", binName)
	err := utils.EntryPointMap[binName](imageFilePath, deviceID)
	if err != nil {
		utils.HandleError(err)
	}
	log.Printf("Finished: %v", binName)
}
