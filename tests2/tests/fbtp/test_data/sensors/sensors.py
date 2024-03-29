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

SENSORS_MB = [
    "MB_INLET_TEMP",
    "MB_OUTLET_TEMP",
    "MB_INLET_REMOTE_TEMP",
    "MB_OUTLET_REMOTE_TEMP",
    "MB_FAN0_TACH",
    "MB_FAN1_TACH",
    "MB_P3V3",
    "MB_P5V",
    "MB_P12V",
    "MB_P1V05",
    "MB_PVNN_PCH_STBY",
    "MB_P3V3_STBY",
    "MB_P5V_STBY",
    "MB_P3V_BAT",
    "MB_HSC_IN_VOLT",
    "MB_HSC_OUT_CURR",
    "MB_HSC_TEMP",
    "MB_HSC_IN_POWER",
    "MB_CPU0_TEMP",
    "MB_CPU0_TJMAX",
    "MB_CPU0_PKG_POWER",
    "MB_CPU0_THERM_MARGIN",
    "MB_CPU1_TEMP",
    "MB_CPU1_TJMAX",
    "MB_CPU1_PKG_POWER",
    "MB_CPU1_THERM_MARGIN",
    "MB_PCH_TEMP",
    "MB_CPU0_DIMM_GRPA_TEMP",
    "MB_CPU0_DIMM_GRPB_TEMP",
    "MB_CPU1_DIMM_GRPC_TEMP",
    "MB_CPU1_DIMM_GRPD_TEMP",
    "MB_VR_CPU0_VCCIN_TEMP",
    "MB_VR_CPU0_VCCIN_CURR",
    "MB_VR_CPU0_VCCIN_VOLT",
    "MB_VR_CPU0_VCCIN_POWER",
    "MB_VR_CPU0_VSA_TEMP",
    "MB_VR_CPU0_VSA_CURR",
    "MB_VR_CPU0_VSA_VOLT",
    "MB_VR_CPU0_VSA_POWER",
    "MB_VR_CPU0_VCCIO_TEMP",
    "MB_VR_CPU0_VCCIO_CURR",
    "MB_VR_CPU0_VCCIO_VOLT",
    "MB_VR_CPU0_VCCIO_POWER",
    "MB_VR_CPU0_VDDQ_GRPA_TEMP",
    "MB_VR_CPU0_VDDQ_GRPA_CURR",
    "MB_VR_CPU0_VDDQ_GRPA_VOLT",
    "MB_VR_CPU0_VDDQ_GRPA_POWER",
    "MB_VR_CPU0_VDDQ_GRPB_TEMP",
    "MB_VR_CPU0_VDDQ_GRPB_CURR",
    "MB_VR_CPU0_VDDQ_GRPB_VOLT",
    "MB_VR_CPU0_VDDQ_GRPB_POWER",
    "MB_VR_CPU1_VCCIN_TEMP",
    "MB_VR_CPU1_VCCIN_CURR",
    "MB_VR_CPU1_VCCIN_VOLT",
    "MB_VR_CPU1_VCCIN_POWER",
    "MB_VR_CPU1_VSA_TEMP",
    "MB_VR_CPU1_VSA_CURR",
    "MB_VR_CPU1_VSA_VOLT",
    "MB_VR_CPU1_VSA_POWER",
    "MB_VR_CPU1_VCCIO_TEMP",
    "MB_VR_CPU1_VCCIO_CURR",
    "MB_VR_CPU1_VCCIO_VOLT",
    "MB_VR_CPU1_VCCIO_POWER",
    "MB_VR_CPU1_VDDQ_GRPC_TEMP",
    "MB_VR_CPU1_VDDQ_GRPC_CURR",
    "MB_VR_CPU1_VDDQ_GRPC_VOLT",
    "MB_VR_CPU1_VDDQ_GRPC_POWER",
    "MB_VR_CPU1_VDDQ_GRPD_TEMP",
    "MB_VR_CPU1_VDDQ_GRPD_CURR",
    "MB_VR_CPU1_VDDQ_GRPD_VOLT",
    "MB_VR_CPU1_VDDQ_GRPD_POWER",
    "MB_VR_PCH_PVNN_TEMP",
    "MB_VR_PCH_PVNN_CURR",
    "MB_VR_PCH_PVNN_VOLT",
    "MB_VR_PCH_PVNN_POWER",
    "MB_VR_PCH_P1V05_TEMP",
    "MB_VR_PCH_P1V05_CURR",
    "MB_VR_PCH_P1V05_VOLT",
    "MB_VR_PCH_P1V05_POWER",
    "MB_CONN_P12V_INA230_VOL",
    "MB_CONN_P12V_INA230_CURR",
    "MB_CONN_P12V_INA230_PWR",
    "MB_HOST_BOOT_TEMP",
]

SENSORS_MEZZ = ["MEZZ_SENSOR_TEMP"]

SENSORS_RISER_SLOT2 = [
    "MB_C2_AVA_FTEMP",
    "MB_C2_AVA_RTEMP",
    "MB_C2_NVME_CTEMP",
    "MB_C2_0_NVME_CTEMP",
    "MB_C2_1_NVME_CTEMP",
    "MB_C2_2_NVME_CTEMP",
    "MB_C2_3_NVME_CTEMP",
    "MB_C2_P12V_INA230_VOL",
    "MB_C2_P12V_INA230_CURR",
    "MB_C2_P12V_INA230_PWR",
]

SENSORS_RISER_SLOT3 = [
    "MB_C3_AVA_FTEMP",
    "MB_C3_AVA_RTEMP",
    "MB_C3_NVME_CTEMP",
    "MB_C3_0_NVME_CTEMP",
    "MB_C3_1_NVME_CTEMP",
    "MB_C3_2_NVME_CTEMP",
    "MB_C3_3_NVME_CTEMP",
    "MB_C3_P12V_INA230_VOL",
    "MB_C3_P12V_INA230_CURR",
    "MB_C3_P12V_INA230_PWR",
]

SENSORS_RISER_SLOT4 = [
    "MB_C4_AVA_FTEMP",
    "MB_C4_AVA_RTEMP",
    "MB_C4_NVME_CTEMP",
    "MB_C4_0_NVME_CTEMP",
    "MB_C4_1_NVME_CTEMP",
    "MB_C4_2_NVME_CTEMP",
    "MB_C4_3_NVME_CTEMP",
    "MB_C4_P12V_INA230_VOL",
    "MB_C4_P12V_INA230_CURR",
    "MB_C4_P12V_INA230_PWR",
]

SENSORS_AGGREGATE = ["SYSTEM_AIRFLOW"]

SENSORS = {
    "mb": SENSORS_MB,
    "nic": SENSORS_MEZZ,
    "riser_slot2": SENSORS_RISER_SLOT2,
    "riser_slot3": SENSORS_RISER_SLOT3,
    "riser_slot4": SENSORS_RISER_SLOT4,
    "aggregate": SENSORS_AGGREGATE,
}
