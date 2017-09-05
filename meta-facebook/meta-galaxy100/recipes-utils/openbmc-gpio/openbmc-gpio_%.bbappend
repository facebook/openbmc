# Copyright 2014-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "\
            file://openbmc-gpio-1/board_gpio_table_v2.py \
            file://openbmc-gpio-1/board_gpio_table_v1.py \
            "

OPENBMC_GPIO_SOC_TABLE = "ast2400_gpio_table.py"

