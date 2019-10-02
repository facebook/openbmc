#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions

ARGS="mb nic0 nic1"
exec /usr/local/bin/sensord $ARGS
