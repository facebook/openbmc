#!/usr/bin/env python3

import urllib.request, urllib.error, urllib.parse
import subprocess
import syslog
import time
import ssl
import os
import os.path
import signal
from rest_config import RestConfig

listen_ssl = RestConfig.getboolean('listen', 'ssl')
port = RestConfig.getint('listen', 'port')

# it's on localhost if i can't trust it we have bigger problems.
sslctx = ssl.create_default_context()
sslctx.check_hostname = False
sslctx.verify_mode = ssl.CERT_NONE

checkurl = "http{}://localhost:{}/api".format(
    's' if listen_ssl else '', port)
interval = 30
timeout = 10
service = "restapi"
# intervals before forced kill
grace = 3
graceleft = grace

def killall(killcmd=b'python\x00/usr/local/bin/rest.py\x00'):
    try:
        pids = [pid for pid in os.listdir('/proc') if pid.isdigit()]
    except Exception as e:
        syslog.syslog(syslog.LOG_ERR,
                      "Error scanning /proc: {}".format(str(e)))
    for pid in pids:
        try:
            with open(os.path.join('/proc', pid, 'cmdline'), 'rb') as cf:
                cmdline = cf.read()
                if cmdline == killcmd:
                    syslog.syslog(syslog.LOG_ERR, 'Killing leftover rest '
                            'process: pid={} ({})...'
                            .format(pid, cmdline.replace('\x00', ' ')))
                    # would sigint but at this point we should be touching only
                    # procs that escaped runit, they're probably broken badly
                    # already anyway
                    os.kill(int(pid), signal.SIGKILL)
        except IOError:
            continue


def runit_status(svc):
    return subprocess.call(["/usr/bin/sv", "status", svc])


def runit_kill_restart(svc):
    # force-stop is SIGTERM, wait 7s, if still alive, SIGKILL
    subprocess.call(["/usr/bin/sv", "force-stop", svc])
    # kill any lingering hung forks
    killall()
    # bring it back
    subprocess.call(["/usr/bin/sv", "start", svc])


def main():
    syslog.openlog("restwatchdog")
    syslog.syslog(syslog.LOG_INFO,
                  "REST API watchdog checking %s, every %d seconds"
                  % (checkurl, interval))
    while True:
        global graceleft
        time.sleep(interval)
        status = runit_status(service)
        timeout_seen = 0;
        if status != 0:
            syslog.syslog(syslog.LOG_WARNING,
                          "REST API not running (sv status: %d)" % (status, ))
            continue
        else:
            try:
                # Send request to REST service to see if it's alive
                f = urllib.request.urlopen(checkurl, timeout=timeout,
                        context=sslctx)
                f.read()
            except Exception as ex:
                timeout_seen = 1;
                if graceleft == 1:
                    # With python3, all exceptions can be
                    # printed out as a string
                    syslog.syslog(syslog.LOG_WARNING,
                       "REST API timeout too many times : {}".format(str(ex)))
                if graceleft <= 0:
                    syslog.syslog(syslog.LOG_ERR,
                                  "Killing unresponsive REST service")
                    runit_kill_restart(service)
                    graceleft = grace
                else:
                    graceleft -= 1
            if timeout_seen == 0 :
                graceleft = grace

if __name__ == "__main__":
    main()
