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
import argparse
import os
from ctypes import *
from lib_pal import *

syslogfiles = ['/mnt/data/logfile.0', '/mnt/data/logfile']
APPNAME = 'log-util'

def print_usage(parser):
    parser.print_help()


def log_main():

    # Get the list of frus from PAL library
    frus = pal_get_fru_list()
    frulist = re.split(r',\s', frus)

    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--clear', action='store', dest='clearfru',
            help='Clear the log of particular fru [%s]' % frus)
    parser.add_argument('-p', '--print', action='store', dest='printfru',
            help='Print the event logs for a particular fru [%s]' % frus)
    args = parser.parse_args()

    # Atleast one of the two command (print or clear) should be passed
    if args.clearfru == None and args.printfru == None:
        print_usage(parser)
        return -1

    # Both print and clear commands cannot co-exist
    if args.clearfru != None and args.printfru != None:
        print_usage(parser)
        return -1

    # Check if the fru passed in as argument exists in the fru list
    if (args.clearfru in frulist) == (args.printfru in frulist):
        print "Error: Fru not in the list [%s]" % frus
        return -1

    if args.printfru is not None:
        print '%-4s %-8s %-22s %-16s %s' % (
            "FRU#",
            "FRU_NAME",
            "TIME_STAMP",
            "APP_NAME",
            "MESSAGE"
            )

    for file in syslogfiles:

        fd = open(file, 'r')
        syslog = fd.readlines()
        fd.close()

        if args.clearfru is not None:

            newlog = ''

            for log in syslog:
                # Print only critical logs
                if not (re.search(r' bmc [a-z]*.crit ', log)):
                    continue

                # Find the FRU number
                if re.search(r'FRU: [0-9]{1,2}', log, re.IGNORECASE):
                    fru = ''.join(re.findall(r'FRU: [0-9]{1,2}', log, re.IGNORECASE))
                    # FRU is in format "FRU: X"
                    fru = fru[5]
                    # FRU # is always aligned with indexing of fru list
                    name = frulist[int(fru)]

                # Clear the log is the argument fru matches the log fru
                if args.clearfru == 'all' or args.clearfru == name:
                    # Drop this log line
                    continue
                else:
                    newlog = newlog + log

            # Dump the new log in a tmp file
            tmpfd = open('%s.tmp' % file, 'w')
            tmpfd.write(newlog)
            tmpfd.close()
            # Rename the tmp file to original syslog file
            os.rename('%s.tmp' % file, file)

        if args.printfru is not None:

            for log in syslog:
                # Print only critical logs
                if not (re.search(r' bmc [a-z]*.crit ', log)):
                    continue

                # Find the FRU number
                if re.search(r'FRU: [0-9]{1,2}', log, re.IGNORECASE):
                    fru = ''.join(re.findall(r'FRU: [0-9]{1,2}', log, re.IGNORECASE))
                    # FRU is in format "FRU: X"
                    fru = fru[5]
                    # FRU # is always aligned with indexing of fru list
                    name = frulist[int(fru)]

                # Print only if the argument fru matches the log fru
                if args.printfru != 'all' and args.printfru != name:
                    continue

                # Time format Sep 28 22:10:50
                temp = re.split(r' bmc [a-z]*.crit ', log)
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
