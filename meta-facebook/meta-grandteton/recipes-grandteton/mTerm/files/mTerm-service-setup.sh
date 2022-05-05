#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions
echo "Starting mTerm console server..."
runsv /etc/sv/mTerm1 > /dev/null 2>&1 &
runsv /etc/sv/mTerm2 > /dev/null 2>&1 &
echo "done."
