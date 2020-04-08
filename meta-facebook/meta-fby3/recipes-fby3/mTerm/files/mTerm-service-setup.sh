#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions
BOARD_ID=$(get_bmc_board_id)

# PLATFORM MAY CHOOSE TO OVERRIDE THIS FILE
echo -n "Starting mTerm console server..."
runsv /etc/sv/mTerm1 > /dev/null 2>&1 &

if [ $BOARD_ID -eq 14 ] || [ $BOARD_ID -eq 7 ]; then
  runsv /etc/sv/mTerm2 > /dev/null 2>&1 &
  runsv /etc/sv/mTerm3 > /dev/null 2>&1 &
  runsv /etc/sv/mTerm4 > /dev/null 2>&1 &
fi

echo "done."
