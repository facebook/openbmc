#!/bin/bash -e

function remediation_log {
	logger "fewer than 4 nic temp sensors detected; restarting xyz.openbmc_project.hwmontempsensor"
}

function remediation_action {
	systemctl restart xyz.openbmc_project.hwmontempsensor.service
}

SENSOR_COUNT=$(busctl tree xyz.openbmc_project.HwmonTempSensor | awk '/NIC._TEMP_C/{print $NF}' | wc -l)

if [ "$SENSOR_COUNT" -lt 4 ]; then
	remediation_log
	remediation_action
fi

exit 0
