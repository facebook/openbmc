#!/bin/sh

FILE=$1
FAN=$2
cat $FILE | head -n $FAN | tail -n 1
