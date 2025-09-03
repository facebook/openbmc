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

package flash

import (
    "testing"
    "github.com/facebook/openbmc/tools/flashy/lib/step"
    "github.com/facebook/openbmc/tools/flashy/lib/utils"
)

func TestYv3Flash_PfrSystem(t *testing.T) {
    isPfrSystemOrig := utils.IsPfrSystem
    flashFwUtilOrig := flashFwUtilFunc
    flashCpVbootOrig := flashCpVbootFunc

    defer func() {
        utils.IsPfrSystem = isPfrSystemOrig
        flashFwUtilFunc = flashFwUtilOrig
        flashCpVbootFunc = flashCpVbootOrig
    }()

    utils.IsPfrSystem = func() bool { return true }
    flashFwUtilFunc = func(params step.StepParams) step.StepExitError {
        return nil
    }
    flashCpVbootFunc = func(params step.StepParams) step.StepExitError {
        t.Fatal("FlashCpVboot should not be called when IsPfrSystem is true")
        return nil
    }

    err := Yv3Flash(step.StepParams{})
    if err != nil {
        t.Errorf("want 'nil' got '%v'", err)
    }
}

func TestYv3Flash_NotPfrSystem(t *testing.T) {
    isPfrSystemOrig := utils.IsPfrSystem
    flashFwUtilOrig := flashFwUtilFunc
    flashCpVbootOrig := flashCpVbootFunc

    defer func() {
        utils.IsPfrSystem = isPfrSystemOrig
        flashFwUtilFunc = flashFwUtilOrig
        flashCpVbootFunc = flashCpVbootOrig
    }()

    utils.IsPfrSystem = func() bool { return false }
    flashFwUtilFunc = func(params step.StepParams) step.StepExitError {
        t.Fatal("FlashFwUtil should not be called when IsPfrSystem is false")
        return nil
    }
    flashCpVbootFunc = func(params step.StepParams) step.StepExitError {
        return nil
    }

    err := Yv3Flash(step.StepParams{})
    if err != nil {
        t.Errorf("want 'nil' got '%v'", err)
    }
}
