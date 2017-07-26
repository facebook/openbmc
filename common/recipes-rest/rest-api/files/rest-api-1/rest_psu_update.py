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
from subprocess import Popen
import os
import os.path
import json
import uuid
import urllib.request, urllib.error, urllib.parse
from tempfile import mkstemp
from aiohttp import web


UPDATE_JOB_DIR = '/var/rackmond/update_jobs'
UPDATERS = {'delta': '/usr/local/bin/psu-update-delta.py',
            'belpower': '/usr/local/bin/psu-update-bel.py'}

def get_jobs():
    jobs = []
    if not os.path.exists(UPDATE_JOB_DIR):
        os.makedirs(UPDATE_JOB_DIR)
    for f in os.listdir(UPDATE_JOB_DIR):
        fullpath = os.path.join(UPDATE_JOB_DIR, f)
        if f.endswith('.json') and os.path.isfile(fullpath):
            with open(fullpath, 'r') as fh:
                jdata = json.load(fh)
                jdata['job_id'] = os.path.splitext(f)[0]
                jobs.append(jdata)
    return {'jobs': jobs}

updater_process = None
def begin_job(jobdesc):
    global updater_process
    if updater_process is not None:
        if updater_process.poll() is not None:
            # Update complete
            updater_process = None
        else:
            error_body = {'error': 'update_already_running',
                    'pid': updater_process.pid }
            raise web.HTTPConflict(body=error_body)
    job_id = str(uuid.uuid1())
    (fwfd, fwfilepath) = mkstemp()
    if not os.path.exists(UPDATE_JOB_DIR):
        os.makedirs(UPDATE_JOB_DIR)
    statusfilepath = os.path.join(UPDATE_JOB_DIR, str(job_id) + '.json')
    status = {'pid': 0,
              'state': 'fetching' }
    with open(statusfilepath, 'wb') as sfh:
        sfh.write(json.dumps(status))
    try:
        fwdata = urllib.request.urlopen(jobdesc['fw_url'])
        with os.fdopen(fwfd, 'wb') as fwfile:
            fwfile.write(fwdata.read())
            fwfile.flush()
    except:
        os.remove(statusfilepath)
        raise

    updater = UPDATERS[jobdesc.get('updater', 'delta')]
    updater_process = Popen([updater,
               '--addr', str(jobdesc['address']),
               '--statusfile', statusfilepath,
               '--rmfwfile',
               fwfilepath])
    status = {'pid': updater_process.pid,
              'state': 'starting' }
    with open(statusfilepath, 'wb') as sfh:
        sfh.write(json.dumps(status))
    return {'job_id': job_id, 'pid': updater_process.pid}
