#!/bin/sh

# shellcheck disable=SC1091
# shellcheck disable=SC2012
. /usr/local/bin/openbmc-utils.sh

CPLD_JTAG_SEL_L="CPLD_JTAG_SEL_L"
JTAG_TRST_L="JTAG_TRST_L"
SCM_FPGA_LATCH_L="SCM_FPGA_LATCH_L"
JTAG_EN="${SCMCPLD_SYSFS_DIR}/jtag_en"
PROGRAM_SEL="${SCMCPLD_SYSFS_DIR}/program_sel"
SPI_CTRL="${SMBCPLD_SYSFS_DIR}/spi_ctrl"
SPI_PIM_SEL="${SMBCPLD_SYSFS_DIR}/spi_pim_en"
SPI_TH4_QSPI_SEL="${SMBCPLD_SYSFS_DIR}/spi_th4_qspi_en"
JTAG_CTRL="${SMBCPLD_SYSFS_DIR}/jtag_ctrl"
SMB_SPIDEV="spidev1.1"

SCM_PROGRAM=false
SMB_PROGRAM=false
CACHED_SCM_PWR_ON_SYSFS="0x1"

trap disconnect_program_paths INT TERM QUIT EXIT

usage() {
    program=$(basename "$0")
    echo "Usage:"
    echo "$program <fpga> <action> <fpga file>"
    echo "      <fpga> : scm, smb, smb_cpld, fan, th4_qspi,"
    echo "               pim, pim_base, pim16q, pim8ddm"
    echo "      <action> : program, verify, erase (spi only)"
    exit 1
}

disconnect_program_paths() {
    # Return values to defaults
    gpio_set_value $CPLD_JTAG_SEL_L 1
    gpio_set_value $JTAG_TRST_L 0
    if [ "$SCM_PROGRAM" = true ]; then
        # Give SCM cpld time to come back on SMBus
        sleep 2
        # Restore x86 power state
        echo "$CACHED_SCM_PWR_ON_SYSFS" > "$SCM_PWR_ON_SYSFS" || {
           echo "Failed to recover CPU power state."
        }
        sleep 1
        gpio_set_value $SCM_FPGA_LATCH_L 1
    else
        echo 0 > "$JTAG_EN"
        echo 1 > "$PROGRAM_SEL"
    fi

    if [ "$SMB_PROGRAM" = false ]; then
        echo 0 > "$SPI_CTRL"
        echo 0 > "$JTAG_CTRL"
    fi
}

connect_scm_jtag() {
    # Store initial state of x86 power in order to restore after programming
    CACHED_SCM_PWR_ON_SYSFS="$(head -n 1 "$SCM_PWR_ON_SYSFS" 2> /dev/null)"
    gpio_set_value $SCM_FPGA_LATCH_L 0

    gpio_set_value $CPLD_JTAG_SEL_L 0
    gpio_set_value $JTAG_TRST_L 1
    echo 0 > "$JTAG_EN"
}

connect_smb_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    gpio_set_value $JTAG_TRST_L 1
    echo 1 > "$JTAG_EN"
    echo 1 > "$PROGRAM_SEL"
}

connect_smb_cpld_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    gpio_set_value $JTAG_TRST_L 1
    echo 1 > "$JTAG_EN"
    echo 0 > "$PROGRAM_SEL"
    echo 0 > "$SPI_CTRL"
    echo 0x2 > "$JTAG_CTRL"
}

connect_fan_jtag() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    gpio_set_value $JTAG_TRST_L 1
    echo 1 > "$JTAG_EN"
    echo 0 > "$PROGRAM_SEL"
    echo 0 > "$SPI_CTRL"
    echo 0x1 > "$JTAG_CTRL"
}

connect_pim_flash() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    echo 0 > "$PROGRAM_SEL"
    echo 1 > "$SPI_PIM_SEL"
    devmem_set_bit 0x1e6e2438 8 # SPI1CS1 Function enable

    for i in $(seq 1 5); do
        msg="$(flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c "MT25QL256" 2>/dev/null)"
        if echo "$msg" | grep -q "Found Micron flash chip"; then
            echo "Detected PIM Flash device."
            return 0;
        else
            echo "Attempt ${i} to detect pim flash failed..."
        fi
    done

    echo "Failed to detect PIM Flash device with flashrom"
    echo "$msg"
    exit 1
}

connect_th4_qspi_flash() {
    gpio_set_value $CPLD_JTAG_SEL_L 1
    echo 0 > "$PROGRAM_SEL"
    echo 1 > "$SPI_TH4_QSPI_SEL"
    devmem_set_bit 0x1e6e2438 8 # SPI1CS1 Function enable

    for i in $(seq 1 5); do
        msg="$(flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c "MT25QU256" 2>/dev/null)"
        if echo "$msg" | grep -q "Found Micron flash chip"; then
            echo "Detected TH4 QSPI Flash device."
            return 0;
        else
            echo "Attempt ${i} to detect TH4 QSPI flash failed..."
        fi
    done

    echo "Failed to detect TH4 QSPI Flash device with flashrom"
    echo "$msg"
    exit 1
}

do_scm() {
    # verify the fpga file
    if ! grep "DESIGN.*ird" "$2" > /dev/null; then
        echo "$2 is not a valid SCM FPGA file"
        exit 1
    fi
    SCM_PROGRAM=true
    connect_scm_jtag
    jam -l/usr/lib/libcpldupdate_dll_ioctl.so -v -a"${1^^}" "$2" \
      --ioctl IOCBITBANG
}

do_smb() {
    # verify the fpga file
    if ! grep "DESIGN.*onda" "$2" > /dev/null; then
        echo "$2 is not a valid SMB FPGA file"
        exit 1
    fi
    SMB_PROGRAM=true
    connect_smb_jtag
    jam -l/usr/lib/libcpldupdate_dll_ioctl.so -v -a"${1^^}" "$2" \
      --ioctl IOCBITBANG
}

do_smb_cpld() {
    # verify the fpga file
    if ! grep "DESIGN.*ide" "$2" > /dev/null; then
        echo "$2 is not a valid SMB CPLD file"
        exit 1
    fi
    connect_smb_cpld_jtag
    jam -l/usr/lib/libcpldupdate_dll_ioctl.so -v -a"${1^^}" "$2" \
      --ioctl IOCBITBANG
}

do_fan() {
    # verify the fpga file
    if ! grep "DESIGN.*ckels" "$2" > /dev/null; then
        echo "$2 is not a valid FAN FPGA file"
        exit 1
    fi
    connect_fan_jtag
    jam -l/usr/lib/libcpldupdate_dll_ioctl.so -v -a"${1^^}" "$2" \
      --ioctl IOCBITBANG
}

program_spi_image() {
    ORIGINAL_IMAGE="$1" # Image path
    CHIP_TYPE="$2"      # Chip type
    ACTION="${4^^}"         # Program or verify

    case "${3^^}" in # Selects passed partition name
        PIM_BASE)
           PARTITION="pim_base"
           SKIP_MB=0
           # TODO: Restrict PIM_BASE later
           # FPGA_TYPE=0
           FPGA_TYPE=-1
           ;;
        PIM16Q)
           PARTITION="pim16q"
           SKIP_MB=1
           FPGA_TYPE=1
           ;;
        PIM8DDM)
           PARTITION="pim8ddm"
           SKIP_MB=2
           FPGA_TYPE=2
           ;;
        FULL)
           PARTITION="full" # Flash entire spi flash
           SKIP_MB=0
           FPGA_TYPE=-1
           ;;
        *)
           echo "Unknown region $3 specified"
           exit 1
           ;;
    esac

    echo "Selected partition $PARTITION to program."

    if [ "$FPGA_TYPE" -ge 0 ] ; then
        # Verify the image type
        IMG_FPGA_TYPE=$(head -c3 "$ORIGINAL_IMAGE" | hexdump -v -e '/1 "%u\n"' | \
                        tail -n1)
        if [ "$IMG_FPGA_TYPE" != "$FPGA_TYPE" ]; then
            echo "FPGA type in image, $IMG_FPGA_TYPE, does not match expected" \
                 "FPGA type, $FPGA_TYPE, for partition $PARTITION"
            exit 1
        fi
    fi

    # We need to 32 MB binary, we will fill if needed with 1s.
    # We will only program/verify the specified image partition
    EEPROM_SIZE="33554432" # 32 MB
    IMAGE_SIZE="$(ls -l "$ORIGINAL_IMAGE" | awk '{print $5}')"
    TEMP_IMAGE="/tmp/pim_image_32mb.bin"

    if [ "$IMAGE_SIZE" -gt "$EEPROM_SIZE" ] ; then
        echo "$IMAGE size $IMAGE_SIZE is greater than eeprom capacity $EEPROM_SIZE"
        exit 1
    fi

    OPERATION="-w"
    if [ "$ACTION" = "VERIFY" ]
    then
        OPERATION="-v"
    fi

    fill=$((EEPROM_SIZE - IMAGE_SIZE))
    if [ "$fill" = 0 ] ; then
        echo "$ORIGINAL_IMAGE already 32MB, no need to resize..."
        flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c "$CHIP_TYPE" -N \
            --layout /etc/elbert_pim.layout --image "$PARTITION" $OPERATION "$ORIGINAL_IMAGE"
    else
        bs=1048576 # 1MB blocks

        echo "$ORIGINAL_IMAGE size $IMAGE_SIZE < $EEPROM_SIZE (32MB)"
        echo "Resizing $ORIGINAL_IMAGE by $fill bytes to $TEMP_IMAGE"

        # Let's make a copy of the program image and 0xff-fill it to 32MB
        # Prepend/append 0x1 bytes accordingly
        {
            dd if=/dev/zero bs=$bs count="$SKIP_MB" | tr "\000" "\377"
            dd if="$ORIGINAL_IMAGE"
        } 2>/dev/null > "$TEMP_IMAGE"
        # cp "$ORIGINAL_IMAGE" "$TEMP_IMAGE"

        div=$((fill/bs)) # Number of 1MB blocks to write
        div=$((div - SKIP_MB))
        rem=$((fill%bs)) # Remaining
        {
            dd if=/dev/zero bs=$bs count=$div | tr "\000" "\377"
            dd if=/dev/zero bs=$rem count=1 | tr "\000" "\377"
        } 2>/dev/null >> "$TEMP_IMAGE"

        TEMP_IMAGE_SIZE="$(ls -l "$TEMP_IMAGE" | awk '{print $5}')"
        if [ "$TEMP_IMAGE_SIZE" -ne "$EEPROM_SIZE" ] ; then
            echo "Failed to resize $ORIGINAL_IMAGE to 32MB... exiting."
            exit 1
        fi
        # Program the 32MB pim image
        flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c "$CHIP_TYPE" -N \
             --layout /etc/elbert_pim.layout --image "$PARTITION" $OPERATION "$TEMP_IMAGE"
        rm "$TEMP_IMAGE"
    fi
}

read_spi_partition_image() {
    DEST="$1" # READ Image output path
    TEMP="/tmp/tmp_pim_image" # Temporary path for 32MB read
    CHIP_TYPE="$2"   # Chip type
    COUNT=1 # Default partition size is 1MB
    SKIP_MB=0

    case "${3^^}" in # Selects passed partition name
        PIM_BASE)
           PARTITION="pim_base"
           ;;
        HEADER_PIM_BASE)
           PARTITION="header_pim_base"
           ;;
        PIM16Q)
           PARTITION="pim16q"
           SKIP_MB=1
           ;;
        HEADER_PIM16Q)
           PARTITION="header_pim16q"
           SKIP_MB=1
           ;;
        PIM8DDM)
           PARTITION="pim8ddm"
           SKIP_MB=2
           ;;
        HEADER_PIM8DDM)
           PARTITION="header_pim8ddm"
           SKIP_MB=2
           ;;
        FULL)
           PARTITION="full" # Read Entire SPI flash
           COUNT=32 # Full 32MB image
           ;;
        *)
           echo "Unknown region $3 specified"
           exit 1
           ;;
    esac

    echo "Selected partition $PARTITION to read."

    flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c "$CHIP_TYPE" \
        --layout /etc/elbert_pim.layout --image "$PARTITION" -r "$TEMP"
    dd if="$TEMP" of="$DEST" bs=1M count="$COUNT" skip="$SKIP_MB" 2> /dev/null
    rm "$TEMP"
}

strip_pim_header() {
    ORIG_IMAGE="$1"
    NEW_IMAGE="$2"
    POS=$(head -c4096 "$ORIG_IMAGE" | hexdump -v -e '/1 "%u\n"' | \
          (grep -n -e '^0$' || echo 0) | head -n1 | cut -d':' -f1)
    if [ "$POS" -gt 0 ]
    then
        if ! dd if="$ORIG_IMAGE" bs=1 count="$POS" 2> /dev/null | grep _REVISION \
             > /dev/null
        then
            # No REVISION header found
            POS=0
        fi
    fi

    if [ "$POS" -gt 0 ]
    then
        dd if="$ORIG_IMAGE" of="$NEW_IMAGE" bs="$POS" skip=1 2> /dev/null
    else
        cp "$ORIG_IMAGE" "$NEW_IMAGE"
    fi
}

do_pim() {
    case "${1^^}" in
        PROGRAM|VERIFY)
            # Image with header stripped
            BIN_IMAGE="/tmp/stripped_pim_image"
            connect_pim_flash
            strip_pim_header "$2" "$BIN_IMAGE"
            program_spi_image "$BIN_IMAGE" "MT25QL256" "$3" "$1"
            rm "$BIN_IMAGE"
            ;;
        READ)
            connect_pim_flash
            if [ -z "$3" ]; then
                flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c MT25QL256 -r "$2"
            else
                read_spi_partition_image "$2" "MT25QL256" "$3"
            fi
            ;;
        ERASE)
            connect_pim_flash
            if [ -z "$2" ]; then
                flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c MT25QL256 -E
            else
                flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c MT25QL256 \
                    --layout /etc/elbert_pim.layout --image "$2" -E
            fi
            ;;
        *)
            echo "PIM only supports program/verify/read/erase action. Exiting..."
            exit 1
            ;;
    esac
}

do_th4_qspi() {
    case "${1^^}" in
        PROGRAM|VERIFY)
            connect_th4_qspi_flash
            program_spi_image "$2" "MT25QU256" full "$1"
            ;;
        READ)
            connect_th4_qspi_flash
            flashrom -p linux_spi:dev=/dev/"$SMB_SPIDEV" -c "MT25QU256" -r "$2"
            ;;
        *)
            echo "TH4 QSPI only supports program/verify/read action. Exiting..."
            exit 1
            ;;
    esac
}

if [ $# -lt 2 ]; then
    usage
fi

case "$1" in
   smb) shift 1
      do_smb "$@"
      ;;
   smb_cpld) shift 1
      do_smb_cpld "$@"
      ;;
   scm) shift 1
      do_scm "$@"
      ;;
   pim) shift 1
      do_pim "$@" full
      ;;
   pim_base) shift 1
      do_pim "$@" pim_base
      ;;
   pim16q) shift 1
      do_pim "$@" pim16q
      ;;
   pim8ddm) shift 1
      do_pim "$@" pim8ddm
      ;;
   header_pim_base) shift 1
      do_pim "$@" header_pim_base
      ;;
   header_pim16q) shift 1
      do_pim "$@" header_pim16q
      ;;
   header_pim8ddm) shift 1
      do_pim "$@" header_pim8ddm
      ;;
   fan) shift 1
      do_fan "$@"
      ;;
   th4_qspi) shift 1
      do_th4_qspi "$@"
      ;;
   *)
      usage
      ;;
esac
