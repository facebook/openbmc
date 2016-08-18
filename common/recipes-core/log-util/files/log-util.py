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
import os
from ctypes import *
from lib_pal import *

syslogfiles = ['/mnt/data/logfile.0', '/mnt/data/logfile']
cmdlist = ['--print', '--clear']
APPNAME = 'log-util'
frulist = ''

def print_usage():
    global frulist

    print 'Usage: %s [ %s ] %s' % (APPNAME, ' | '.join(frulist), cmdlist[0])
    print '       %s [ %s ] %s' % (APPNAME, ' | '.join(frulist), cmdlist[1])


def log_main():

    global frulist

    # Get the list of frus from PAL library
    frus = pal_get_fru_list()
    frulist = re.split(r',\s', frus)

    if len(sys.argv) is not 3:
        print_usage()
        return -1

    fru = sys.argv[1]
    cmd = sys.argv[2]

    # Check if the fru passed in as argument exists in the fru list
    if fru not in frulist:
        print "Error: Fru not in the list [ %s ] \n" % ' | '.join(frulist)
        print_usage()
        return -1

    # Check if the cmd passed in as argument exists in the cmd list
    if cmd not in cmdlist:
        print "Unknown command: %s \n" % cmd
        print_usage()
        return -1

    # Print cmd
    if cmd == cmdlist[0]:
        print '%-4s %-8s %-22s %-16s %s' % (
            "FRU#",
            "FRU_NAME",
            "TIME_STAMP",
            "APP_NAME",
            "MESSAGE"
            )

    for logfile in syslogfiles:

        try:
            fd = open(logfile, 'r')
            syslog = fd.readlines()
            fd.close()
        except IOError:
            continue

        # Clear cmd
        if cmd == cmdlist[1]:

            newlog = ''

            for log in syslog:
                # Print only critical logs
                if not (re.search(r' bmc [a-z]*.crit ', log)):
                    continue

                # Find the FRU number
                if re.search(r'FRU: [0-9]{1,2}', log, re.IGNORECASE):
                    fru_num = ''.join(re.findall(r'FRU: [0-9]{1,2}', log, re.IGNORECASE))
                    # FRU is in format "FRU: X"
                    fru_num = fru_num[5]
                else:
                    fru_num = '0'

                # FRU # is always aligned with indexing of fru list
                fruname = frulist[int(fru_num)]

                # Clear the log is the argument fru matches the log fru
                if fru == 'all' or fru == fruname:
                    # Drop this log line
                    continue
                else:
                    newlog = newlog + log

            # Dump the new log in a tmp file
            tmpfd = open('%s.tmp' % logfile, 'w')
            tmpfd.write(newlog)
            tmpfd.close()
            # Rename the tmp file to original syslog file
            os.rename('%s.tmp' % logfile, logfile)

        # Print cmd
        if cmd == cmdlist[0]:

            for log in syslog:
                # Print only critical logs
                if not (re.search(r' bmc [a-z]*.crit ', log)):
                    continue

                # Find the FRU number
                if re.search(r'FRU: [0-9]{1,2}', log, re.IGNORECASE):
                    fru_num = ''.join(re.findall(r'FRU: [0-9]{1,2}', log, re.IGNORECASE))
                    # FRU is in format "FRU: X"
                    fru_num = fru_num[5]
                else:
                    fru_num = '0'

                    # FRU # is always aligned with indexing of fru list
                fruname = frulist[int(fru_num)]

                # Print only if the argument fru matches the log fru
                if fru != 'all' and fru != fruname:
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
                message = temp2[1].rstrip('\n')

                print '%-4s %-8s %-22s %-16s %s' % (
                    fru_num,
                    fruname,
                    time,
                    app,
                    message
                    )

    if cmd == cmdlist[1]:
       pal_log_clear(fru)

if __name__ == '__main__':
    log_main()
