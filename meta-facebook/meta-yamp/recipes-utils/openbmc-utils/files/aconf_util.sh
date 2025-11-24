#!/bin/bash

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

trap cleanup INT TERM QUIT EXIT

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program show"
    echo "$program [-f <conf-file>] program [(bmc|cpu)]"
    echo "      <conf-file> : custom conf-file with name=value entries"
    echo "      bmc: program using conf-file $BMC_CONF_FILE"
    echo "      cpu: program using conf-file $CPU_CONF_FILE"
    echo "      By default, the bmc conf-file is used"
}

# Temp file for storing constructed aconf image.
TEMP_ACONF_IMAGE="/tmp/tmp_aconfutil_aconf_image"

cleanup() {
    rm -f $TEMP_ACONF_IMAGE
}

narg_err() {
    echo "Invalid number of arguments"
    usage
    exit 1
}

conf_file=""
while getopts "f:p:" opt; do
    case $opt in
        f)
            conf_file="$OPTARG"
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done
shift $((OPTIND-1))

case "${1^^}" in
    SHOW)
        if [ $# -ne 1 ]; then
            narg_err
        fi
        bios_util.sh read "$TEMP_ACONF_IMAGE" --partition aboot_conf || exit 1
        echo "Contents of aboot_conf:"
        parse_aboot_conf "$TEMP_ACONF_IMAGE"
        ;;
    PROGRAM)
        if [ $# -eq 1 ]; then
            if [ -z "$conf_file" ]; then
                conf_file=$BMC_CONF_FILE
            fi
        elif [ $# -eq 2 ]; then
            if [ "$2" == "bmc" ]; then
                conf_file=$BMC_CONF_FILE
            elif [ "$2" == "cpu" ]; then
                conf_file=$CPU_CONF_FILE
            else
                echo "Invalid configuration: $2"
                usage
                exit 1
            fi
        else
            narg_err
        fi

        create_aboot_conf_image "$conf_file" /dev/zero "$TEMP_ACONF_IMAGE" || exit 1
        bios_util.sh write "$TEMP_ACONF_IMAGE" --partition aboot_conf
        ;;
    *)
        echo "Unknown action: $1"
        usage
        ;;
esac
