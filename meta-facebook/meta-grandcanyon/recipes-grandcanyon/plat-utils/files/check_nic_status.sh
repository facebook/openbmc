#!/bin/bash
# Workaround for NIC power loss after AC cycle.
# Reinitialize NIC power and reconfigure network.
#
# Debug switch:
#   Enable debug:  echo 1 >/mnt/data/NIC_DEBUG
#   Disable debug: echo 0 >/mnt/data/NIC_DEBUG
#   Or remove it:  rm -f /mnt/data/NIC_DEBUG
#
# Debug log:
#   /mnt/data/nic_debug.log
#   /mnt/data/nic_debug.log.1
#
# Runtime config override:
#   /mnt/data/NIC_RECOVERY_CONF
#
# Example:
#   POWER_OFF_SLEEP=1
#   NIC_BOOT_SLEEP=5
#   NCSID_RESTART_SLEEP=3
#   IFDOWN_IFUP_SLEEP=1
#   NCSI_INIT_SLEEP=5
#   FW_UTIL_TIMEOUT=20
#   I2C_TIMEOUT=3
#   NCSID_RESTART_TIMEOUT=15
#   IFDOWN_TIMEOUT=10
#   IFUP_TIMEOUT=20
# shellcheck source=/dev/null
. /usr/local/fbpackages/utils/ast-functions

TAG="check_nic_status"
MAX_RETRY=5
NIC_FW_CACHE="/tmp/cache_store/nic_fw_ver"

NIC_DEBUG_FLAG="/mnt/data/NIC_DEBUG"
NIC_RECOVERY_CONF="/mnt/data/NIC_RECOVERY_CONF"
NIC_DEBUG_LOG="/mnt/data/nic_debug.log"
NIC_DEBUG_LOG_PREV="/mnt/data/nic_debug.log.1"

# Default timing values.
POWER_OFF_SLEEP=1
NIC_BOOT_SLEEP=5
NCSID_RESTART_SLEEP=3
IFDOWN_IFUP_SLEEP=1
NCSI_INIT_SLEEP=5

# Default timeout values.
FW_UTIL_TIMEOUT=20
I2C_TIMEOUT=3
NCSID_RESTART_TIMEOUT=15
IFDOWN_TIMEOUT=10
IFUP_TIMEOUT=20

is_uint() {
  case "$1" in
    ''|*[!0-9]*)
      return 1
      ;;
    *)
      return 0
      ;;
  esac
}

load_runtime_config() {
  local key
  local val
  local line

  if [ ! -f "$NIC_RECOVERY_CONF" ]; then
    return 0
  fi

  while IFS= read -r line || [ -n "$line" ]; do
    # Remove leading/trailing spaces.
    line=$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

    # Ignore empty lines and comments.
    case "$line" in
      ''|\#*)
        continue
        ;;
    esac

    key=${line%%=*}
    val=${line#*=}

    # Remove spaces around key/value.
    key=$(echo "$key" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    val=$(echo "$val" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

    if ! is_uint "$val"; then
      logger -t "$TAG" -p daemon.crit "Ignore invalid config: $key=$val"
      continue
    fi

    case "$key" in
      POWER_OFF_SLEEP)
        POWER_OFF_SLEEP="$val"
        ;;
      NIC_BOOT_SLEEP)
        NIC_BOOT_SLEEP="$val"
        ;;
      NCSID_RESTART_SLEEP)
        NCSID_RESTART_SLEEP="$val"
        ;;
      IFDOWN_IFUP_SLEEP)
        IFDOWN_IFUP_SLEEP="$val"
        ;;
      NCSI_INIT_SLEEP)
        NCSI_INIT_SLEEP="$val"
        ;;
      FW_UTIL_TIMEOUT)
        FW_UTIL_TIMEOUT="$val"
        ;;
      I2C_TIMEOUT)
        I2C_TIMEOUT="$val"
        ;;
      NCSID_RESTART_TIMEOUT)
        NCSID_RESTART_TIMEOUT="$val"
        ;;
      IFDOWN_TIMEOUT)
        IFDOWN_TIMEOUT="$val"
        ;;
      IFUP_TIMEOUT)
        IFUP_TIMEOUT="$val"
        ;;
      MAX_RETRY)
        MAX_RETRY="$val"
        ;;
      *)
        logger -t "$TAG" -p daemon.crit "Ignore unknown config key: $key"
        ;;
    esac
  done < "$NIC_RECOVERY_CONF"
}

is_debug_enabled() {
  local val

  [ -f "$NIC_DEBUG_FLAG" ] || return 1

  val=$(tr -d '[:space:]' < "$NIC_DEBUG_FLAG" 2>/dev/null)

  [ "$val" = "1" ]
}

init_debug_log() {
  if ! is_debug_enabled; then
    return 0
  fi

  if [ -f "$NIC_DEBUG_LOG" ]; then
    mv -f "$NIC_DEBUG_LOG" "$NIC_DEBUG_LOG_PREV"
  fi

  {
    echo "============================================"
    echo "NIC debug log started"
    echo "Time: $(date)"
    echo "============================================"
  } > "$NIC_DEBUG_LOG"
}

write_debug_log_file() {
  if is_debug_enabled; then
    echo "[$(date '+%H:%M:%S')] $1" >> "$NIC_DEBUG_LOG"
  fi
}

log() {
  logger -t "$TAG" -p daemon.crit "$1"

  if is_debug_enabled; then
    echo "[$(date '+%H:%M:%S')] $1"
    write_debug_log_file "$1"
  fi
}

debug_log() {
  if is_debug_enabled; then
    log "$1"
  fi
}

log_runtime_config() {
  if ! is_debug_enabled; then
    return 0
  fi

  debug_log "Runtime config:"
  debug_log "  MAX_RETRY=$MAX_RETRY"
  debug_log "  POWER_OFF_SLEEP=$POWER_OFF_SLEEP"
  debug_log "  NIC_BOOT_SLEEP=$NIC_BOOT_SLEEP"
  debug_log "  NCSID_RESTART_SLEEP=$NCSID_RESTART_SLEEP"
  debug_log "  IFDOWN_IFUP_SLEEP=$IFDOWN_IFUP_SLEEP"
  debug_log "  NCSI_INIT_SLEEP=$NCSI_INIT_SLEEP"
  debug_log "  FW_UTIL_TIMEOUT=$FW_UTIL_TIMEOUT"
  debug_log "  I2C_TIMEOUT=$I2C_TIMEOUT"
  debug_log "  NCSID_RESTART_TIMEOUT=$NCSID_RESTART_TIMEOUT"
  debug_log "  IFDOWN_TIMEOUT=$IFDOWN_TIMEOUT"
  debug_log "  IFUP_TIMEOUT=$IFUP_TIMEOUT"
}

log_gpio() {
  local pin="$1"
  local val_gpio_get
  local val_gpiocli

  val_gpio_get=$(gpio_get "$pin" 2>&1)
  val_gpiocli=$(gpiocli -s "$pin" get-value 2>&1)

  debug_log "GPIO $pin: gpio_get=$val_gpio_get, gpiocli=$val_gpiocli"
}

log_eth0() {
  local state
  local carrier

  state=$(ip link show eth0 2>/dev/null | awk '
    /state/ {
      for (i = 1; i <= NF; i++) {
        if ($i == "state") {
          print $(i + 1)
          exit
        }
      }
    }
  ')

  [ -n "$state" ] || state="unknown"

  carrier=$(cat /sys/class/net/eth0/carrier 2>/dev/null || echo "unknown")

  debug_log "eth0 state=$state carrier=$carrier"
}

log_ncsid() {
  local pid

  if pgrep -x ncsid > /dev/null; then
    pid=$(pgrep -x ncsid | tr '\n' ' ')
    debug_log "ncsid is RUNNING (pid=$pid)"
  else
    debug_log "ncsid is NOT RUNNING"
  fi
}

log_i2c_nic() {
  local reg3
  local reg4

  reg3=$(timeout "$I2C_TIMEOUT" i2ctransfer -y 5 w1@0xf 0x3 r1 2>&1 || echo "timeout/error")
  reg4=$(timeout "$I2C_TIMEOUT" i2ctransfer -y 5 w1@0xf 0x4 r1 2>&1 || echo "timeout/error")

  debug_log "I2C NIC reg 0x3 (power mode, expect 0x01) = $reg3"
  debug_log "I2C NIC reg 0x4 (prog  mode, expect 0x00) = $reg4"
}

log_ncsi_dmesg() {
  local label="$1"

  debug_log "--- dmesg NCSI/eth0 [$label] ---"

  dmesg | grep -i -E "ncsi|eth0" | tail -15 | while IFS= read -r line; do
    debug_log "  dmesg: $line"
  done
}

debug_snapshot() {
  local label="$1"

  if ! is_debug_enabled; then
    return 0
  fi

  debug_log "--- Debug snapshot: $label ---"
  log_gpio "NIC_PRSNTB3_N"
  log_gpio "BMC_NIC_FULL_PWR_EN_R"
  log_eth0
  log_ncsid
  log_i2c_nic
  log_ncsi_dmesg "$label"
}

check_nic_fw() {
  local out
  local rc

  rm -f "$NIC_FW_CACHE"

  out=$(timeout "$FW_UTIL_TIMEOUT" /usr/bin/fw-util nic --version 2>&1)
  rc=$?

  if is_debug_enabled; then
    debug_log "fw-util nic --version rc=$rc: $out"

    if [ -s "$NIC_FW_CACHE" ]; then
      debug_log "nic_fw_ver cache: $(cat "$NIC_FW_CACHE")"
    else
      debug_log "nic_fw_ver cache: NOT PRESENT"
    fi
  fi

  if [ "$rc" -eq 0 ] && [ -s "$NIC_FW_CACHE" ]; then
    return 0
  fi

  return 1
}

restart_ncsid() {
  if [ -x /etc/init.d/ncsid ]; then
    debug_log "Restarting ncsid..."

    if timeout "$NCSID_RESTART_TIMEOUT" /etc/init.d/ncsid restart; then
      debug_log "ncsid restart done"
    else
      log "ncsid restart failed or timeout"
    fi

    sleep "$NCSID_RESTART_SLEEP"
    log_ncsid
  else
    debug_log "/etc/init.d/ncsid not found or not executable"
  fi
}

reinit_eth0() {
  debug_log "Running ifdown eth0..."

  if timeout "$IFDOWN_TIMEOUT" ifdown eth0; then
    debug_log "ifdown eth0 done"
  else
    log "ifdown eth0 failed or timeout"
  fi

  sleep "$IFDOWN_IFUP_SLEEP"
  log_eth0

  debug_log "Running ifup eth0..."

  if timeout "$IFUP_TIMEOUT" ifup eth0; then
    debug_log "ifup eth0 done"
  else
    log "ifup eth0 failed or timeout"
  fi

  log_eth0
}

power_cycle_nic() {
  log "Reinit NIC..."

  debug_snapshot "before-power-cycle"

  gpio_set BMC_NIC_FULL_PWR_EN_R 0
  debug_log "NIC power off"
  log_gpio "BMC_NIC_FULL_PWR_EN_R"
  log_eth0

  sleep "$POWER_OFF_SLEEP"

  gpio_set BMC_NIC_FULL_PWR_EN_R 1
  debug_log "NIC power on"
  log_gpio "BMC_NIC_FULL_PWR_EN_R"

  # Wait for NIC internal boot.
  sleep "$NIC_BOOT_SLEEP"

  log_eth0
  log_i2c_nic

  # Restart NCSI daemon to avoid stale NCSI state.
  restart_ncsid

  # Re-trigger eth0.
  reinit_eth0

  # Wait for NCSI initialization.
  sleep "$NCSI_INIT_SLEEP"

  debug_snapshot "after-power-cycle"
}

check_nic_status() {
  local initial_failure_snapshot_done=0

  log "Checking NIC power status..."

  if is_debug_enabled; then
    log "NIC debug mode enabled: $NIC_DEBUG_FLAG contains 1"
    debug_log "Debug log file: $NIC_DEBUG_LOG"

    if [ -f "$NIC_RECOVERY_CONF" ]; then
      debug_log "Runtime config file detected: $NIC_RECOVERY_CONF"
    else
      debug_log "Runtime config file not found, using default timing values"
    fi

    log_runtime_config
  fi

  if [ "$(gpio_get NIC_PRSNTB3_N)" = "1" ]; then
    log "NIC card is missing"
    exit 0
  fi

  debug_log "NIC_PRSNTB3_N=0: NIC card is present"

  for retry in $(seq 1 "$MAX_RETRY"); do
    debug_log "--- Retry $retry/$MAX_RETRY ---"

    if check_nic_fw; then
      if [ "$retry" -eq 1 ]; then
        log "NIC firmware detected, no recovery needed"
      else
        log "NIC firmware detected, recovery successful at retry $retry"
      fi

      log_eth0
      log_ncsid
      exit 0
    fi

    if [ "$initial_failure_snapshot_done" -eq 0 ]; then
      debug_snapshot "initial-failure"
      initial_failure_snapshot_done=1
    fi

    if [ "$retry" -eq "$MAX_RETRY" ]; then
      log "Fail to power on NIC after $MAX_RETRY retries"
      debug_snapshot "final-failure"
      exit 1
    fi

    power_cycle_nic
  done

  log "Fail to power on NIC"
  exit 1
}

load_runtime_config
init_debug_log
check_nic_status