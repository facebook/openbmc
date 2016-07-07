#!/usr/bin/env python
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

import argparse
import urllib2
import time
import math

parser = argparse.ArgumentParser(description = 'Measure latency \
                                                of service-based IPC.')
parser.add_argument('--i', nargs='?', const=1, type=int, default=1)
args = parser.parse_args()

# function to get the current time in microsecond
get_time = lambda: int(round(time.time() * 1000000))

it_num = args.i # iteration number
time_diff = [0 for x in range(0, it_num)] # record the difference of time

# keep pinging the service through the IPC server
# measure the roundtrip latency
for x in range(0, it_num):
    timestamp1 = get_time()
    urllib2.urlopen("http://localhost:8043/ipc/service0").read()
    timestamp2 = get_time()
    time_diff[x] = float(timestamp2 - timestamp1)

avg_latency = float(sum(time_diff))/float(it_num)
N = it_num - 1 if it_num > 1 else 1
latency_std = \
  math.sqrt(float(sum([math.pow(t - avg_latency, 2) for t in time_diff])) \
            / float(N))
print "iteration number = ", it_num
print "average latency = ", avg_latency
print "latency std = ", latency_std
