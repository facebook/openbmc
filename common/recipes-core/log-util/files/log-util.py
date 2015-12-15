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

import re
from datetime import datetime
import sys
from ctypes import *
from lib_pal import *

SYSLOGFILE = '/mnt/data/logfile'
APPNAME = 'log-util'

def print_usage():
    print 'Usage: %s --show' % (APPNAME)

def log_main():

    if len(sys.argv) is not 2:
        print_usage()
        return -1

    if sys.argv[1] != '--show':
        print_usage()
        return -1

    fd = open(SYSLOGFILE, 'r')
    syslog = fd.readlines()
    fd.close()

    print '%-4s %-8s %-22s %-16s %s' % (
            "FRU#",
            "FRU_NAME",
            "TIME_STAMP",
            "APP_NAME",
            "MESSAGE"
            )

    for log in syslog:
        if not (re.search(r' bmc [a-z]*.crit ', log)):
            continue
        if re.search(r'FRU: [0-9]{1,2}', log, re.IGNORECASE):
            fru = ''.join(re.findall(r'FRU: [0-9]{1,2}', log, re.IGNORECASE))
            # fru is in format "FRU: X"
            fru = fru[5]

            # Name of the FRU from the PAL library
            name = pal_get_fru_name(int(fru))
        else :
            fru = '0'

            name = 'all'


        temp = re.split(r' bmc [a-z]*.crit ', log)

        # Time format Sep 28 22:10:50
        ts = temp[0]
        currtime = datetime.now()
        ts = '%d %s' % (currtime.year, ts)
        time = datetime.strptime(ts, '%Y %b %d %H:%M:%S')
        time = time.strftime('%Y-%m-%d %H:%M:%S')

        temp2 = re.split(r': ', temp[1], 1)
        app = temp2[0]
        message = temp2[1]

        print '%-4s %-8s %-22s %-16s %s' % (
                    fru,
                    name,
                    time,
                    app,
                    message
                    )


if __name__ == '__main__':
    log_main()
