#!/bin/sh

echo -n "Setup sensor monitoring for FBGC... "

FRUS="uic nic"

exec /usr/local/bin/sensord $FRUS
