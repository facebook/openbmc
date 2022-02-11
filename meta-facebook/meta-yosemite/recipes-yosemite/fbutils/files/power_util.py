#!/usr/bin/env python
#
# Copyright 2015-present Facebook. All Rights Reserved.
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

import logging
import os
import re
import time
from ctypes import CDLL, Structure, c_char, pointer


POR_DIR = "/mnt/data/power/por"
POR_CONFIG = "%s/config" % POR_DIR
POR_LPS = "%s/last_state" % POR_DIR

logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)


class PORConfig:
    on = "1"  # Default ON
    off = "2"  # Default OFF
    lps = "3"  # Default is to use the Last Power State


# Handler for Bridge IC libraries
bic = CDLL("libbic.so.0")


class BIC_GPIO(Structure):
    _fields_ = [("bic_gpio_data", c_char * 4)]


# Get 32-bit GPIO data
def get_bic_gpio():
    gpio = BIC_GPIO()
    p_gpio = pointer(gpio)
    bic.bic_get_gpio(p_gpio)
    return gpio


# Get the CPU power status
def get_pwr_cpu():
    gpio = get_bic_gpio()
    pwrgood_cpu = ord(gpio.bic_gpio_data[0]) & 0x01
    return pwrgood_cpu


# Initilize the POR configuration files in /mnt/data
def init_por():

    por = PORConfig()

    # For the Power On Reset Config
    if not os.path.isfile(POR_CONFIG):
        try:
            os.makedirs(POR_DIR)
        except OSError:
            pass

        por_cnfg = open(POR_CONFIG, "w")
        por_cnfg.write("%s\n" % por.on)
        por_cnfg.close()

    # For the Last Power State info
    if not os.path.isfile(POR_LPS):
        curr_time = int(time.time())
        lps = "on %s" % str(curr_time)

        f_lps = open(POR_LPS, "w")
        f_lps.write("%s\n" % lps)
        f_lps.close()


# Get the POR config [ ON | OFF | LPS ]
def get_por_config():

    por = PORConfig()

    if os.path.isfile(POR_CONFIG):
        por_cnfg = open(POR_CONFIG, "r")
        cnfg = por_cnfg.read(1)

        if cnfg in [por.on, por.off, por.lps]:
            return cnfg
        else:
            return 0
    else:
        return -1


# To check whether the last power state was on or off
def get_por_lps():

    if os.path.isfile(POR_LPS):
        f_lps = open(POR_LPS, "r")
        lps = f_lps.readline()
        if re.search(r"on", lps):
            return 1
        elif re.search(r"off", lps):
            return 0
    else:
        return -1


# This tells whether to Power ON or not on POR
# 1 - Power ON
# 0 - Do not Power ON
def por_policy():

    por = PORConfig()
    cnfg = get_por_config()
    if cnfg < 1:
        log.error("power_util: Error getting the POR config.")
        exit(-1)

    if cnfg == por.on:
        # cpu power ON
        log.debug("ON: Powering ON")
        return 1

    elif cnfg == por.off:
        # cpu power OFF
        log.debug("OFF: Powering OFF")
        return 0

    elif cnfg == por.lps:
        lps = get_por_lps()
        if lps < 0:
            log.error("power_util: Error getting the POR Last State.")
            exit(-1)

        if lps == 1:
            # cpu power ON
            log.debug("LPS: Powering ON")
            return 1

        elif lps == 0:
            # cpu power OFF
            log.debug("LPS: Powering OFF")
            return 0


if __name__ == "__main__":
    main()
