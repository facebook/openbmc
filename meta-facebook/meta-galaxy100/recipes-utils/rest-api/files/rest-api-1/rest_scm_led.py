#!/usr/bin/env python
#
# Copyright 2016-present Facebook. All Rights Reserved.
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

import subprocess
LED_CMD = "/sys/class/i2c-adapter/i2c-12/12-0031/led_"


def set_led(color):
    cmd = 'echo 1 > ' + LED_CMD + color
    p = subprocess.Popen([cmd],
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = p.communicate()
    rc = p.returncode

    if rc < 0:
        status = ' failed with returncode = ' + str(rc) + ' and error ' + err
    else:
        status = ' done '

    return {"status": 'scm led set to ' + color + "=" + status}
