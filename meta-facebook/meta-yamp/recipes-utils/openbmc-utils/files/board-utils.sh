#!/bin/sh

# Copyright 2018-present Facebook. All Rights Reserved.

# shellcheck disable=SC2034
SUPCPLD_SYSFS_DIR="/sys/bus/i2c/devices/12-0043"
SCDCPLD_SYSFS_DIR="/sys/bus/i2c/devices/4-0023"
SUP_PWR_ON_SYSFS="${SUPCPLD_SYSFS_DIR}/cpu_control"
PWR_SYSTEM_SYSFS="${SUPCPLD_SYSFS_DIR}/chassis_power_cycle"
SCD_TH3_RST_ON_SYSFS="${SCDCPLD_SYSFS_DIR}/th3_reset"
SCD_TH3_PCI_RST_ON_SYSFS="${SCDCPLD_SYSFS_DIR}/th3_pci_reset"
SCD_RST="${SCDCPLD_SYSFS_DIR}/scd_reset"
LC_SMB_MUX_RST="${SCDCPLD_SYSFS_DIR}/lc_smb_mux_rst"

LAYOUT_FILE=/etc/yamp_flash.layout
CPU_CONF_FILE=/etc/cpu_aboot.conf
BMC_CONF_FILE=/etc/bmc_aboot.conf

BOARD_NAME_VAR="DMI_BOARD_NAME"
BOARD_NAME_VAL="YAMP"
BOARD_VER_VAR="DMI_BOARD_VERSION"
BOARD_VER_VAL="1.0"

wedge_iso_buf_enable() {
    return 0
}

wedge_iso_buf_disable() {
    return 0
}

wedge_is_us_on() {
    isWedgeOn=$(cat $SUP_PWR_ON_SYSFS 2> /dev/null | head -n 1)
    if [ -z "$isWedgeOn" ]; then
        return 1
    elif [ "$isWedgeOn" = "0x1" ]; then
        return 0 # uServer is on
    else
        return 1
    fi
}

wedge_board_type() {
    echo 'YAMP'
}

wedge_slot_id() {
    printf "1\n"
}

wedge_board_rev() {
    # assume P2
    return 2
}

# Should we enable OOB interface or not
wedge_should_enable_oob() {
    # YAMP uses BMC MAC since beginning
    return 1
}

wedge_power_off_asic() {
    # It is not exact power off.
    # Instead, the ASIC is kept in reset in this case.
    echo 1 > $SCD_TH3_RST_ON_SYSFS
    echo 1 > $SCD_RST
    sleep 1
    echo 1 > $SCD_TH3_PCI_RST_ON_SYSFS
}

wedge_power_on_asic() {
    # Order matters here
    echo 0 > $SCD_TH3_RST_ON_SYSFS
    echo 0 > $SCD_RST
    usleep 250000
    echo 0 > $SCD_TH3_PCI_RST_ON_SYSFS
}

wedge_power_on_board() {
    # Order matters here
    echo 1 > $SUP_PWR_ON_SYSFS
    sleep 1
    wedge_power_off_asic
    sleep 1
    wedge_power_on_asic
    sleep 1
}

wedge_power_off_board() {
    # Order matters here
    echo 0 > $SUP_PWR_ON_SYSFS
    sleep 1
    wedge_power_off_asic
    sleep 1
}

get_section_start() {
    name="$1"
    startstr=$(awk -v name="$name" '$2 == name {print $1}' $LAYOUT_FILE | \
               cut -d ':' -f1)
    echo "0x$startstr"
}

get_section_size() {
    name="$1"
    section_start=$(get_section_start "$name")
    endstr=$(awk -v name="$name" '$2 == name {print $1}' $LAYOUT_FILE | \
             cut -d ':' -f2)
    section_end="0x$endstr"

    echo $((section_end + 1 - section_start))
}

get_total_size() {
    get_section_size total
}

ABOOT_CONF_START=$(get_section_start aboot_conf)
FLASH_SIZE=$(get_total_size)
ABOOT_CONF_SIZE=$(get_section_size aboot_conf)
SECTION_BLOCK_SIZE=$((0x1000))

create_aboot_conf_image() {
    conf_file="$1"
    image_file="$2"
    dest="$3"

    echo "Using Aboot conf file: $conf_file"

    if [ ! -f "$conf_file" ]; then
        echo "File not found: $conf_file" >&2
        return 1
    fi

    rm -f "$dest"
    touch "$dest"
    pad_blocks="$((ABOOT_CONF_START / SECTION_BLOCK_SIZE))"
    dd if="$image_file" of="$dest" bs="$SECTION_BLOCK_SIZE" \
       count="$pad_blocks" 2> /dev/null

    awk -F '=' '/^[^#\s]+=\S+/ {print $1 " " $2}' "$1" | while : ; do
        read -r name val
        if [ -z "$name" ]; then
            break
        fi
        printf "%s=" "$name" >> "$dest"
        printf "%s" "$val" | base64 >> "$dest"
    done

    ( printf "%s=" "$BOARD_NAME_VAR"; printf "%s" "$BOARD_NAME_VAL" | base64;
      printf "%s=" "$BOARD_VER_VAR"; printf "%s" "$BOARD_VER_VAL" | base64
    ) >> "$dest"

    size="$(stat -c "%s" "$dest")"
    pad_size="$((ABOOT_CONF_START + ABOOT_CONF_SIZE - size))"
    dd if=/dev/zero bs=1 count="$pad_size" >> "$dest" 2> /dev/null

    pad_size="$((FLASH_SIZE - ABOOT_CONF_START - ABOOT_CONF_SIZE))"
    pad_blocks="$((pad_size / SECTION_BLOCK_SIZE))"
    skip_size="$((ABOOT_CONF_START + ABOOT_CONF_SIZE))"
    skip_blocks="$((skip_size / SECTION_BLOCK_SIZE))"
    dd if="$image_file" bs="$SECTION_BLOCK_SIZE" count="$pad_blocks" \
       skip="$skip_blocks" >> "$dest" 2> /dev/null
}

parse_aboot_conf() {
    if [ ! -f "$1" ]; then
        echo "File not found: $1" >&2
        return 1
    fi

    skip_blocks="$((ABOOT_CONF_START / SECTION_BLOCK_SIZE))"
    num_blocks="$((ABOOT_CONF_SIZE / SECTION_BLOCK_SIZE))"
    dd if="$1" bs="$((SECTION_BLOCK_SIZE))" skip="$skip_blocks" \
    count="$num_blocks" 2> /dev/null | tr '\001-\011\013-\037\177-\377' '.' | while : ; do
        read -r nv
        if [ -z "$nv" ]; then
            break
        fi
        name="${nv%%=*}"
        val="${nv#*=}"
        if [ "$name" = "$nv" ] || [ "$val" = "$nv" ]; then
            echo "Error: detected invalid data in aboot_conf section"
            exit 1
        fi
        if ! enc_val=$(printf "%s" "$val" | base64 -d); then
            echo "Error: detected invalid data in aboot_conf section"
            exit 1
        fi
        echo "${name}=${enc_val}"
    done
}
