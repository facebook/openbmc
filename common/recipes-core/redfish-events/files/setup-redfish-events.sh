#!/bin/sh
runsv /etc/sv/redfishevents >/dev/null 2>&1 &
# delay 0.5 seconds to avoid suprising error print like below:
# warning: restapi: can't open supervise/control: No such file or directory
# root casue is runit daemon is not ready.
if [ -n "$1" ]; then
    (sleep 0.5; sv "$1" redfishevents) &
fi
