#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions
echo "Starting mTerm console server..."
runsv /etc/sv/mTerm1 > /dev/null 2>&1 &
runsv /etc/sv/mTerm2 > /dev/null 2>&1 &
runsv /etc/sv/mTerm3 > /dev/null 2>&1 &

for i in {1..12}
do
    for j in {1..2}
    do
        runsv "/etc/sv/mTerm"$i"_"$j"" 2>&1 &
    done
done

echo "done."
