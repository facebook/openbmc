#!/bin/sh

. /usr/local/fbpackages/utils/ast-functions

SCRIPT_TAG="clear-hsc-fault"

EXPANDER_NETFN=0x06
EXPANDER_CMD=0x52
CLEAR_FAULT_CMD=0x03
STATUS_WORD_CMD=0x79
STATUS_VOUT_CMD=0x7A
PMBUS_MFR_ID_CMD=0x99

READ_WORD_TYPE=2
SEND_BYTE_TYPE=0
BLOCK_READ_4BYTE_TYPE=4
BLOCK_READ_1BYTE_TYPE=1

# STATUS_WORD bit masks (16-bit)
# UV            = BIT(3)  = 0x0008
# OT            = BIT(2)  = 0x0004
# IOUT_OC_FAULT = BIT(4)  = 0x0010
# IOUT_POUT     = BIT(13) = 0x2000
# TI IOUT_STAT  = BIT(14) = 0x4000
# TI OUT_STAT   = BIT(15) = 0x8000

UV_MASK=0x0008
INPUT_FAULT_BIT=0x2000
MPS_OTHER_FAULT_MASK=0x6014   # BIT(2)|BIT(4)|BIT(13)|BIT(14)
TI_OTHER_FAULT_MASK=0xE004    # BIT(2)|BIT(13)|BIT(14)|BIT(15)
UV_OTHER_FAULT=3
OTHER_FAULT_ONLY=1

OUT_STATUS_BIT=0x8000
STATUS_OUT_VOUT_UV_WARN=0x20

RETRY_MAX=3
RETRY_INTERVAL=1

log() {
  logger -t "$SCRIPT_TAG" "$*"
  echo "$SCRIPT_TAG: $*"
}

send_sel() {
  comp="$1"
  status_word="$2"
  msg="${comp}: other fault detected, STATUS_WORD(79h): ${status_word}"
  logger -p user.crit -t "$SCRIPT_TAG" "$msg"
}

get_system_stage() {
  stage=0

  for i in $(seq 2 -1 0); do
    val=$(gpio_get "BOARD_REV_ID${i}")
    stage=$((stage * 2 + val))
  done
  echo "$stage"
}

calc_bus_sel_hex() {
  bus="$1"
  printf '0x%02x' $((bus * 2 + 1))
}

read_status_word() {
  expander-util "$EXPANDER_NETFN" "$EXPANDER_CMD" \
    "$1" "$2" "$READ_WORD_TYPE" "$STATUS_WORD_CMD"
}

read_status_vout() {
  expander-util "$EXPANDER_NETFN" "$EXPANDER_CMD" \
    "$1" "$2" "$BLOCK_READ_1BYTE_TYPE" "$STATUS_VOUT_CMD"
}

clear_fault() {
  expander-util "$EXPANDER_NETFN" "$EXPANDER_CMD" \
    "$1" "$2" "$SEND_BYTE_TYPE" "$CLEAR_FAULT_CMD"
}

read_mfr_id() {
  expander-util "$EXPANDER_NETFN" "$EXPANDER_CMD" \
    "$1" "$2" "$BLOCK_READ_4BYTE_TYPE" "$PMBUS_MFR_ID_CMD"
}

# raw bytes "lo hi" -> 0xHHLL
bytes_to_hex() {
  low_val=$(printf '%d' "0x${1:-00}")
  high_val=$(printf '%d' "0x${2:-00}")
  printf "0x%04X" "$(((high_val << 8) | low_val))"
}

detect_mfr_id() {
  bus="$1"
  addr="$2"

  bus_sel_hex=$(calc_bus_sel_hex "$bus")

  if ! mfr_raw=$(read_mfr_id "$bus_sel_hex" "$addr" 2>&1); then
    log "read MFR_ID failed (bus=exp[$bus] addr=$addr data=$mfr_raw)"
    echo "UNKNOWN"
    return
  fi

  read -r b0 b1 b2 b3 <<EOF
$mfr_raw
EOF

  # MPS: len=3, payload="SPM" (0x53 0x50 0x4D)
  if [ "$(printf '%d' "0x${b0:-00}")" -eq 3 ] &&
     [ "$b1" = "53" ] &&
     [ "$b2" = "50" ] &&
     [ "$b3" = "4D" ]; then
    echo "MPS"
    return
  fi

  # TI: len=2, payload="TI" (0x54 0x49)
  if [ "$(printf '%d' "0x${b0:-00}")" -eq 2 ] &&
     [ "$b1" = "54" ] &&
     [ "$b2" = "49" ]; then
    echo "TI"
    return
  fi

  log "unknown MFR_ID (data=$mfr_raw)"
  echo "UNKNOWN"
}

get_mfr_fault_masks() {
  case "$1" in
    MPS)
      MFR_OTHER_FAULT_MASK="$MPS_OTHER_FAULT_MASK"
      return 0
      ;;
    TI)
      MFR_OTHER_FAULT_MASK="$TI_OTHER_FAULT_MASK"
      return 0
      ;;
    *)
      MFR_OTHER_FAULT_MASK="$MPS_OTHER_FAULT_MASK"
      return 1
      ;;
  esac
}

check_uv_only_fault() {
  bus="$1"
  addr="$2"
  uv_mask="$3"
  other_mask="$4"
  mfr="$5"

  bus_sel_hex=$(calc_bus_sel_hex "$bus")

  if ! status_raw=$(read_status_word "$bus_sel_hex" "$addr" 2>&1); then
    STATUS_WORD_HEX="$status_raw"
    return 4
  fi

  read -r low_byte high_byte <<EOF
$status_raw
EOF

  if [ -z "$low_byte" ] || [ -z "$high_byte" ]; then
    STATUS_WORD_HEX="$status_raw"
    return 4
  fi

  low_val=$(printf '%d' "0x$low_byte")
  high_val=$(printf '%d' "0x$high_byte")
  status16=$(((high_val << 8) | low_val))

  STATUS_WORD_HEX=$(printf "0x%04X" "$status16")

  status_vout_val=0

  if [ "$mfr" = "TI" ] && [ $((status16 & OUT_STATUS_BIT)) -ne 0 ]; then
    if ! status_vout_raw=$(read_status_vout "$bus_sel_hex" "$addr" 2>&1); then
      STATUS_WORD_HEX="$STATUS_WORD_HEX, STATUS_VOUT(7Ah) read failed: $status_vout_raw"
      return 4
    fi

    read -r status_vout_b0 _ <<EOF
$status_vout_raw
EOF

    status_vout_val=$(printf '%d' "0x${status_vout_b0:-00}")
  fi

  uv_val=$((uv_mask))
  other_val=$((other_mask))
  input_bit=$((INPUT_FAULT_BIT))
  has_uv=0
  has_other=0

  [ $((status16 & uv_val)) -ne 0 ] && has_uv=1

  if [ "$has_uv" -eq 1 ]; then
    other_check=$((other_val & (0xFFFF ^ input_bit)))
  else
    other_check=$other_val
  fi

  if [ "$mfr" = "TI" ] && [ $((status16 & OUT_STATUS_BIT)) -ne 0 ]; then
    if [ $((status_vout_val & STATUS_OUT_VOUT_UV_WARN)) -ne 0 ]; then
      has_uv=1
      other_check=$((other_check & (0xFFFF ^ OUT_STATUS_BIT)))
    fi
  fi

  [ $((status16 & other_check)) -ne 0 ] && has_other=1

  case "${has_uv}${has_other}" in
    00) return 0 ;;
    10) return 2 ;;
    01) return 1 ;;
    11) return 3 ;;
    *)  return 4 ;;
  esac
}

run_clear_fault() {
  comp="$1"
  bus="$2"
  addr="$3"

  bus_sel_hex=$(calc_bus_sel_hex "$bus")
  mfr=$(detect_mfr_id "$bus" "$addr")

  get_mfr_fault_masks "$mfr"
  check_uv_only_fault "$bus" "$addr" "$UV_MASK" "$MFR_OTHER_FAULT_MASK" "$mfr"
  uv_rc=$?

  case "$uv_rc" in
    0)
      log "$comp: no UV fault detected, STATUS_WORD(79h): $STATUS_WORD_HEX, skip clear fault on startup"
      return 0
      ;;
    1)
      send_sel "$comp" "$STATUS_WORD_HEX"
      status_before="$STATUS_WORD_HEX"
      ;;
    2)
      status_before="$STATUS_WORD_HEX"
      ;;
    3)
      send_sel "$comp" "$STATUS_WORD_HEX"
      status_before="$STATUS_WORD_HEX"
      ;;
    4)
      log "$comp: read  fault status failed (data=$STATUS_WORD_HEX)"
      return 1
      ;;
  esac

  if ! clear_fault "$bus_sel_hex" "$addr" >/dev/null 2>&1; then
    log "$comp: UV fault detected, but CLEAR_FAULTS failed, STATUS_WORD(79h): $status_before"
    return 1
  fi

  if ! status_after_raw=$(read_status_word "$bus_sel_hex" "$addr" 2>&1); then
    log "$comp: UV fault detected, cleared, but readback failed, STATUS_WORD(79h): $status_before"
    return 0
  fi

  read -r status_after_b0 status_after_b1 <<EOF
$status_after_raw
EOF

  status_after_hex=$(bytes_to_hex "$status_after_b0" "$status_after_b1")

  if [ "$uv_rc" -eq "$OTHER_FAULT_ONLY" ]; then
    log "$comp: Other fault detected, add SEL and cleared STATUS_WORD(79h): $status_before -> $status_after_hex"
  elif [ "$uv_rc" -eq "$UV_OTHER_FAULT" ]; then
    log "$comp: UV fault and other fault detected, add SEL and cleared STATUS_WORD(79h): $status_before -> $status_after_hex"
  else
    log "$comp: UV fault detected, cleared STATUS_WORD(79h): $status_before -> $status_after_hex"
  fi

  return 0
}

get_device_table() {
  get_uic_location
  uic_location=$?

  case "$uic_location" in
    "$UIC_LOCATION_A")
      DEVICE_TABLE="P12V_A 3 0x94"
      ;;
    "$UIC_LOCATION_B")
      DEVICE_TABLE="P12V_B 3 0x94"
      ;;
    *)
      DEVICE_TABLE=""
      log "unknown UIC location ($uic_location), Please confirm the UIC location..."
      ;;
  esac
}

main() {
  stage=
  retry=
  comp=
  bus=
  addr=

  stage=$(get_system_stage)
  if [ "$stage" -eq 6 ]; then
    log "Hack stage, skip"
    exit 0
  fi

  get_device_table

  if [ -z "$DEVICE_TABLE" ]; then
    log "no device to process, exit"
    exit 0
  fi

  echo "$DEVICE_TABLE" | while read -r comp bus addr; do
    [ -z "$comp" ] && continue

    retry=1
    while [ "$retry" -le "$RETRY_MAX" ]; do
      log "clear attempt $retry/$RETRY_MAX: comp=$comp bus=exp[$bus] addr=$addr"
      run_clear_fault "$comp" "$bus" "$addr" && break
      retry=$((retry + 1))
      [ "$retry" -le "$RETRY_MAX" ] && sleep "$RETRY_INTERVAL"
    done

    [ "$retry" -gt "$RETRY_MAX" ] && \
      log "$comp: failed after $RETRY_MAX retries"
  done
}

main "$@"
exit $?