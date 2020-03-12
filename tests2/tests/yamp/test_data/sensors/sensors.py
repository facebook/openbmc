# ",/usr/bin/env python,
# ,
# Copyright 2018-present Facebook. All Rights Reserved.,
# ,
# This program file is free software; you can redistribute it and/or modify it,
# under the terms of the GNU General Public License as published by the,
# Free Software Foundation; version 2 of the License.,
# ,
# This program is distributed in the hope that it will be useful, but WITHOUT,
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or,
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License,
# for more details.,
# ,
# You should have received a copy of the GNU General Public License,
# along with this program in a file named COPYING; if not, write to the,
# Free Software Foundation, Inc.,,
# 51 Franklin Street, Fifth Floor,,
# Boston, MA 02110-1301 USA,
# ,

FAN_SENSORS = ["FAN1", "FAN2", "FAN3", "FAN4", "FAN5"]

PIM_PMBUS_SENSORS = [
    "PIM{} ECB Vin",
    "PIM{} ECB Vout",
    "PIM{} ECB Temp",
    "PIM{} ECB Power",
    "PIM{} ECB Curr",
]

PIM_MAX_SENSORS = ["PIM{} Temp1", "PIM{} Temp2"]

SUP_TEMP_SENSORS = ["Sup Temp1", "Sup Temp2"]

PIM_VOLT_SENSORS = [
    "PIM{} 12vEC",
    "PIM{} 12v_F",
    "PIM{} 3v3_E",
    "PIM{} 3v3",
    "PIM{} 1v8",
    "PIM{} 1v2",
    "PIM{} 0v9_L",
    "PIM{} 0v9_R",
    "PIM{} ECB_I",
    "PIM{} OVTMP",
]

TH3_SENSORS = [
    "TH3 VRD2 Vin",
    "TH3 VRD2 Vloop0",
    "TH3 VRD2 Vloop1",
    "TH3 VRD2 Temp",
    "TH3 VRD2 Ploop0",
    "TH3 VRD2 Ploop1",
    "TH3 VRD2 Iin",
    "TH3 VRD2 Iloop0",
    "TH3 VRD2 Iloop1",
]
