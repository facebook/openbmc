#!/bin/bash

# Copyright 2022-present Facebook. All Rights Reserved.

# shellcheck disable=SC2034

wedge_iso_buf_enable() {
    return 0
}

wedge_iso_buf_disable() {
    return 0
}


wedge_is_us_on() {
    return 0
}

wedge_board_type() {
    echo 'FBDARWIN'
}

wedge_slot_id() {
    printf "1\n"
}

wedge_board_rev() {
    # Assume P1
    return 1
}

wedge_should_enable_oob() {
    return 1
}

retry_command() {
    # Retry command up to $1 attempts
    local retries=$1
    shift

    local count=0
    until "$@"; do
        exit=$?
        count=$((count+1))
        if [ "$count" -lt "$retries" ]; then
            msg="Attempt $count/$retries failed with $exit, retrying..."
            echo "$msg"
            logger retry_command: "$msg"
        else
            msg="Retry $count/$retries failed with $exit, no more retries left"
            echo "$msg"
            logger retry_command: "$msg"
            return $exit
        fi
    done
    return 0
}
