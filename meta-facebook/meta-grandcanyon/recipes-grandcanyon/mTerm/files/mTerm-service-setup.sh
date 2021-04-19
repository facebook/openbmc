#!/bin/sh

# PLATFORM MAY CHOOSE TO OVERRIDE THIS FILE
echo -n "Starting mTerm console server..."
runsv /etc/sv/mTerm-scc_exp_smart > /dev/null 2>&1 &
runsv /etc/sv/mTerm-scc_exp_sdb > /dev/null 2>&1 &
runsv /etc/sv/mTerm-scc_ioc_smart > /dev/null 2>&1 &
runsv /etc/sv/mTerm-server > /dev/null 2>&1 &
runsv /etc/sv/mTerm-bic > /dev/null 2>&1 &
runsv /etc/sv/mTerm-iocm_ioc_smart > /dev/null 2>&1 &

echo "done."
