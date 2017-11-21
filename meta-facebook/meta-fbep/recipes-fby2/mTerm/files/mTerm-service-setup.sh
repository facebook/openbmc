#!/bin/sh

# PLATFORM MAY CHOOSE TO OVERRIDE THIS FILE
echo -n "Starting mTerm console server..."
runsv /etc/sv/mTerm1 > /dev/null 2>&1 &
runsv /etc/sv/mTerm2 > /dev/null 2>&1 &
runsv /etc/sv/mTerm3 > /dev/null 2>&1 &
runsv /etc/sv/mTerm4 > /dev/null 2>&1 &
echo "done."
