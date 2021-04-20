#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

# HACK: Temporary schema until we use something more robust
PAYLOAD_SCHEMA = {
    "global_ts": int,
    "data": {
        "qsfp_1_temp": {"value": float, "ts": int},  # QSFP port 1 temperature
        "qsfp_2_temp": {"value": float, "ts": int},  # QSFP port 2 temperature
        "qsfp_3_temp": {"value": float, "ts": int},  # QSFP port 3 temperature
        "qsfp_4_temp": {"value": float, "ts": int},  # QSFP port 4 temperature
        "qsfp_5_temp": {"value": float, "ts": int},  # QSFP port 5 temperature
        "qsfp_6_temp": {"value": float, "ts": int},  # QSFP port 6 temperature
        "qsfp_7_temp": {"value": float, "ts": int},  # QSFP port 7 temperature
        "qsfp_8_temp": {"value": float, "ts": int},  # QSFP port 8 temperature
        "qsfp_9_temp": {"value": float, "ts": int},  # QSFP port 9 temperature
        "qsfp_10_temp": {"value": float, "ts": int},  # QSFP port 10 temperature
        "qsfp_11_temp": {"value": float, "ts": int},  # QSFP port 11 temperature
        "qsfp_12_temp": {"value": float, "ts": int},  # QSFP port 12 temperature
        "qsfp_13_temp": {"value": float, "ts": int},  # QSFP port 13 temperature
        "qsfp_14_temp": {"value": float, "ts": int},  # QSFP port 14 temperature
        "qsfp_15_temp": {"value": float, "ts": int},  # QSFP port 15 temperature
        "qsfp_16_temp": {"value": float, "ts": int},  # QSFP port 16 temperature
        "qsfp_17_temp": {"value": float, "ts": int},  # QSFP port 17 temperature
        "qsfp_18_temp": {"value": float, "ts": int},  # QSFP port 18 temperature
        "qsfp_19_temp": {"value": float, "ts": int},  # QSFP port 19 temperature
        "qsfp_20_temp": {"value": float, "ts": int},  # QSFP port 20 temperature
        "qsfp_21_temp": {"value": float, "ts": int},  # QSFP port 21 temperature
        "qsfp_22_temp": {"value": float, "ts": int},  # QSFP port 22 temperature
        "qsfp_23_temp": {"value": float, "ts": int},  # QSFP port 23 temperature
        "qsfp_24_temp": {"value": float, "ts": int},  # QSFP port 24 temperature
        "qsfp_25_temp": {"value": float, "ts": int},  # QSFP port 25 temperature
        "qsfp_26_temp": {"value": float, "ts": int},  # QSFP port 26 temperature
        "qsfp_27_temp": {"value": float, "ts": int},  # QSFP port 27 temperature
        "qsfp_28_temp": {"value": float, "ts": int},  # QSFP port 28 temperature
        "qsfp_29_temp": {"value": float, "ts": int},  # QSFP port 29 temperature
        "qsfp_30_temp": {"value": float, "ts": int},  # QSFP port 30 temperature
        "qsfp_31_temp": {"value": float, "ts": int},  # QSFP port 31 temperature
        "qsfp_32_temp": {"value": float, "ts": int},  # QSFP port 32 temperature
        "macsec_1_temp1": {"value": float, "ts": int},  # macsec 1 temperature 1
        "macsec_1_temp2": {"value": float, "ts": int},  # macsec 1 temperature 2
        "macsec_2_temp1": {"value": float, "ts": int},  # macsec 2 temperature 1
        "macsec_2_temp2": {"value": float, "ts": int},  # macsec 2 temperature 2
        "macsec_3_temp1": {"value": float, "ts": int},  # macsec 3 temperature 1
        "macsec_3_temp2": {"value": float, "ts": int},  # macsec 3 temperature 2
        "macsec_4_temp1": {"value": float, "ts": int},  # macsec 4 temperature 1
        "macsec_4_temp2": {"value": float, "ts": int},  # macsec 4 temperature 2
        "macsec_5_temp1": {"value": float, "ts": int},  # macsec 5 temperature 1
        "macsec_5_temp2": {"value": float, "ts": int},  # macsec 5 temperature 2
        "macsec_6_temp1": {"value": float, "ts": int},  # macsec 6 temperature 1
        "macsec_6_temp2": {"value": float, "ts": int},  # macsec 6 temperature 2
        "macsec_7_temp1": {"value": float, "ts": int},  # macsec 7 temperature 1
        "macsec_7_temp2": {"value": float, "ts": int},  # macsec 7 temperature 2
        "macsec_8_temp1": {"value": float, "ts": int},  # macsec 8 temperature 1
        "macsec_8_temp2": {"value": float, "ts": int},  # macsec 8 temperature 2
        "macsec_9_temp1": {"value": float, "ts": int},  # macsec 9 temperature 1
        "macsec_9_temp2": {"value": float, "ts": int},  # macsec 9 temperature 2
        "macsec_10_temp1": {"value": float, "ts": int},  # macsec 10 temperature 1
        "macsec_10_temp2": {"value": float, "ts": int},  # macsec 10 temperature 2
        "macsec_11_temp1": {"value": float, "ts": int},  # macsec 11 temperature 1
        "macsec_11_temp2": {"value": float, "ts": int},  # macsec 11 temperature 2
        "macsec_12_temp1": {"value": float, "ts": int},  # macsec 12 temperature 1
        "macsec_12_temp2": {"value": float, "ts": int},  # macsec 12 temperature 2
        "macsec_13_temp1": {"value": float, "ts": int},  # macsec 13 temperature 1
        "macsec_13_temp2": {"value": float, "ts": int},  # macsec 13 temperature 2
        "macsec_14_temp1": {"value": float, "ts": int},  # macsec 14 temperature 1
        "macsec_14_temp2": {"value": float, "ts": int},  # macsec 14 temperature 2
        "macsec_15_temp1": {"value": float, "ts": int},  # macsec 15 temperature 1
        "macsec_15_temp2": {"value": float, "ts": int},  # macsec 15 temperature 2
        "macsec_16_temp1": {"value": float, "ts": int},  # macsec 16 temperature 1
        "macsec_16_temp2": {"value": float, "ts": int},  # macsec 16 temperature 2
    },
}

EXAMPLE_PAYLOAD = {
    "global_ts": 1604676343,
    "data": {
        "qsfp_1_temp": {"value": 68.7, "ts": 1604676372},
        "qsfp_2_temp": {"value": 70.7, "ts": 1604676372},
        "qsfp_3_temp": {"value": 72.3, "ts": 1604676372},
        "qsfp_4_temp": {"value": 71.6, "ts": 1604676372},
        "qsfp_5_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_6_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_7_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_8_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_9_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_10_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_11_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_12_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_13_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_14_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_15_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_16_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_17_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_18_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_19_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_20_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_21_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_22_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_23_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_24_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_25_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_26_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_27_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_28_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_29_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_30_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_31_temp": {"value": 69.7, "ts": 1604676372},
        "qsfp_32_temp": {"value": 69.7, "ts": 1604676372},
        "macsec_1_temp1": {"value": 68.7, "ts": 1604676372},
        "macsec_1_temp2": {"value": 70.7, "ts": 1604676372},
        "macsec_2_temp1": {"value": 72.3, "ts": 1604676372},
        "macsec_2_temp2": {"value": 71.6, "ts": 1604676372},
        "macsec_3_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_3_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_4_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_4_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_5_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_5_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_6_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_6_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_7_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_7_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_8_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_8_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_9_temp1": {"value": 68.7, "ts": 1604676372},
        "macsec_9_temp2": {"value": 70.7, "ts": 1604676372},
        "macsec_10_temp1": {"value": 72.3, "ts": 1604676372},
        "macsec_10_temp2": {"value": 71.6, "ts": 1604676372},
        "macsec_11_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_11_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_12_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_12_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_13_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_13_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_14_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_14_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_15_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_15_temp2": {"value": 69.7, "ts": 1604676372},
        "macsec_16_temp1": {"value": 69.7, "ts": 1604676372},
        "macsec_16_temp2": {"value": 69.7, "ts": 1604676372},
    },
}
