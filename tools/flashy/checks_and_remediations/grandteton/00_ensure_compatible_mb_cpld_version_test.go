package remediations_grandteton

import (
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

var fwUtilOutputOldCpldExample = `
[
    {
        "COMPONENT": "mb_cpld",
        "FRU": "mb",
        "PRETTY_COMPONENT": "MB_CPLD",
        "VERSION": "00020008"
    }
]
`

var fwUtilOutputNewCpldExample = `
[
    {
        "COMPONENT": "mb_cpld",
        "FRU": "mb",
        "PRETTY_COMPONENT": "MB_CPLD",
        "VERSION": "00020010"
    }
]
`

func TestEnsureCompatibleMBCPLDVersion(t *testing.T) {
	// mock and defer restore RunCommand
	runCommandOrig := utils.RunCommand
	getOpenBMCVersionFromImageFileOrig := GetOpenBMCVersionFromImageFile
	defer func() {
		utils.RunCommand = runCommandOrig
		GetOpenBMCVersionFromImageFile = getOpenBMCVersionFromImageFileOrig
	}()
	cases := []struct {
		name            string
		bmcImageVersion string
		runCmdStdout    string
		want            step.StepExitError
	}{
		{
			name:            "succeeded as image version == v2023.39.2 and CPLD is old",
			bmcImageVersion: "grandteton-v2023.39.2",
			runCmdStdout:    fwUtilOutputOldCpldExample,
			want:            nil,
		},
		{
			name:            "succeeded as image version == v2024.11.1 and CPLD is new",
			bmcImageVersion: "grandteton-v2024.11.1",
			runCmdStdout:    fwUtilOutputNewCpldExample,
			want:            nil,
		},
		{
			name:            "succeeded as image version > v2024.11.1 and CPLD is new",
			bmcImageVersion: "grandteton-v2024.12.1",
			runCmdStdout:    fwUtilOutputNewCpldExample,
			want:            nil,
		},
		{
			name:            "failed as image version == v2024.11.1 but CPLD is old",
			bmcImageVersion: "grandteton-v2024.11.1",
			runCmdStdout:    fwUtilOutputOldCpldExample,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("S365492: Image version ('grandteton-v2024.11.1') requires MB CPLD version 20010 or greater (current MB CPLD version is 20008)"),
			},
		},
		{
			name:            "failed as image version > v2024.11.1 but CPLD is old",
			bmcImageVersion: "grandteton-v2024.12.0",
			runCmdStdout:    fwUtilOutputOldCpldExample,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("S365492: Image version ('grandteton-v2024.12.0') requires MB CPLD version 20010 or greater (current MB CPLD version is 20008)"),
			},
		},
		{
			name:            "invalid image version",
			bmcImageVersion: "fby3-v2024.12.0",
			runCmdStdout:    fwUtilOutputOldCpldExample,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Image version 'fby3-v2024.12.0' does not match expected format ^grandteton-(v[0-9]+[.][0-9]+[.][0-9]+)$"),
			},
		},
	}

	wantCmd := "fw-util mb --version-json mb_cpld"
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				gotCmd := strings.Join(cmdArr, " ")
				if wantCmd != gotCmd {
					t.Errorf("cmd: want '%v' got '%v'", wantCmd, gotCmd)
				}
				return 0, nil, tc.runCmdStdout, ""
			}
			GetOpenBMCVersionFromImageFile = func(imageFile string) (string, error) {
				return tc.bmcImageVersion, nil
			}
			got := ensureCompatibleMBCPLDVersion(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
