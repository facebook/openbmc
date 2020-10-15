#!/bin/bash

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

printf "%-1s | %-3s | %-30s\n" "v" "dir" "GPIONAME"
for i in $(ls /tmp/gpionames/):
do
   val="$(gpio_get_value "$i")"
   direction="$(gpio_get_direction "$i")"
   printf "%-1s | %-3s | %-30s\n" "$val" "$direction" "$i"
done
