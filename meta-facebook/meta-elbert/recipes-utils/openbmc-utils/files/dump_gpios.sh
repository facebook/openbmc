#!/bin/bash

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

printf "%-1s | %-3s | %-30s\n" "v" "dir" "GPIONAME"
for i in /tmp/gpionames/*
do
   gpio="$(basename "$i")"
   val="$(gpio_get_value "$gpio")"
   direction="$(gpio_get_direction "$gpio")"
   printf "%-1s | %-3s | %-30s\n" "$val" "$direction" "$gpio"
done
