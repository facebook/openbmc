#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

ARGS="mb nic riser_slot2 riser_slot3 riser_slot4"
exec /usr/local/bin/sensord $ARGS
