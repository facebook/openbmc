#!/bin/sh

# shellcheck source=/usr/local/fbpackages/utils/ast-functions
. /usr/local/fbpackages/utils/ast-functions

echo "Setup sensor monitoring for FBGC... "

FRUS="server uic nic e1s_iocm"

exec /usr/local/bin/sensord $FRUS
