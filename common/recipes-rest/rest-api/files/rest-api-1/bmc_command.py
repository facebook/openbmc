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
import os
import subprocess
import select
import sys
import time

DEFAULT_TIMEOUT = 10 #sec

# Note: Python 3.0 supports communicate() with a timeout option.
# If we upgrade to this version we will no longer need timed_communicate

class TimeoutError(Exception):
    def __init__(self, output, error):
        super(TimeoutError, self).__init__('process timed out')
        self.output = output
        self.error = error

class WaitTimeoutError(Exception):
    pass

def kill_process(proc):
    proc.terminate()
    try:
        timed_wait(proc, 0.1)
    except WaitTimeoutError:
        proc.kill()
        try:
            timed_wait(proc, 0.1)
        except WaitTimeoutError:
            # This can happen if the process is stuck waiting inside a system
            # call for a long time.  There isn't much we can do unless we want
            # to keep waiting forever.  Just give up.  The child process will
            # remain around as a zombie until we exit.
            pass

def timed_wait(proc, timeout):
    # There unfortunately isn't a great way to wait for a process with a
    # timeout, other than polling.  (Registering for SIGCHLD and sleeping might
    # be one option, but that's fragile and not thread-safe.)
    poll_interval = 0.1
    end_time = time.time() + timeout
    while True:
        if proc.poll() is not None:
            return
        time_left = max(end_time - time.time(), 0)
        if time_left <= 0:
            raise WaitTimeoutError()
        time.sleep(min(time_left, poll_interval))

def timed_communicate(proc, timeout=DEFAULT_TIMEOUT):
    end_time = time.time() + timeout

    p = select.poll()
    outfd = proc.stdout.fileno()
    errfd = proc.stderr.fileno()
    p.register(outfd, select.POLLIN)
    p.register(errfd, select.POLLIN)
    results = {outfd: [], errfd: []}
    remaining_fds = set([outfd, errfd])

    bufsize = 4096
    while remaining_fds:
        time_left = max(end_time - time.time(), 0)
        r = p.poll(time_left * 1000)  # poll() takes timeout in milliseconds
        if not r:
            kill_process(proc)
            raise TimeoutError(b''.join(results[outfd]),
                               b''.join(results[errfd]))
        for fd, flags in r:
            # We didn't put the fds in non-blocking mode, but we know the fd
            # has data to read, so os.read() will return immediately.
            d = os.read(fd, bufsize)
            if d:
                results[fd].append(d)
            else:
                # EOF on this fd, stop listening on it
                p.unregister(fd)
                remaining_fds.remove(fd)

    try:
        timed_wait(proc, max(end_time - time.time(), 0))
    except WaitTimeoutError:
        kill_process(proc)
        raise TimeoutError(b''.join(results[outfd]),
                           b''.join(results[errfd]))

    return b''.join(results[outfd]), b''.join(results[errfd])
