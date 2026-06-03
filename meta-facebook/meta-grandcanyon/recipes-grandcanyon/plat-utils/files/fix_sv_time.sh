#!/bin/sh
#
# fix_sv_time.sh
#
# Fix incorrect sv status runtime after BMC time sync.
#
# Problem: BMC boots with wrong wall clock (~2018), runit records
# service start time using that wrong clock. After sync_date.sh
# corrects the clock, sv status shows hugely inflated runtimes.
#
# Solution: After time sync succeeds, check each service's displayed
# runtime against system uptime. If runtime > uptime (physically
# impossible), patch supervise/status TAI64N timestamp using the
# real process start time from /proc/<pid>/stat.
#

SV_DIR="/etc/sv"
CLK_TCK=100  # ARM Linux (AST2600) clock ticks per second

fix_service() {
    svc="$1"
    status_file="${SV_DIR}/${svc}/supervise/status"

    [ ! -f "$status_file" ] && return 0

    # Get sv status output, only process running services
    sv_output=$(sv status "$svc" 2>/dev/null)
    echo "$sv_output" | grep -q "^run:" || return 0

    # Extract displayed runtime in seconds
    displayed_runtime=$(echo "$sv_output" | sed -n 's/.*) \([0-9]*\)s.*/\1/p')
    [ -z "$displayed_runtime" ] && return 0

    # Snapshot wall clock and uptime to calculate the boot epoch
    now=$(date +%s)
    uptime_sec=$(awk '{printf "%d", $1}' /proc/uptime)
    boot_epoch=$((now - uptime_sec))

    # Runtime cannot exceed system uptime
    if [ "$displayed_runtime" -le "$uptime_sec" ]; then
        return 0
    fi

    logger -s -p user.info -t fix-sv-time \
        "${svc}: abnormal runtime ${displayed_runtime}s > uptime ${uptime_sec}s, fixing..."

    # Extract PID from sv status output
    pid=$(echo "$sv_output" | sed -n 's/.*(pid \([0-9]*\)).*/\1/p')
    [ -z "$pid" ] && return 1
    [ ! -d "/proc/$pid" ] && return 1

    # Field 22 is process start time in clock ticks since boot
    start_ticks=$(awk '{print $22}' "/proc/$pid/stat" 2>/dev/null)
    [ -z "$start_ticks" ] && return 1

    # Calculate the corrected service start epoch
    proc_start_sec=$((start_ticks / CLK_TCK))
    correct_start=$((boot_epoch + proc_start_sec))
    real_runtime=$((uptime_sec - proc_start_sec))

    # Build TAI64N timestamp:
    # - 8 bytes TAI64 seconds in big-endian format
    # - 4 bytes nanoseconds, set to zero
    #
    # TAI64 seconds = unix_epoch + 2^62
    # High 32 bits of 2^62 = 0x40000000 = 1073741824
    # Since correct_start < 2^31, tai_lo can directly use correct_start.
    tai_hi=1073741824
    tai_lo=$correct_start

    # Save PID and flags, which are bytes 12-19 in supervise/status
    dd if="$status_file" bs=1 skip=12 count=8 of="/tmp/.sv_tail_${svc}" 2>/dev/null

    # Generate an escaped byte string, then write it as binary data.
    # Using a fixed printf format string avoids ShellCheck SC2059.
    timestamp_bytes=$(printf '\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\\x%02x\\x00\\x00\\x00\\x00' \
        $(( (tai_hi >> 24) & 0xff )) \
        $(( (tai_hi >> 16) & 0xff )) \
        $(( (tai_hi >> 8) & 0xff )) \
        $(( tai_hi & 0xff )) \
        $(( (tai_lo >> 24) & 0xff )) \
        $(( (tai_lo >> 16) & 0xff )) \
        $(( (tai_lo >> 8) & 0xff )) \
        $(( tai_lo & 0xff )))

    printf '%b' "$timestamp_bytes" > "/tmp/.sv_head_${svc}"

    # Combine corrected timestamp with the original PID and flags
    cat "/tmp/.sv_head_${svc}" "/tmp/.sv_tail_${svc}" > "$status_file"

    # Cleanup temporary files
    rm -f "/tmp/.sv_head_${svc}" "/tmp/.sv_tail_${svc}"

    logger -s -p user.info -t fix-sv-time \
        "${svc}: patched runtime ${displayed_runtime}s -> ${real_runtime}s"

    return 0
}

# Iterate through all supervised services
for svc_dir in "$SV_DIR"/*/; do
    [ ! -d "$svc_dir" ] && continue
    svc=$(basename "$svc_dir")
    fix_service "$svc"
done

