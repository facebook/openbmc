#!/bin/bash
. /usr/local/fbpackages/utils/ast-functions

ME_UTIL="/usr/bin/me-util"
FRUID_UTIL="/usr/local/bin/fruid-util"
FW_UTIL="/usr/bin/fw-util"
FRU_NAME="$1"
INTERFACE="PECI_INTERFACE"
PCIE_INTERFACE="PECI_INTERFACE"

DUMP_UTIL='/usr/bin/crashdump'
DUMP_DIR='/tmp/crashdump/output'
PARSE_UTIL='/usr/bin/bafi'
ACD=true

if [ ! -f "$DUMP_UTIL" ]; then
  ACD=false
fi

function is_numeric {
  if [ "$(echo "$1" | grep -cE "^\-?([[:xdigit:]]+)(\.[[:xdigit:]]+)?$")" -gt 0 ]; then
    return 1
  else
    return 0
  fi
}

if [ "$FRU_NAME" != "server" ]; then
  echo "FRU: $FRU_NAME is not supported"
  exit 1
fi

case "$FRU_NAME" in
  server) FRU_ID=1 ;;
  *)
    echo "Unknown FRU: $FRU_NAME"
    exit 1
    ;;
esac

# File format autodump.pid (See pal_is_crashdump_ongoing()
# function definition)
PID=$$
PID_FILE="/var/run/autodump.pid"

# check if running auto dump
[ -r "$PID_FILE" ] && OLDPID=$(cat "$PID_FILE") || OLDPID=''

# Set current pid
echo "$PID" > "$PID_FILE"

# kill previous autodump if exist
if [ -n "$OLDPID" ] && grep "autodump" "/proc/$OLDPID/cmdline" &> /dev/null; then
  # Check if 2nd dump is running
  if pgrep -f "autodump\.sh.*server.*(second|dwr)" > /dev/null 2>&1; then
    echo "$OLDPID" > "$PID_FILE"
    echo "2nd DUMP or Demoted Warm Reset DUMP is running. exit."
    exit 1
  fi
  IS_SECOND_OR_DWR=false
  for arg in "$@"; do
    if [ "$arg" = "--second" ] || [ "$arg" = "--dwr" ]; then
      IS_SECOND_OR_DWR=true
      break
    fi
  done

  if [ "$IS_SECOND_OR_DWR" = false ]; then
    echo "$OLDPID" > "$PID_FILE"
    echo "Crashdump is already running (PID: $OLDPID), skip this trigger."
    exit 0
  fi

  LOG_FILE="/tmp/autodump.log"
  LOG_ARCHIVE="/mnt/data/autodump_uncompleted_server.tar.gz"
  {
    echo -n "(uncompleted) Auto Dump End at "
    date
  } >> "$LOG_FILE"

  tar zcf "$LOG_ARCHIVE" -C "$(dirname "$LOG_FILE")" "$(basename "$LOG_FILE")" && \
  rm -rf "$LOG_FILE"

  echo "kill pid $OLDPID..."
  kill -s 9 "$OLDPID"

  if [ "$ACD" = true ]; then
    pkill -KILL -f "$DUMP_UTIL $FRU_ID --type IERR" >/dev/null 2>&1
    pkill -KILL -f "$PARSE_UTIL .*server" >/dev/null 2>&1
  else
    pkill -f 'dump\.sh.*server' > /dev/null 2>&1 || true
    pkill -f 'me-util.*server' > /dev/null 2>&1 || true
    pkill -f 'bic-util.*server' > /dev/null 2>&1 || true
    pkill -f 'fw-util.*server.*version' > /dev/null 2>&1 || true
  fi
fi
unset OLDPID

# Set crashdump timestamp
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" "$sys_runtime")
kv set server_crashdump $((sys_runtime+1200))

DUMP_SCRIPT="/usr/bin/dump.sh"
CRASHDUMP_FILE="/mnt/data/crashdump_server"
CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_server.tar.gz"
PARSE_FILE="/mnt/data/bafi_server.log"
LOG_MSG_PREFIX=""

DWR=0
SECOND_DUMP=0
DELAY_SEC=30

while test $# -gt 1
do
  case "$2" in
  --dwr)
    DWR=1
    ;;
  --now)
    DELAY_SEC=0
    ;;
  --second)
    SECOND_DUMP=1
    ;;
  *)
    echo "unknown argument $2"
    ;;
  esac
  shift
done

if [ "$DWR" == "1" ] || [ "$SECOND_DUMP" == "1" ]; then
  echo "Auto Dump after System Reset or Demoted Warm Reset"
fi

if [ "$DELAY_SEC" != "0" ]; then
  echo "Auto Dump will start after ${DELAY_SEC}s..."
  sleep "${DELAY_SEC}"
fi

echo "Auto Dump for Server Started"
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: server started"

if [ "$ACD" = true ]; then
  echo "Using ACD for crashdump..."

  "$DUMP_UTIL" "$FRU_ID" --type IERR
  LOG_FILE=""
  for f in "${DUMP_DIR}"/*.json; do
    [ -f "$f" ] || continue
    if [ -z "$LOG_FILE" ] || [ "$f" -nt "$LOG_FILE" ]; then
      LOG_FILE="$f"
    fi
  done
  if [ -z "$LOG_FILE" ]; then
    logger -t "ipmid" -p daemon.crit "No crashdump file found"
    rm -f "$PID_FILE"
    exit 1
  fi

  if [ -f "$PARSE_UTIL" ]; then
    BAFI_TEST=$("$PARSE_UTIL" "$LOG_FILE" 2>&1)
    if echo "$BAFI_TEST" | grep -q "Unrecognized CPU"; then
      echo "bafi does not support this CPU, skipping bafi..."
      tar zcf "$CRASHDUMP_LOG_ARCHIVE" \
        -C "$DUMP_DIR" "./$(basename "$LOG_FILE")"
    else
      echo "$BAFI_TEST" > "$PARSE_FILE"
      tar zcf "$CRASHDUMP_LOG_ARCHIVE" \
        -C "$DUMP_DIR" "./$(basename "$LOG_FILE")" \
        -C "$(dirname "$PARSE_FILE")" "./$(basename "$PARSE_FILE")"
    fi
  else
    tar zcf "$CRASHDUMP_LOG_ARCHIVE" \
      -C "$DUMP_DIR" "./$(basename "$LOG_FILE")"
  fi

else
  echo "ACD not found, using legacy dump method..."

  # HEADER LINE for the dump
  "$DUMP_SCRIPT" "time" > "$CRASHDUMP_FILE"

  # Get BMC version & hostname
  {
    strings /dev/mtd0 | grep 2019.04
    uname -a
    cat /etc/issue
  } >> "$CRASHDUMP_FILE"

  # Get fw info
  FW_RES=$("$FW_UTIL" "server" "--version")
  FRUID_RES=$("$FRUID_UTIL" server)
  RES=$("$ME_UTIL" server 0x18 0x01)
  RET=$?
  {
    echo "Get firmware version info: "
    echo "$FW_RES"
    echo "Get FRUID Info:"
    echo "$FRUID_RES"
    echo "Get Device ID:"
    echo "$RES"
  } >> "$CRASHDUMP_FILE"

  # if ME has response and in operational mode, PECI through ME
  if [ "$RET" -eq "0" ] && [ "${RES:6:1}" == "0" ]; then
    RES=$("$ME_UTIL" server 0xb8 0x40 0x57 0x01 0x00 0x30 0x05 0x05 0xa1 0x00 0x00 0x00 0x00)
    RET=$?
    if [ "$RET" -eq "0" ] && [ "${RES:0:11}" == "57 01 00 40" ]; then
      INTERFACE="ME_INTERFACE"
    else
      INTERFACE="PECI_INTERFACE"
    fi
  else
    INTERFACE="PECI_INTERFACE"
  fi
  PCIE_INTERFACE="$INTERFACE"

  # Major Firmware Revision
  REV=$(echo "$RES"| awk '{print $3;}')
  is_numeric "$(echo "$RES"| awk '{print $1;}')"
  if [ "$?" == 1 ] ;then
    Mode=$((REV & 0x80))
    if [ "$Mode" -ne 0 ] ;then
      echo "Device firmware update or Self-initialization in progress or Firmware in the recovery boot-loader mode" >> "$CRASHDUMP_FILE"
    fi
    # Get Self-test result
    echo "Get Self-test result:" >> "$CRASHDUMP_FILE"
    RES=$("$ME_UTIL" server 0x18 0x04)
    echo "$RES" >> "$CRASHDUMP_FILE"
    CC=$(echo "$RES"| awk '{print $1;}')
    is_numeric "$CC"
    if [ "$?" == 1 ] ;then
      if [ "$CC" -ne 55 ] ;then
        if [ "$CC" -eq 56 ] ;then
          echo "Self Test function not implemented in this controller" >> "$CRASHDUMP_FILE"
        elif [ "$CC" -eq 57 ] ;then
          echo "Corrupted or inaccessible data or devices" >> "$CRASHDUMP_FILE"
        elif [ "$CC" -eq 58 ] ;then
          echo "Fatal hardware error" >> "$CRASHDUMP_FILE"
        elif [ "$CC" -eq 80 ] ;then
          echo "PSU Monitoring service error" >> "$CRASHDUMP_FILE"
        elif [ "$CC" -eq 81 ] ;then
          echo "Firmware entered Recovery boot-loader mode" >> "$CRASHDUMP_FILE"
        elif [ "$CC" -eq 82 ] ;then
          echo "HSC Monitoring service error" >> "$CRASHDUMP_FILE"
        elif [ "$CC" -eq 83 ] ;then
          echo "Firmware entered non-UMA restricted mode of operation" >> "$CRASHDUMP_FILE"
        else
          echo "Unknown error" >> "$CRASHDUMP_FILE"
        fi
      fi
    fi

    # PCI config read is not support ME DMI interface
    RES=$("$ME_UTIL" server 0xb8 0x40 0x57 0x01 0x00 0x30 0x06 0x05 0x61 0x00 0x00 0x81 0x0D 0x00)
    RET=$?
    if [ "$RET" -eq "0" ] && [ "${RES:0:19}" == "Completion Code: AC" ]; then
      PCIE_INTERFACE="PECI_INTERFACE"
    fi
  fi

  {
    echo "Set coreid msr interface = $INTERFACE"
    echo "Set pcie dwr interface = $PCIE_INTERFACE"
    echo "Sensor history at dump:"
    "$DUMP_SCRIPT" server "sensors"
    echo "Sensor threshold at dump:"
    "$DUMP_SCRIPT" server "threshold"
    # COREID dump
    "$DUMP_SCRIPT" server "coreid" "$INTERFACE"
    # MSR dump
    "$DUMP_SCRIPT" server "msr" "$INTERFACE"
    # PCIe dump
    "$DUMP_SCRIPT" server "pcie" "$PCIE_INTERFACE"
  } >> "$CRASHDUMP_FILE"

  # only second/dwr autodump need to rename accordingly
  if [ "$DWR" == "1" ] || [ "$SECOND_DUMP" == "1" ]; then
    "$DUMP_SCRIPT" server "dwr" "$PCIE_INTERFACE" >> "$CRASHDUMP_FILE"
    if [ "$?" == "2" ]; then
      CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_dwr_server.tar.gz"
      LOG_MSG_PREFIX="DWR "
    else
      CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_second_server.tar.gz"
      LOG_MSG_PREFIX="SECOND_DUMP "
    fi
  fi

  {
    echo -n "Auto Dump End at "
    date
  } >> "$CRASHDUMP_FILE"

  tar zcf "$CRASHDUMP_LOG_ARCHIVE" \
    -C "$(dirname "$CRASHDUMP_FILE")" \
    "$(basename "$CRASHDUMP_FILE")" && \
  rm -rf "$CRASHDUMP_FILE"

fi

logger -t "ipmid" -p daemon.crit \
  "${LOG_MSG_PREFIX}Crashdump for FRU: server is generated at $CRASHDUMP_LOG_ARCHIVE"
cp -f "$CRASHDUMP_LOG_ARCHIVE" /tmp
echo "Auto Dump for Server Completed"

# Remove current pid file
rm "$PID_FILE"

echo "${LOG_MSG_PREFIX}Auto Dump Stored in $CRASHDUMP_LOG_ARCHIVE"
exit 0