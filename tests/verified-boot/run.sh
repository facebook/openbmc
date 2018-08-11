#!/bin/bash

set -e

# This is expected to run in Jenkins.
# This script is not idempotent.

export CACHE="/var/jenkins/sstate-cache"
export BRANCH="krogoth"

sudo DEBIAN_FRONTEND=noninteractive apt-get -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" dist-upgrade
sudo apt-get install -y build-essential wget diffstat chrpath texinfo bzip2 libthread-queue-perl cmake bc libgmp-dev device-tree-compiler

mkdir -p "${CACHE}"

# Build the TPM emulator
export TPM_EMULATOR_BRANCH=hardware-model

git clone -b $TPM_EMULATOR_BRANCH --depth 1 https://github.com/theopolis/tpm-emulator
(cd tpm-emulator; ./build.sh)

# Build QEMU.
export QEMU_BRANCH=aspeed-mods-i2c-tpm

sudo apt-get install -y libpixman-1-dev zlib1g-dev flex bison libfdt1 libfdt-dev python-pip python-dev python-pexpect pkg-config libglib2.0-dev
sudo pip install pycrypto jinja2

git clone -b $QEMU_BRANCH --depth 1 https://github.com/theopolis/qemu
(
  cd qemu
  git clone https://github.com/qemu/dtc
  ./configure --target-list=arm-softmmu
  make -j 4
  cd dtc
  make clean
  make dtc
  sudo ln -fns $(realpath dtc) /usr/local/bin/dtc
)
export QEMU=$PWD/qemu/arm-softmmu/qemu-system-arm

# Checkout and build OpenBMC.
export BOARD=fbtp
export OPENBMC=$PWD

mkdir -p yocto/$BRANCH

# Clone and change directory into Poky.
git clone -b $BRANCH --depth 1 https://git.yoctoproject.org/git/poky yocto/$BRANCH/poky
git clone -b $BRANCH --depth 1 https://github.com/openembedded/meta-openembedded.git yocto/$BRANCH/meta-openembedded
git clone -b $BRANCH --depth 1 https://git.yoctoproject.org/git/meta-security yocto/$BRANCH/meta-security

source openbmc-init-build-env meta-facebook/meta-${BOARD} build-${BOARD}

echo "SSTATE_DIR ?= \"${CACHE}\"" >> ./conf/local.conf

# Stage the U-Boot build.
bitbake -f -c fetch u-boot
bitbake -f -c unpack u-boot

# Patch the code to run in our emulator.
export BUILD_DIR=$PWD
export UBOOT_WORK=$BUILD_DIR/tmp/work/armv6-fb-linux-gnueabi/u-boot/v2016.07-r0/git
cp $OPENBMC/tests/verified-boot/qemu-rom.patch ${UBOOT_WORK}/
(cd $UBOOT_WORK; patch -p1 < qemu-rom.patch)

# Build U-Boot and the image.
bitbake -f u-boot
bitbake ${BOARD}-image

# Get back to the OpenBMC checkout.
cd $OPENBMC

# Run the TPM-emulator.
(cd tpm-emulator; sudo ./build/tpmd/unix/tpmd -d -s ./data -E)

# Create tests from the output.
rm -rf /tmp/outputs
mkdir -p /tmp/outputs
(cd tests/verified-boot; ./create-tests.sh $BUILD_DIR $BUILD_DIR/tmp/deploy/images/fbtp/flash-${BOARD} /tmp/outputs)

# Run the unit-tests.
(cd tests/verified-boot; sudo ./run-tests.py --tpm $QEMU /tmp/outputs flash-${BOARD})

# Clean up the TPM-emulator.
pgrep tpmd | sudo xargs kill

