#!/bin/sh
# execute <fscd.py> <min syslog level>
# log_level = <debug, warning, info>
# Default log level in fscd is set to warning, if any other level is needed
# set here
exec /usr/bin/fscd.py 'warning'
