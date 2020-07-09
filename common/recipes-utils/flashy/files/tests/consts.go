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

// constants/examples used for testing
package tests

import (
	"github.com/facebook/openbmc/tree/helium/common/recipes-utils/flashy/files/utils"
)

const ExampleWedge100MemInfo = `MemTotal:         246900 kB
MemFree:           88928 kB
MemAvailable:     192496 kB
Buffers:               0 kB
Cached:           107288 kB
SwapCached:            0 kB
Active:            62848 kB
Inactive:          80988 kB
Active(anon):      36872 kB
Inactive(anon):      340 kB
Active(file):      25976 kB
Inactive(file):    80648 kB
Unevictable:           0 kB
Mlocked:               0 kB
SwapTotal:             0 kB
SwapFree:              0 kB
Dirty:                 0 kB
Writeback:             0 kB
AnonPages:         36560 kB
Mapped:            12680 kB
Shmem:               664 kB
Slab:               8800 kB
SReclaimable:       3440 kB
SUnreclaim:         5360 kB
KernelStack:         656 kB
PageTables:         1184 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:      123448 kB
Committed_AS:     248268 kB
VmallocTotal:     770048 kB
VmallocUsed:        1280 kB
VmallocChunk:     635736 kB`

// minilaketb healthd-config.json
const ExampleMinilaketbHealthdConfigJSON = `{
	"version": "1.0",
	"heartbeat": {
		"interval": 500
	},
	"bmc_cpu_utilization": {
		"enabled": true,
		"window_size": 120,
		"monitor_interval": 1,
		"threshold": [
			{
				"value": 80.0,
				"hysteresis": 5.0,
				"action": [
					"log-warning",
					"bmc-error-trigger"
				]
			}
		]
	},
	"bmc_mem_utilization": {
		"enabled": true,
		"enable_panic_on_oom": true,
		"window_size": 120,
		"monitor_interval": 1,
		"threshold": [
			{
				"value": 60.0,
				"hysteresis": 5.0,
				"action": [
					"log-warning"
				]
			},
			{
				"value": 70.0,
				"hysteresis": 5.0,
				"action": [
					"log-critical",
					"bmc-error-trigger"
				]
			},
			{
				"value": 95.0,
				"hysteresis": 5.0,
				"action": [
					"log-critical",
					"bmc-error-trigger",
					"reboot"
				]
			}
		]
	},
	"i2c": {
		"enabled": false,
		"busses": [
			0,
			1,
			2,
			3,
			4,
			5,
			6,
			7,
			8,
			9,
			10,
			11,
			12,
			13
		]
	},
	"ecc_monitoring": {
		"enabled": false,
		"ecc_address_log": false,
		"monitor_interval": 2,
		"recov_max_counter": 255,
		"unrec_max_counter": 15,
		"recov_threshold": [
			{
				"value": 0.0,
				"action": [
					"log-critical",
					"bmc-error-trigger"
				]
			},
			{
				"value": 50.0,
				"action": [
					"log-critical"
				]
			},
			{
				"value": 90.0,
				"action": [
					"log-critical"
				]
			}
		],
		"unrec_threshold": [
			{
				"value": 0.0,
				"action": [
					"log-critical",
					"bmc-error-trigger"
				]
			},
			{
				"value": 50.0,
				"action": [
					"log-critical"
				]
			},
			{
				"value": 90.0,
				"action": [
					"log-critical"
				]
			}
		]
	},
	"bmc_health": {
		"enabled": false,
		"monitor_interval": 2,
		"regenerating_interval": 1200
	},
	"verified_boot": {
		"enabled": false
	}
}`

// minimal part of minilaketb json
const ExampleMinimalHealthdConfigJSON = `{
	"bmc_mem_utilization": {
		"threshold": [
			{
				"value": 60.0,
				"hysteresis": 5.0,
				"action": [
					"log-warning"
				]
			},
			{
				"value": 70.0,
				"hysteresis": 5.0,
				"action": [
					"log-critical",
					"bmc-error-trigger"
				]
			},
			{
				"value": 95.0,
				"hysteresis": 5.0,
				"action": [
					"log-critical",
					"bmc-error-trigger",
					"reboot"
				]
			}
		]
	}
}`

const ExampleCorruptedHealthdConfig = `{
	"BMC_MEM_UTILIZATI: {
		"threshold": [
			{
				"value": 60.0,
				"hysteresis": 5.0,
				"action": [
					"log-warning"
				]
			},
			{
				"value": 70.0,
				"hysteresis": 5.0,
				"action": [
					"log-critical",
					"bmc-error-trigger"
				]
			},
			{
				"value": 95.0,
				"hysteresis": 5.0,
				"action": [
					"log-critical",
					"bmc-error-trigger",
					"reboot"
				]
			}
		]
	}
}`

var ExampleMinilaketbHealthdConfig = utils.HealthdConfig{
	utils.BmcMemUtil{
		[]utils.BmcMemThres{
			utils.BmcMemThres{
				Value:   float32(60.0),
				Actions: []string{"log-warning"},
			},
			utils.BmcMemThres{
				Value:   float32(70.0),
				Actions: []string{"log-critical", "bmc-error-trigger"},
			},
			utils.BmcMemThres{
				Value:   float32(95.0),
				Actions: []string{"log-critical", "bmc-error-trigger", "reboot"},
			},
		},
	},
}

var ExampleNoRebootHealthdConfig = utils.HealthdConfig{
	utils.BmcMemUtil{
		[]utils.BmcMemThres{
			utils.BmcMemThres{
				Value:   float32(60.0),
				Actions: []string{"log-warning"},
			},
			utils.BmcMemThres{
				Value:   float32(70.0),
				Actions: []string{"log-critical", "bmc-error-trigger"},
			},
		},
	},
}
