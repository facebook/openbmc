#!/bin/sh

while true; do
    pid=$(pidof pldmd)

    if [ -z "$pid" ]; then
        sleep 60
        continue
    fi

    total_memory=$(free | awk 'NR==2 {print $2}')
    free_memory=$(free | awk 'NR==2 {print $4}')
    free_memory_percentage=$(awk "BEGIN {printf \"%.0f\", ($free_memory / $total_memory) * 100}")

    if [ "$free_memory_percentage" -le 20 ]; then
        systemctl restart pldmd
    fi

    sleep 60
done

