#!/bin/sh

#TODO: Add functions as shell's lib in /usr/local/fbpackages/utils/ast-functions#
echo "Setup sensor monitoring for NETLAKEMTP... "

FRUS="server bmc pdb fio"

exec /usr/local/bin/sensord $FRUS
