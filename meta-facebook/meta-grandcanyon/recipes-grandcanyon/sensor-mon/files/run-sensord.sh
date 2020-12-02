#!/bin/sh

echo -n "Setup sensor monitoring for FBGC... "

FRUS="server uic nic e1s_iocm"

exec /usr/local/bin/sensord $FRUS
