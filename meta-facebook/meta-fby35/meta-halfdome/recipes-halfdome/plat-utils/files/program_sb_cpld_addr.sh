#!/bin/bash

exp_addr=0x44

if [ $exp_addr = 0x44 ]; then
  addr=0x40
  exp_frs=(0x00 0x00 0x11 0x00 0x00 0x00 0x00 0x00)
else
  addr=0x44
  exp_frs=(0x00 0x00 0x10 0x00 0x00 0x00 0x00 0x00)
fi
exp_idcode="0x61 0x2b 0xc0 0x43"
exp_fbs=(0x02 0x20)
retcode=0

slot=$1

check_busy() {
  dev=$1

  for _ in {1..30}; do
    usleep 100000
    busy=$(i2ctransfer -y "$bus" w4@"$dev" 0xf0 0x00 0x00 0x00 r1 2>/dev/null)
    if [[ "$busy" != "" && $((busy & 0x80)) = 0 ]]; then
      break
    fi
  done
  if [[ "$busy" == "" || $((busy & 0x80)) != 0 ]]; then
    printf "Failed\n"
    exit 1
  fi
}

case $slot in
  slot1)
    bus=4
    ;;
  slot2)
    bus=5
    ;;
  slot3)
    bus=6
    ;;
  slot4)
    bus=7
    ;;
  *)
    N=${0##*/}
    echo "Usage: $N {slot1|slot2|slot3|slot4}"
    exit 1
    ;;
esac

# Check if already programmed
idcode=$(i2ctransfer -y $bus w4@$exp_addr 0xe0 0x00 0x00 0x00 r4 2>/dev/null)
if [ "$idcode" = "$exp_idcode" ]; then
  addr=$exp_addr
  i2ctransfer -y $bus w3@$addr 0x74 0x08 0x00 >/dev/null 2>&1
  frs=$(i2ctransfer -y $bus w4@$addr 0xe7 0x00 0x00 0x00 r8 2>/dev/null)
  fbs=$(i2ctransfer -y $bus w4@$addr 0xfb 0x00 0x00 0x00 r2 2>/dev/null)
  i2ctransfer -y $bus w3@$addr 0x26 0x00 0x00 >/dev/null 2>&1
  if [[ "$frs" = "${exp_frs[*]}" && "$fbs" = "${exp_fbs[*]}" ]]; then
    echo "Already programmed!"
    echo "Feature Rows: $frs"
    echo "FEABITS: $fbs"
    exit 0
  fi
fi


# Check the IDCODE
idcode=$(i2ctransfer -y $bus w4@$addr 0xe0 0x00 0x00 0x00 r4)
echo "IDCODE: $idcode"
if [ "$idcode" != "$exp_idcode" ]; then
  echo "Unexpected IDCODE!"
  exit 1
fi

# Check the STATUS
read -r -a sts <<< "$(i2ctransfer -y $bus w4@$addr 0x3c 0x00 0x00 0x00 r4)"
echo "Status Register: ${sts[*]}"
if [[ $((sts[2] & 0x70)) != 0 || $((sts[3] & 0x20)) != 0 ]]; then
  echo "Unexpected Status!"
  exit 1
fi


printf "Enable XPROGRAM mode..."
if ! i2ctransfer -y $bus w3@$addr 0x74 0x08 0x00 >/dev/null; then
  printf "Failed\n"
  exit 1
fi
printf "OK\n"

frs=$(i2ctransfer -y $bus w4@$addr 0xe7 0x00 0x00 0x00 r8)
echo "Feature Rows before programming: $frs"

fbs=$(i2ctransfer -y $bus w4@$addr 0xfb 0x00 0x00 0x00 r2)
echo "FEABITS before programming: $fbs"


printf "Erase the Feature Rows..."
i2ctransfer -y $bus w4@$addr 0x0e 0x02 0x00 0x00 >/dev/null
addr=0x40  # address become to default
check_busy $addr
printf "OK\n"

printf "Program Feature Rows..."
if ! i2ctransfer -y $bus w4@$addr 0x46 0x02 0x00 0x00 >/dev/null; then
  printf "Failed\n"
  exit 1
fi

i2ctransfer -y $bus w12@$addr 0xe4 0x00 0x00 0x00 "${exp_frs[@]}" >/dev/null
addr=$exp_addr
check_busy $addr
frs=$(i2ctransfer -y $bus w4@$addr 0xe7 0x00 0x00 0x00 r8)

i2ctransfer -y $bus w6@$addr 0xf8 0x00 0x00 0x00 "${exp_fbs[@]}" >/dev/null
check_busy $addr
fbs=$(i2ctransfer -y $bus w4@$addr 0xfb 0x00 0x00 0x00 r2)

if [[ "$frs" = "${exp_frs[*]}" && "$fbs" = "${exp_fbs[*]}" ]]; then
  printf "OK\n"
else
  printf "Failed\n"
  retcode=1
fi
echo "Feature Rows after programming: $frs"
echo "FEABITS after programming: $fbs"


printf "Exit the programming mode..."
if ! i2ctransfer -y $bus w3@$addr 0x26 0x00 0x00 >/dev/null; then
  printf "Failed\n"
  exit 1
fi
printf "OK\n"
exit $retcode

