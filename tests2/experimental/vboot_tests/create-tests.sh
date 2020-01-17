#!/usr/bin/env bash

#  Copyright (c) 2014-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.

set -e

SOURCE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPTS="$SOURCE/../tools/signing"

# Option parsing.
if [ "$#" -lt 3 ]; then
  echo "Invalid parameters:"
  echo "./build.sh POKY_BUILD INPUT_FLASH OUTPUT_DIR [-e]"
  echo ""
  echo "  POKY_BUILD: Directory of build-dir within poky"
  echo "  INPUT_FLASH: The input flash-BOARD file"
  echo "  OUTPUT_DIR: Existing directory that will contain output flashes"
  echo "  [-e] Create failure tests from a signed input-flash"
  exit 1
fi

# Run a series of tests for installed applications.
PYTHON=`which python2 || true`
if [ "$PYTHON" = "" ]; then
  echo "Error: cannot find 'python2'"
  exit 1
fi

DTC=`which dtc || true`
if [ "$DTC" = "" ]; then
  echo "Error: cannot find 'dtc' (please install device-tree-compiler)"
  exit 1
fi

OPENSSL=`which openssl || true`
if [ "$OPENSSL" = "" ]; then
  echo "Error: cannot find 'openssl'"
  exit 1
fi

PYCRYPTO=`$PYTHON -c 'import Crypto' 2>&1 &> /dev/null; echo $?`
if [ ! "$PYCRYPTO" = "0" ]; then
  echo "Error: Python ($PYTHON) cannot import 'Crypto' (install python-crypto)"
  exit 1
fi

JINJA2=`$PYTHON -c 'import jinja2' 2>&1 &> /dev/null; echo $?`
if [ ! "$JINJA2" = "0" ]; then
  echo "Error: Python ($PYTHON) cannot import 'jinja2' (install python-jinja2)"
  exit 1
fi

POKY_BUILD="$1"
if [ ! -d "$POKY_BUILD" ]; then
  echo "Error: the POKY_BUILD argument ($POKY_BUILD) does not exist?"
  exit 1
fi

MKIMAGE="$POKY_BUILD/tmp/sysroots/x86_64-linux/usr/bin/mkimage"
if [ ! -f "$MKIMAGE" ]; then
  echo "Error: cannot find mkimage at ($MKIMAGE) have you run 'bitbake MACHINE-image'?"
  echo "Error: you should run 'bitbake' in the ($POKY_BUILD) directory first"
  exit 1
fi

INPUT_FLASH="$2"
if [ ! -f "$INPUT_FLASH" ]; then
  echo "Error: the INPUT_FLASH argument ($INPUT_FLASH) does not exist?"
  exit 1
fi
INPUT_NAME=`basename $INPUT_FLASH`

OUTPUT_DIR="$3"
OUTPUT_DIR=`realpath $OUTPUT_DIR || echo $OUTPUT_DIR`

EASY_TESTS=0
if [ ! -z "$4" ]; then
  EASY_TESTS=1
fi

DIRTY=`find $OUTPUT_DIR | wc -l`
if [ ! "$DIRTY" = "1" ]; then
  echo "Note: the OUTPUT_DIR ($OUTPUT_DIR) is not empty..."
fi

if [ ! -d "$OUTPUT_DIR" ]; then
  echo "Error: the OUTPUT_DIR argument ($OUTPUT_DIR) does not exist?"
  exit 1
fi

# Output all of the discovered paths.
echo -e "Using scripts: \t$SCRIPTS"
echo -e "Using build: \t$POKY_BUILD"
echo -e "Using flash: \t$INPUT_FLASH"
echo -e "Using output: \t$OUTPUT_DIR"
echo -e "Using python2: \t$PYTHON"
echo -e "Using dtc: \t$DTC"
echo -e "Using openssl: \t$OPENSSL"
echo ""

function easy_corrupt_uboot() {
  INPUT=$1
  OUTPUT=$2
  cp $INPUT $OUTPUT
  dd if=/dev/random of=$OUTPUT bs=1 seek=540772 count=16 conv=notrunc
}

function easy_corrupt_kernel() {
  INPUT=$1
  OUTPUT=$2
  cp $INPUT $OUTPUT
  dd if=/dev/random of=$OUTPUT bs=1 seek=921600 count=16 conv=notrunc

}

if [ "$EASY_TESTS" = "1" ]; then
  echo "[+] Creating simple failure case tests"
  echo ""
  easy_corrupt_uboot $INPUT_FLASH $OUTPUT_DIR/$INPUT_NAME.corrupt_uboot
  easy_corrupt_kernel $INPUT_FLASH $OUTPUT_DIR/$INPUT_NAME.corrupt_kernel
  echo " Wrote: $OUTPUT_DIR/$INPUT_NAME.corrupt_uboot"
  echo " Wrote: $OUTPUT_DIR/$INPUT_NAME.corrupt_kernel"
  exit 0
fi

# Create the example KEK (ROM root key).
mkdir -p "$OUTPUT_DIR/kek"
openssl genrsa -F4 -out "$OUTPUT_DIR/kek/kek.key" 4096
openssl rsa -in "$OUTPUT_DIR/kek/kek.key" -pubout > "$OUTPUT_DIR/kek/kek.pub"
echo -e "\nCreated KEK (ROM key): $OUTPUT_DIR/kek/kek.{key,pub}\n"

# Create the example ODM keypair (U-Boot, Kernel, rootfs signing key).
mkdir -p "$OUTPUT_DIR/odm0"
openssl genrsa -F4 -out "$OUTPUT_DIR/odm0/odm0.key" 4096
openssl rsa -in "$OUTPUT_DIR/odm0/odm0.key" -pubout > "$OUTPUT_DIR/odm0/odm0.pub"
echo -e "\nCreated ODM0 key (used to signed U-Boot and Linux): $OUTPUT_DIR/odm0/odm0.{key,pub}\n"

mkdir -p "$OUTPUT_DIR/odm1"
openssl genrsa -F4 -out "$OUTPUT_DIR/odm1/odm1.key" 4096
openssl rsa -in "$OUTPUT_DIR/odm1/odm1.key" -pubout > "$OUTPUT_DIR/odm1/odm1.pub"
echo -e "\nCreated (optional) ODM key (used to signed U-Boot and Linux): $OUTPUT_DIR/odm1/odm1.{key,pub}\n"

mkdir -p "$OUTPUT_DIR/subordinates"
cp "$OUTPUT_DIR/odm0/odm0.pub" "$OUTPUT_DIR/subordinates"
cp "$OUTPUT_DIR/odm1/odm1.pub" "$OUTPUT_DIR/subordinates"

# Create certificate stores and the signed subordinate store.
$SCRIPTS/fit-cs --template $SCRIPTS/store.dts.in $OUTPUT_DIR/kek $OUTPUT_DIR/kek/kek.dtb
$SCRIPTS/fit-cs --template $SCRIPTS/store.dts.in --subordinate --subtemplate $SCRIPTS/sub.dts.in \
  $OUTPUT_DIR/subordinates $OUTPUT_DIR/subordinates/subordinates.dtb
$SCRIPTS/fit-signsub --mkimage $MKIMAGE --keydir $OUTPUT_DIR/kek \
  $OUTPUT_DIR/subordinates/subordinates.dtb $OUTPUT_DIR/subordinates/subordinates.dtb.signed

mkdir -p $OUTPUT_DIR/flashes
rm -f $OUTPUT_DIR/flashes/*

FLASH_UNSIGNED=$OUTPUT_DIR/flashes/$INPUT_NAME.unsigned
FLASH_SIGNED=$OUTPUT_DIR/flashes/${INPUT_NAME}.signed
cp $INPUT_FLASH $FLASH_UNSIGNED

# Sign the input flash.
$SCRIPTS/fit-sign --mkimage $MKIMAGE --kek $OUTPUT_DIR/kek/kek.dtb \
  --sign-os \
  --signed-subordinate $OUTPUT_DIR/subordinates/subordinates.dtb.signed \
  --keydir $OUTPUT_DIR/odm0 $FLASH_UNSIGNED $FLASH_SIGNED

# Time to generate all test cases.
FLASH_SIGNED=$INPUT_NAME.signed
FLASH_UNSIGNED=$INPUT_NAME.unsigned
SIZE=$(ls -l $OUTPUT_DIR/flashes/$FLASH_SIGNED | cut -d " " -f5)
SIZE=$(expr $SIZE / 1024)

# Place all error intermediate assets in ./error
mkdir -p "$OUTPUT_DIR/error"

# 0.0 Success case
echo "[+] Creating 0.0..."
ln -sf $FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS0.0.0
ln -sf $FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.0.0

# 3.30 Blank CS1
echo "[+] Creating 3.30..."
ln -sf $FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS0.3.30
touch $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.3.30
dd if=/dev/zero of=$OUTPUT_DIR/flashes/$INPUT_NAME.CS1.3.30 bs=1k count=$SIZE

# Extract 0x8:0000 - 0x8:4000 (U-Boot FIT) for modifications
dd if=$OUTPUT_DIR/flashes/$FLASH_SIGNED of=$OUTPUT_DIR/error/u-boot.dtb bs=1k skip=512 count=16
UBOOT_DTS=$OUTPUT_DIR/error/u-boot.dts
dtc -I dtb -O dts $OUTPUT_DIR/error/u-boot.dtb -o $UBOOT_DTS

# Extract the OS for modifications
dd if=$OUTPUT_DIR/flashes/$FLASH_SIGNED of=$OUTPUT_DIR/error/os.dtb bs=1k skip=896
OS_DTS=$OUTPUT_DIR/error/os.dts
dtc -I dtb -O dts $OUTPUT_DIR/error/os.dtb -o $OS_DTS

function edit_dtb() {
  SEEK=$1
  FLASH=$2
  DTS=$3
  dtc -f -I dts -O dtb $DTS -o /tmp/edit.dtb
  dd if=/tmp/edit.dtb of=$FLASH seek=$SEEK bs=1k conv=notrunc
  rm /tmp/edit.dtb
}

function create_bad_fit() {
  T=$1
  N=$2
  EXPR="$3"
  if [ "$T" = "os" ]; then
    D=$OS_DTS
    S=896
  else
    D=$UBOOT_DTS
    S=512
  fi

  echo "[+] Creating ($T) $N..."
  cp $D $OUTPUT_DIR/error/$T.$N.dts
  sed -i "$EXPR" $OUTPUT_DIR/error/$T.$N.dts
  cp $OUTPUT_DIR/flashes/$FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.$N
  edit_dtb $S $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.$N $OUTPUT_DIR/error/$T.$N.dts
  ln -sf $FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS0.$N
}

create_bad_fit "u-boot" "3.31" 's/images {/images1 {/g'
create_bad_fit "u-boot" "3.32" 's/images {/images { }; noimages {/g'
create_bad_fit "u-boot" "3.33" 's/configurations {/configurations1 {/g'

# 34 is difficult, need to remove gd() from CS0

create_bad_fit "u-boot" "3.35" 's/keys {/keys1 {/g'
create_bad_fit "u-boot" "3.36" 's/data = <0xd00dfeed/data2 = <0xd00dfeed/g'
create_bad_fit "u-boot" "3.37.1" 's/data-position =/data2-position =/g'
create_bad_fit "u-boot" "3.37.2" 's/data-size =/data2-size =/g'
create_bad_fit "u-boot" "3.37.3" 's/data-size = <\(.*\)>;/data-size = <0xDDDDDDDD>;/g'
create_bad_fit "u-boot" "3.38" 's/data = <\(.*\)>;/data = <\1 \1 \1 \1 \1 \1 \1 \1\ \1 \1 \1>;/g'

# Generate keys needed for failure cases.
mkdir -p "$OUTPUT_DIR/error/kek"
openssl genrsa -F4 -out "$OUTPUT_DIR/error/kek/kek.key" 4096
openssl rsa -in "$OUTPUT_DIR/error/kek/kek.key" -pubout > "$OUTPUT_DIR/error/kek/kek.pub"
echo -e "\nCreated error-KEK (ROM key): $OUTPUT_DIR/error/kek/kek.{key,pub}\n"

mkdir -p "$OUTPUT_DIR/error/subordinates"
$SCRIPTS/fit-signsub --mkimage $MKIMAGE --keydir $OUTPUT_DIR/error/kek \
  $OUTPUT_DIR/subordinates/subordinates.dtb $OUTPUT_DIR/error/subordinates/subordinates.dtb.signed

echo "[+] Creating 4.40..."
$SCRIPTS/fit-sign --mkimage $MKIMAGE --kek $OUTPUT_DIR/kek/kek.dtb \
  --signed-subordinate $OUTPUT_DIR/error/subordinates/subordinates.dtb.signed \
  --keydir $OUTPUT_DIR/odm0 $OUTPUT_DIR/flashes/$FLASH_UNSIGNED \
  $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.4.40.1
ln -sf $FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS0.4.40.1

create_bad_fit "u-boot" "4.40.2" 's/key-name-hint = "kek"/key-name-hint = "not-kek"/g'
create_bad_fit "u-boot" "4.40.3" 's/data = <0xd00dfeed/data = <0xd00dfefd/g'

# Test-tests
# create_bad_fit "u-boot" "0.0.1" 's/key-name-hint = "odm0"/key-name-hint = "odm0"/g'
# create_bad_fit "os" "0.0.2" 's/key-name-hint = "odm0"/key-name-hint = "odm0"/g'

echo "[+] Creating 4.43..."
easy_corrupt_uboot $OUTPUT_DIR/flashes/$FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.4.43.1
ln -sf $FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS0.4.43.1

create_bad_fit "u-boot" "4.43.2" 's/key-name-hint = "odm0"/key-name-hint = "kek"/g'
create_bad_fit "u-boot" "4.43.3" 's/key-name-hint = "odm0"/key-name-hint = "odm1"/g'
create_bad_fit "u-boot" "4.43.4" 's/compression = "none";/compression = "none";\n\n\t\t\thash@2 { algo = "sha256"; };/g'

# The timestamp is part of the signature.
create_bad_fit "u-boot" "4.43.5" 's/timestamp = <\(.*\)>;/timestamp = <0x10>;/g'

BAD_HASH="0x10 0x10 0x10 0x10 0x10 0x10 0x10 0x10"
MATCH='compression = "none";\(.*\)value = <\(.*\)>;\n\t\t\t\talgo\(.*\)config'
MATCH=":a;N;\$!ba;s/${MATCH}/compression = \"none\";\\1value = "
MATCH="${MATCH}<${BAD_HASH}>;\\n\\t\\t\\t\\talgo\\3/g"
create_bad_fit "u-boot" "4.43.6" "$MATCH"

create_bad_fit "os" "6.60.1" 's/key-name-hint = "odm0"/key-name-hint = "bad"/g'
create_bad_fit "os" "6.60.2" 's/key-name-hint = "odm0"/key-name-hint = "odm1"/g'
create_bad_fit "os" "6.60.3" 's/timestamp = <\(.*\)>;/timestamp = <0x10>;/g'

echo "[+] Creating 6.60.4..."
easy_corrupt_kernel $OUTPUT_DIR/flashes/$FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.6.60.4

echo "[+] Creating ODM1 signed 0.0.3"
$SCRIPTS/fit-sign --mkimage $MKIMAGE --kek $OUTPUT_DIR/kek/kek.dtb \
  --signed-subordinate $OUTPUT_DIR/subordinates/subordinates.dtb.signed \
  --keydir $OUTPUT_DIR/odm1 $OUTPUT_DIR/flashes/$FLASH_UNSIGNED \
  --sign-os \
  $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.0.0.3
ln -sf $FLASH_SIGNED $OUTPUT_DIR/flashes/$INPUT_NAME.CS0.0.0.3

# Create newly-signed (correct) firmware for fallback checks
$SCRIPTS/fit-sign --mkimage $MKIMAGE --kek $OUTPUT_DIR/kek/kek.dtb \
  --sign-os \
  --signed-subordinate $OUTPUT_DIR/subordinates/subordinates.dtb.signed \
  --keydir $OUTPUT_DIR/odm0 \
  $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.0.0 $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.next.1

$SCRIPTS/fit-sign --mkimage $MKIMAGE --kek $OUTPUT_DIR/kek/kek.dtb \
  --sign-os \
  --signed-subordinate $OUTPUT_DIR/subordinates/subordinates.dtb.signed \
  --keydir $OUTPUT_DIR/odm0 \
  $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.0.0 $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.next.2

# This invalidates the signature but sets the timestamp too far into the future.
create_bad_fit "u-boot" "4.43.5.1" 's/timestamp = <\(.*\)>;/timestamp = <0xEE6B2800>;/g'

# Create a subordinate that does not include the ODM1 key.
$SCRIPTS/fit-cs --template $SCRIPTS/store.dts.in --subordinate --subtemplate $SCRIPTS/sub.dts.in \
  $OUTPUT_DIR/odm0 $OUTPUT_DIR/odm0/odm0.dtb
$SCRIPTS/fit-signsub --mkimage $MKIMAGE --keydir $OUTPUT_DIR/kek \
  $OUTPUT_DIR/odm0/odm0.dtb $OUTPUT_DIR/odm0/odm0.dtb.signed

# Sign an image that does not include the ODM1 key at least 1s into the future.
$SCRIPTS/fit-sign --mkimage $MKIMAGE --kek $OUTPUT_DIR/kek/kek.dtb \
  --sign-os \
  --signed-subordinate $OUTPUT_DIR/odm0/odm0.dtb.signed \
  --keydir $OUTPUT_DIR/odm0 \
  $OUTPUT_DIR/flashes/$INPUT_NAME.unsigned $OUTPUT_DIR/flashes/$INPUT_NAME.CS1.future
ln -sf $INPUT_NAME.CS1.future $OUTPUT_DIR/flashes/$INPUT_NAME.CS0.future

echo -e "\n\nThe test firmware images are now located in $OUTPUT_DIR/flashes"
