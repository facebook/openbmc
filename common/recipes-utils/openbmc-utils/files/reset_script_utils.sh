#!/bin/bash

DEFAULT_WARNING_MSG="This operation is potentially disruptive."

function warn_and_confirm {
    # Params
    # 1: "force" Set true if -f or --force was passed into the main script
    # 2: Optional warning message to override the default message

    local force="${1:-"false"}"
    local message="${2:-$DEFAULT_WARNING_MSG}"

    if [[ "$force" == "true" ]]; then
        return 0
    fi

    echo "Warning: $message"
    read -r -p "Do you wish to continue? [y/N]: " user_input
    if [ "$user_input" != "y" ] && [ "$user_input" != "Y" ] ; then
        exit 0
    fi
}

function check_force_arg {
    # Params
    # 1: Pass in "$@" from the main script
    for arg in $1
    do
        if [[ "$arg" == "-f" || "$arg" == "--force" ]]; then
            echo "true"
        else
            echo "false"
        fi
    done
}
