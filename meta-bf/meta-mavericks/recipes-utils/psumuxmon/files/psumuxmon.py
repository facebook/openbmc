#!/usr/bin/env python3
#
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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
import subprocess
import time
import syslog

pcard_vin = "/sys/class/i2c-adapter/i2c-7/7-006f/in1_input"
mux_bus = 7
mux_addr = 0x70
check_interval = 600


def pcard_read(inp):
    try:
        with open(inp, 'r') as f:
            val = int(f.read())
            return val
    except:
        return None


def set_mux_channel(ch):
    cmd = ["/usr/sbin/i2cset", "-f", "-y", str(mux_bus), str(mux_addr), str(ch)]
    subprocess.call(cmd)


def pcard_read_channel(ch, inp):
    set_mux_channel(ch)
    return pcard_read(inp)


def main():
    syslog.openlog("psumuxmon")
    pcard_channel = None
    while True:
        live_channels = []
        for ch in [0x1, 0x2]:
            vin = pcard_read_channel(ch, pcard_vin)
            if vin:
                live_channels.append(ch)
        if len(live_channels) == 0:
            syslog.syslog(syslog.LOG_WARNING, "No ltc4151 (passthrough card voltage monitor) detected")
            pcard_channel = None
        if len(live_channels) == 1:
            ch = live_channels[0]
            if pcard_channel is None:
                syslog.syslog(syslog.LOG_INFO, "Passsthrough card ltc4151 detected on mux channel %d" % (ch,))
                pcard_channel = ch
                set_mux_channel(pcard_channel)
        if len(live_channels) == 2:
            for ch in live_channels:
                if pcard_channel != ch:
                    pcard_channel = ch
            syslog.syslog(syslog.LOG_INFO,
                    "ltc4151 detected on both mux channels, on channel %d for next %d seconds..." \
                    % (pcard_channel, check_interval))
            set_mux_channel(pcard_channel)
        time.sleep(check_interval)


if __name__ == "__main__":
    main()
