#!/bin/bash

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

trap cleanup INT TERM QUIT EXIT

BMC_CONF_FILE=/etc/bmc_aboot.conf
CPU_CONF_FILE=/etc/cpu_aboot.conf

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program show"
    echo "$program [-f <conf-file>] [-p <product-name> ] program [(bmc|cpu)]"
    echo "      <conf-file> : custom conf-file with name=value entries"
    echo "      <product-name> : product-name to encode. By default, the product"
    echo "                       name in the flash is preserved"
    echo "      bmc: program using conf-file $BMC_CONF_FILE"
    echo "      cpu: program using conf-file $CPU_CONF_FILE"
    echo "      By default, the bmc conf-file is used"
}

ABOOT_CONF_START=$(get_section_start aboot_conf)
FLASH_SIZE=$(get_total_size)
ABOOT_CONF_SIZE=$(get_section_size aboot_conf)
SECTION_BLOCK_SIZE=$((0x1000))

BOARD_NAME_VAR="DMI_BOARD_NAME"
BOARD_VER_VAR="DMI_BOARD_VERSION"

# Temp file for storing constructed bios image.
TEMP_BIOS_IMAGE="/tmp/tmp_bios_image"

cleanup() {
    rm -f $TEMP_BIOS_IMAGE
}

narg_err() {
    echo "Invalid number of arguments"
    usage
    exit 1
}

get_old_product_name() {
    rm -f "$TEMP_BIOS_IMAGE"
    bios_util.sh read "$TEMP_BIOS_IMAGE" --partition aboot_conf \
                 > /dev/null 2> /dev/null || exit 1
    parse_aboot_conf "$TEMP_BIOS_IMAGE"|grep $BOARD_NAME_VAR| \
                     awk -F '=' '{print $2}'
}

get_product_version() {
    local version
    local sub_version

    version=$(weutil -e scm|grep "Product Version"|awk '{print $3}')
    sub_version=$(weutil -e scm|grep "Product Sub-Version"|awk '{print $3}')
    echo "${version}.${sub_version}"
}

create_aboot_conf_image() {
    local conf_file="$1"
    local product_name="$2"

    echo "Using Aboot conf file: $conf_file"

    if [ ! -f "$conf_file" ]; then
        echo "File not found: $conf_file" >&2
        return 1
    fi

    if [ -z "$product_name" ]; then
        product_name=$(get_old_product_name)
    fi

    rm -f "$TEMP_BIOS_IMAGE"

    pad_blocks="$((ABOOT_CONF_START / SECTION_BLOCK_SIZE))"
    dd if=/dev/zero of="$TEMP_BIOS_IMAGE" bs="$SECTION_BLOCK_SIZE" \
       count="$pad_blocks" 2> /dev/null

    awk -F '=' '/^[^#\s]+=\S+/ {print $1 " " $2}' "$1" | while : ; do
        read -r name val
        if [ -z "$name" ]; then
            break
        fi
        echo -n "${name}=" >> "$TEMP_BIOS_IMAGE"
        echo -n "${val}" | base64 >> "$TEMP_BIOS_IMAGE"
    done

    if [ -n "$product_name" ]; then
        echo -n "${BOARD_NAME_VAR}=" >> "$TEMP_BIOS_IMAGE"
        echo -n "${product_name}" | base64 >> "$TEMP_BIOS_IMAGE"
    else
        echo "Warning: no product name encoded" >&2
    fi

    product_version=$(get_product_version)
    echo -n "${BOARD_VER_VAR}=" >> "$TEMP_BIOS_IMAGE"
    echo -n "${product_version}" | base64 >> "$TEMP_BIOS_IMAGE"

    size="$(stat -c "%s" "$TEMP_BIOS_IMAGE")"
    pad_size="$((ABOOT_CONF_START + ABOOT_CONF_SIZE - size))"
    dd if=/dev/zero bs=1 count="$pad_size" >> "$TEMP_BIOS_IMAGE" 2> /dev/null

    pad_size="$((FLASH_SIZE - ABOOT_CONF_START - ABOOT_CONF_SIZE))"
    pad_blocks="$((pad_size / SECTION_BLOCK_SIZE))"
    dd if=/dev/zero bs="$SECTION_BLOCK_SIZE" count="$pad_blocks" \
       >> "$TEMP_BIOS_IMAGE" 2> /dev/null
}

parse_aboot_conf() {
    if [ ! -f "$1" ]; then
        echo "File not found: $1" >&2
        return 1
    fi

    skip_blocks="$((ABOOT_CONF_START / SECTION_BLOCK_SIZE))"
    num_blocks="$((ABOOT_CONF_SIZE / SECTION_BLOCK_SIZE))"
    dd if="$1" bs="$((SECTION_BLOCK_SIZE))" skip="$skip_blocks" \
       count="$num_blocks" 2> /dev/null | while : ; do
        read -r nv
        if [ -z "$nv" ]; then
            break
        fi
        echo -n "${nv%%=*}="
        echo -n "${nv#*=}" | base64 -d
        echo
    done
}

conf_file=""
product_name=""
while getopts "f:p:" opt; do
    case $opt in
        f)
            conf_file="$OPTARG"
            ;;
        p)
            product_name="$OPTARG"
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
        bios_util.sh read "$TEMP_BIOS_IMAGE" --partition aboot_conf || exit 1
        echo "Contents of aboot_conf:"
        parse_aboot_conf "$TEMP_BIOS_IMAGE"
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

        create_aboot_conf_image "$conf_file" "$product_name" || exit 1
        bios_util.sh write "$TEMP_BIOS_IMAGE" --partition aboot_conf
        ;;
    *)
        echo "Unknown action: $1"
        usage
        ;;
esac
