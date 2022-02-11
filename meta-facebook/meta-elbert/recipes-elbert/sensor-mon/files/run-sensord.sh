#!/bin/sh
#shellcheck disable=SC1091
. /usr/local/fbpackages/utils/ast-functions

ARGS="scm smb pim2 pim3 pim4 pim5 pim6 pim7 pim8 pim9 psu1 psu2 psu3 psu4 fan"
#shellcheck disable=SC2086
exec /usr/local/bin/sensord $ARGS
