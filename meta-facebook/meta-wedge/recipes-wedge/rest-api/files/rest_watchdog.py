#!/usr/bin/env python
import urllib2
import subprocess
import syslog
import time
import ssl
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
timeout = 5
service = "restapi"
# intervals before forced kill
grace = 1
graceleft = grace

def runit_status(svc):
    return subprocess.call(["/usr/bin/sv", "status", svc])

def runit_kill_restart(svc):
    # force-stop is SIGTERM, wait 7s, if still alive, SIGKILL
    subprocess.call(["/usr/bin/sv", "force-stop", svc])
    # bring it back
    subprocess.call(["/usr/bin/sv", "start", svc])

def main():
    syslog.openlog("restwatchdog")
    syslog.syslog(syslog.LOG_INFO,
            "REST API watchdog checking %s, every %d seconds" \
            % (checkurl, interval))
    while True:
        time.sleep(interval)
        status = runit_status(service)
        if status != 0:
            syslog.syslog(syslog.LOG_WARNING,
                    "REST API not running (sv status: %d)" % (status, ))
            continue
        else:
            try:
                f = urllib2.urlopen(checkurl, timeout=timeout, context=sslctx)
                f.read()
            except:
                global graceleft
                syslog.syslog(syslog.LOG_WARNING, "REST API not responding")
                if graceleft <= 0:
                    syslog.syslog(syslog.LOG_ERR, "Killing unresponsive REST service")
                    runit_kill_restart(service)
                    graceleft = grace
                else:
                    graceleft -= 1

if __name__ == "__main__":
    main()
