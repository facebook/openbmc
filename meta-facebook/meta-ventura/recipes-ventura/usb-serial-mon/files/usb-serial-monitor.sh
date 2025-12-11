#!/bin/bash

check_device() {
  addr=$1
  if ! rackmoncli read "$addr" 0x9000 > /dev/null; then
    return 1
  fi
  return 0
}

is_problem_usb() {
  cnt=$(lsusb 2> /dev/null | grep -c "Exar USB UART")
  if [[ $cnt -gt 0 ]]; then
    return 0
  fi
  return 1
}

detect_condition() {
  ret=0
  addrs="268 524 780"
  detected=$(rackmoncli list --json | jq '.[].uniqueDevAddress')
  for addr in $addrs; do
    if [[ "$detected" == *"$addr"* ]]; then
      echo "Detected $addr checking it"
      if ! check_device "$addr"; then
        echo "Device $addr failed"
        ret=1
      fi
    fi
  done
  return $ret
}

is_discovered() {
  addrs="268 524 780"
  detected=$(rackmoncli list --json | jq '.[].uniqueDevAddress')
  for addr in $addrs; do
    if [[ "$detected" != *"$addr"* ]]; then
      return 1
    fi
  done
}

remediation_step() {
  echo "usb2" > /sys/bus/usb/drivers/usb/unbind
  sleep 1
  echo "1e6b0000.usb" > /sys/bus/platform/drivers/platform-uhci/unbind
  sleep 1
  echo "usb2" > /sys/bus/usb/drivers/usb/bind
  sleep 1
  echo "1e6b0000.usb" > /sys/bus/platform/drivers/platform-uhci/bind
  sleep 1
  systemctl restart rackmond
}

remediate() {
  remediation_step
  for retry in $(seq 10); do
    sleep 60
    if is_discovered; then
      if detect_condition; then
        echo "Remediated on ${retry} try"
        return 0
      else
        echo "Detected on ${retry} try but still exhibiting condition"
        break
      fi
    fi
  done
  return 1
}

if ! is_problem_usb; then
  # Skip all further checks if we dont have to
  exit 0
fi
DONE_FILE=/tmp/usb-serial-issue.txt
if [ -e $DONE_FILE ]; then
  current_time=$(date +%s)
  done_file_time=$(stat -c %Y "$DONE_FILE")
  time_diff=$((current_time - done_file_time))

  if [ "$time_diff" -gt 3600 ]; then
    # Reboot if we have been broken for an hour
    echo "Time to hard reboot"
    reboot
  elif [ "$time_diff" -lt 1800 ]; then
    # Don't keep doing this. We already discovered its bad recently.
    exit 0
  fi
fi
if ! detect_condition; then
  just_created_done_file=false
  if [ ! -e $DONE_FILE ]; then
    just_created_done_file=true
    date > $DONE_FILE
  fi
  if remediate; then
    echo "Auto-remediation successful!"
    rm -f $DONE_FILE
  else
    if $just_created_done_file; then
      echo "Auto-remediation failed!"
      systemctl start managed-reboot
    fi
  fi
fi
