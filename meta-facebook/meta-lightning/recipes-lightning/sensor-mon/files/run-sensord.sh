#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

/usr/local/bin/init_sensor.sh
exec /usr/local/bin/sensord peb pdpb fcb
