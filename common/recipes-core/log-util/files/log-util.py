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
import subprocess
import codecs
import time
from threading import Thread

syslogfiles = ['/mnt/data/logfile.0', '/mnt/data/logfile']
cmdlist = ['--print', '--clear']
APPNAME = 'log-util'
frulist = ''
filelock = '/tmp/log-util.lock'

def print_usage():
    global frulist

    print('Usage: %s [ %s ] %s' % (APPNAME, ' | '.join(frulist), cmdlist[0]))
    print('       %s [ %s ] %s' % (APPNAME, ' | '.join(frulist), cmdlist[1]))

def rsyslog_hup():
    try:
        pid = subprocess.check_output(['pidof', 'rsyslogd']).decode().strip()
        if re.match(r'^\d+$', pid):
            cmd = ['kill', '-HUP', pid]
            subprocess.check_call(cmd)
    except (OSError, IOError, subprocess.CalledProcessError) as e:
        pass

def log_main():

    global frulist
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, 'strict')

    # Get the list of frus from PAL library
    frus = pal_get_fru_list().decode()
    frulist = re.split(r',\s', frus)
    frulist.append('sys')

    if len(sys.argv) is not 3:
        print_usage()
        return -1

    fru = sys.argv[1]
    cmd = sys.argv[2]

    # Check if the fru passed in as argument exists in the fru list
    if fru not in frulist:
        print("Error: Fru not in the list [ %s ] \n" % ' | '.join(frulist))
        print_usage()
        return -1

    # Check if the cmd passed in as argument exists in the cmd list
    if cmd not in cmdlist:
        print("Unknown command: %s \n" % cmd)
        print_usage()
        return -1

    # Print cmd
    if cmd == cmdlist[0]:
        print('%-4s %-8s %-22s %-16s %s' % (
            "FRU#",
            "FRU_NAME",
            "TIME_STAMP",
            "APP_NAME",
            "MESSAGE"
            ))

    if cmd == cmdlist[1]:
        retry = 5
        while retry!=0:
            # acquire: open file for write, create if possible, exclusive access guaranteed
            try:
                fdlock = os.open(filelock, os.O_CREAT|os.O_WRONLY|os.O_EXCL)
                break
            except OSError:
                # failed to open, another process is running
                print("log_util: another instance is running...\n")
                retry = retry - 1
                time.sleep(5)
                continue

    for logfile in syslogfiles:

        try:
            fd = open(logfile, 'a+', encoding='utf-8')
            fd.seek(0, os.SEEK_SET)
            syslog = fd.readlines()
            fd.close()
        except Exception:
            print("Unexpected error:", sys.exc_info()[0])
            continue

        # Clear cmd
        if cmd == cmdlist[1]:

            newlog = ''

            for log in syslog:
                # Print only critical logs
                if not (re.search(r'[a-z]*.crit ', log) or re.search(r'log-util:', log)):
                    continue

                # Find the FRU number
                if re.search(r'FRU: [0-9]{1,2}', log, re.IGNORECASE):
                    fru_num = ''.join(re.findall(r'FRU: [0-9]{1,2}', log, re.IGNORECASE))
                    # FRU is in format "FRU: X"
                    fru_num = fru_num[5]
                else:
                    fru_num = '0'

                # FRU # is always aligned with indexing of fru list
                if fru == 'sys' and fru_num == '0':
                    fruname = 'sys'
                else:
                    fruname = frulist[int(fru_num)]

                # Clear the log is the argument fru matches the log fru
                if fru == 'all' or fru == fruname:
                    # Drop this log line
                    continue
                else:
                    newlog = newlog + log

            # Dump the new log in a tmp file
            if logfile == syslogfiles[1]:
               if fru == 'all':
                  temp = 'all'
               else:
                  if fru == 'sys':
                     temp = 'sys'
                  else:
                     fru_num = str(frulist.index(fru))
                     temp = 'FRU: ' + fru_num
               curtime = datetime.now()
               newlog = newlog + curtime.strftime('%Y %b %d %H:%M:%S') + ' log-util: User cleared ' + temp + ' logs\n'
            curpid = os.getpid()
            tmpfd = open('%s.tmp%d' % (logfile, curpid), 'w')
            tmpfd.write(newlog)
            tmpfd.close()
            # Rename the tmp file to original syslog file
            os.rename('%s.tmp%d' % (logfile, curpid), logfile)

        # Print cmd
        if cmd == cmdlist[0]:

            for log in syslog:
                # log eg: 2017 Nov 21 21:46:09 rtptest1413-oob.prn3.facebook.com user.crit fbttn-c279551: power-util: SERVER_POWER_CYCLE successful for FRU: 1
                #         =date==========  =hostname======================= =loglevel= =version===== =appname=  =message===========================
                # Print only critical logs
                if not (re.search(r' [a-z]*.crit ', log) or re.search(r'log-util:', log)):
                    continue

                # Find the FRU number
                if re.search(r'FRU: [0-9]{1,2}', log, re.IGNORECASE):
                    fru_num = ''.join(re.findall(r'FRU: [0-9]{1,2}', log, re.IGNORECASE))
                    # FRU is in format "FRU: X"
                    fru_num = fru_num[5]
                else:
                    fru_num = '0'

                # FRU # is always aligned with indexing of fru list
                if fru == 'sys' and fru_num == '0':
                    fruname = 'sys'
                else:
                    fruname = frulist[int(fru_num)]

                # Print only if the argument fru matches the log fru
                if re.search(r'log-util:', log):
                   if re.search(r'all logs', log):
                     print (log)
                     continue

                if fru != 'all' and fru != fruname:
                    continue

                if re.search(r'log-util:', log):
                     print (log)
                     continue


                tmp = log.split()
                if len(tmp[0]) is 4 and re.match(r'[0-9]{4}', tmp[0]) is not None:
                    # Time format 2017 Sep 28 22:10:50
                    ts= ' '.join(tmp[0:4])
                    curtime = datetime.strptime(ts, '%Y %b %d %H:%M:%S')
                    curtime = curtime.strftime('%Y-%m-%d %H:%M:%S')
                else:
                    # Time format Sep 28 22:10:50
                    ts= ' '.join(tmp[0:3])
                    curtime = datetime.strptime(ts, '%b %d %H:%M:%S')
                    curtime = curtime.strftime('%m-%d %H:%M:%S')
                    tmp[1:] = log.split()

                # Hostname
                hostname = tmp[4]

                # OpenBMC Version Information
                version = tmp[6]

                # Application Name
                app = tmp[7].strip(':')

                # Log Message
                message = ' '.join(tmp[8:]).rstrip('\n')

                print('%-4s %-8s %-22s %-16s %s' % (
                    fru_num,
                    fruname,
                    curtime,
                    app,
                    message
                    ))

    if cmd == cmdlist[1]:
        pal_log_clear(fru)
        rsyslog_hup()
        os.close(fdlock)
        # release: delete file
        os.remove(filelock)

if __name__ == '__main__':
    
    run = Thread(target=log_main)
    run.start()
    run.join()
