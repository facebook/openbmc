#!/bin/sh
. /usr/local/bin/board-utils.sh

if wedge_is_us_on; then
    echo 1
else
    echo 0
fi
