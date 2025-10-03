#!/bin/sh

# PLATFORM MAY CHOOSE TO OVERRIDE THIS FILE
echo -n "Starting mTerm console server..."
runsv /etc/sv/mTerm > /dev/null 2>&1 &
echo "done."
