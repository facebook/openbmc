#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup sensor monitoring for minipack... "

# Check for the slots present and run sensord for those slots only.
SLOTS="scm"

exec /usr/local/bin/sensord $SLOTS
