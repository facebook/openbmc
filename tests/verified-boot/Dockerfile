FROM ubuntu:14.04

RUN apt-get update && apt-get install -y \
    git \
    wget \
    ruby \
    ruby-dev \
    python \
    python-dev \
    build-essential \
    curl

RUN mkdir /opt/spl-automate
RUN mkdir /opt/spl-automate/content

ENV QEMU_BRANCH=aspeed-mods
ENV UBOOT_BRANCH=openbmc/helium/v2016.07
RUN git clone -b $QEMU_BRANCH https://github.com/theopolis/qemu /opt/spl-automate/qemu
RUN git clone -b $UBOOT_BRANCH https://github.com/theopolis/u-boot /opt/spl-automate/u-boot

# These are QEMU-specific dependencies and required tools.
RUN apt-get update && apt-get install -y \
    pkg-config \
    zlib1g-dev \
    libglib2.0-dev \
    libfdt-dev \
    autoconf \
    libtool

# Initialize additional development dependencies for QEMU.
RUN (cd /opt/spl-automate/qemu; git submodule update --init pixman)
RUN (cd /opt/spl-automate/qemu; ARCH=arm CROSS_COMPILE=arm-none-eabi- ./configure --target-list=arm-softmmu)
RUN (cd /opt/spl-automate/qemu; make -j 4)
ENV QEMU=/opt/spl-automate/qemu/arm-softmmu/qemu-system-arm

# Install dependencies for building U-Boot for ARM.
RUN apt-get update && apt-get install -y \
    gcc-arm-none-eabi \
    libssl-dev \
    device-tree-compiler \
    bc \
    python-pip

# Install the FIT certificate store generator and its Python dependencies.
RUN git clone https://github.com/theopolis/fit-certificate-store /opt/spl-automate/fit-certificate-store
RUN pip install -U pip && pip install jinja2 pycrypto

# We will need a KEK public key for the normal build, which produces a ROM.
# This is copying a pre-generated key.
RUN mkdir /opt/spl-automate/kek
RUN openssl genrsa -F4 -out /opt/spl-automate/kek/kek.key 4096
RUN openssl rsa -in /opt/spl-automate/kek/kek.key -pubout > /opt/spl-automate/kek/kek.pub

# Create a DTB certificate store to be included in the embedded U-Boot images.
ENV ROM_STORE=/opt/spl-automate/content/rom-store.dtb
RUN /opt/spl-automate/fit-certificate-store/fit-cs.py \
  --template /opt/spl-automate/fit-certificate-store/store.dts.in \
  --required-node image /opt/spl-automate/kek $ROM_STORE

# Small patch to build U-Boot for verified-boot outside of Poky and for QEMU.
COPY qemu-rom.patch /opt/spl-automate/qemu-rom.patch

# Enable features for FBTP to build (1) in QEMU and (2) an SPL ROM.
RUN (cd /opt/spl-automate/u-boot; git apply ../qemu-rom.patch)

# Define a "Normal" U-Boot build, to be executed after verification by the ROM.
ENV NORMAL=/opt/spl-automate/u-boot/build-normal
RUN mkdir -p $NORMAL

# Compiler and configuration for U-Boot.
ENV AMAKE="make ARCH=arm CROSS_COMPILE=arm-none-eabi- -j4"
ENV CONFIG=fbtp_config

# The Normal U-Boot will contain the ROM-store but will not use it in practice.
RUN (cd /opt/spl-automate/u-boot; $AMAKE O=$NORMAL -s $CONFIG)
RUN (cd /opt/spl-automate/u-boot; $AMAKE O=$NORMAL EXT_DTB=$ROM_STORE)

# Define a "Recovery" U-Boot build.
ENV RECOVERY=/opt/spl-automate/u-boot/build-recovery
RUN mkdir -p $RECOVERY

# Now build the Recovery U-Boot.
RUN (cd /opt/spl-automate/u-boot; $AMAKE O=$RECOVERY -s $CONFIG)
RUN echo "CONFIG_ASPEED_RECOVERY_BUILD=y" >> $RECOVERY/.config
RUN (cd /opt/spl-automate/u-boot; $AMAKE O=$RECOVERY EXT_DTB=$ROM_STORE)

# We now need a subordinate key that will be signed by the KEK.
# This is copying a pre-generated key.
RUN mkdir -p /opt/spl-automate/subordinate
RUN openssl genrsa -F4 -out /opt/spl-automate/subordinate/subordinate.key 4096
RUN openssl rsa -in /opt/spl-automate/subordinate/subordinate.key -pubout > /opt/spl-automate/subordinate/subordinate.pub
#  mkdir -p subordinate
#  openssl genrsa -F4 -out subordinate/subordinate.key 4096
#  openssl rsa -in subordinate/subordinate.key -pubout > subordinate/subordinate.pub
# COPY subordinate /opt/spl-automate/subordinate

# Bring in a customized DTS describing the Normal U-Boot as a target.
COPY u-boot.dts /opt/spl-automate/u-boot.dts

# Build an image that starts with a FIT, with signature data.
RUN $NORMAL/tools/mkimage \
  -f /opt/spl-automate/u-boot.dts \
  -k /opt/spl-automate/subordinate \
  -E -p 0x4000 -r $NORMAL/u-boot.img;

# Construct the firmware image (the RO and RW are symmetrical).
RUN dd if=/dev/zero of=/opt/spl-automate/content/flash0 bs=1k count=32768
RUN dd if=$RECOVERY/spl/u-boot-spl.bin of=/opt/spl-automate/content/flash0 conv=notrunc
RUN dd if=$RECOVERY/u-boot.bin of=/opt/spl-automate/content/flash0 bs=1k seek=64 conv=notrunc
RUN dd if=$NORMAL/u-boot.img of=/opt/spl-automate/content/flash0 bs=1k seek=512 conv=notrunc
RUN cp /opt/spl-automate/content/flash0 /opt/spl-automate/content/flash1

# Generate a subordinate key store for our subordinate keys.
# This is usually a 1-time operation performed offline when the KEK is available.
ENV SUBORDINATE_STORE=/opt/spl-automate/content/subordinate-store.dtb
ENV SUBORDINATE_STORE_SOURCE=/opt/spl-automate/content/subordinate-store.dts
ENV SUBORDINATE_STORE_SIGNED=/opt/spl-automate/content/subordinate-store.dtb.signed
RUN /opt/spl-automate/fit-certificate-store/fit-cs.py \
  --template /opt/spl-automate/fit-certificate-store/store.dts.in \
  --required-node image \
  --subordinate --subtemplate /opt/spl-automate/fit-certificate-store/sub.dts.in \
  /opt/spl-automate/subordinate $SUBORDINATE_STORE

# This is the second part of the 1-time operation, signing the subordinate store
# using the KEK private key.
RUN dtc -I dtb -O dts $SUBORDINATE_STORE -o $SUBORDINATE_STORE_SOURCE
RUN $NORMAL/tools/mkimage \
  -f $SUBORDINATE_STORE_SOURCE \
  -k /opt/spl-automate/kek -r $SUBORDINATE_STORE_SIGNED

# The second RW image needs the subordinate public key placed into the FIT.
# It also needs a signature of those keys, signed by the KEK.
RUN /opt/spl-automate/fit-certificate-store/fit-sign.py \
  --mkimage $NORMAL/tools/mkimage /opt/spl-automate/content/flash1 \
  --subordinate $SUBORDINATE_STORE_SIGNED \
  --keydir /opt/spl-automate/subordinate /opt/spl-automate/content/firmware.signed
RUN cp /opt/spl-automate/content/flash1 /opt/spl-automate/content/flash1.signed
RUN dd if=/opt/spl-automate/content/firmware.signed of=/opt/spl-automate/content/flash1.signed bs=1k conv=notrunc

# Aside from the last few lines related to running the test harness, the following
# construct various forms of invalid/incorrect/missing data to test error cases.

# Corrupt the U-Boot FIT structure
RUN cp /opt/spl-automate/content/flash1.signed /opt/spl-automate/content/flash1.corrupted-fit
RUN dd if=/dev/zero of=/opt/spl-automate/content/flash1.corrupted-fit bs=1k seek=513 count=1 conv=notrunc

# Corrupt U-Boot
RUN cp /opt/spl-automate/content/flash1.signed /opt/spl-automate/content/flash1.corrupted-uboot
RUN dd if=/dev/zero of=/opt/spl-automate/content/flash1.corrupted-uboot bs=1k seek=529 count=1 conv=notrunc

# Construct another signed U-Boot without the subordinate information.
RUN /opt/spl-automate/fit-certificate-store/fit-sign.py \
  --mkimage $NORMAL/tools/mkimage /opt/spl-automate/content/flash1 \
  --keydir /opt/spl-automate/subordinate /opt/spl-automate/content/firmware.missing-subordinate
RUN cp /opt/spl-automate/content/flash1 /opt/spl-automate/content/flash1.missing-subordinate
RUN dd if=/opt/spl-automate/content/firmware.missing-subordinate of=/opt/spl-automate/content/flash1.missing-subordinate bs=1k conv=notrunc

# We stage some "fake" or invalid keys also named "subordinate".
RUN mkdir -p /opt/spl-automate/fake/subordinate
RUN openssl genrsa -F4 -out /opt/spl-automate/fake/subordinate/subordinate.key 4096
RUN openssl rsa -in /opt/spl-automate/fake/subordinate/subordinate.key -pubout > /opt/spl-automate/fake/subordinate/subordinate.pub

# Create a fake or invalid intermediate store.
RUN /opt/spl-automate/fit-certificate-store/fit-cs.py \
  --template /opt/spl-automate/fit-certificate-store/store.dts.in \
  --required-node image \
  --subordinate --subtemplate /opt/spl-automate/fit-certificate-store/sub.dts.in \
  /opt/spl-automate/fake/subordinate /opt/spl-automate/content/subordinate-store.fake.dtb
RUN dtc -I dtb -O dts /opt/spl-automate/content/subordinate-store.fake.dtb \
  -o /opt/spl-automate/content/subordinate-store.fake.dts
RUN $NORMAL/tools/mkimage \
  -f /opt/spl-automate/content/subordinate-store.fake.dts \
  -k /opt/spl-automate/subordinate -r /opt/spl-automate/content/subordinate-store.fake.dtb.signed

# Sign the flash content with a real subordinate, but attach an different subordinate store.
RUN /opt/spl-automate/fit-certificate-store/fit-sign.py \
  --mkimage $NORMAL/tools/mkimage /opt/spl-automate/content/flash1 \
  --subordinate /opt/spl-automate/content/subordinate-store.fake.dtb.signed \
  --keydir /opt/spl-automate/fake/subordinate /opt/spl-automate/content/firmware.fake-subordinate
RUN cp /opt/spl-automate/content/flash1 /opt/spl-automate/content/flash1.fake-subordinate
RUN dd if=/opt/spl-automate/content/firmware.fake-subordinate of=/opt/spl-automate/content/flash1.fake-subordinate bs=1k conv=notrunc

# Sign the flash content with a fake subordinate, but attach the correct subordinate store.
RUN /opt/spl-automate/fit-certificate-store/fit-sign.py \
  --mkimage $NORMAL/tools/mkimage /opt/spl-automate/content/flash1 \
  --subordinate $SUBORDINATE_STORE_SIGNED \
  --keydir /opt/spl-automate/fake/subordinate /opt/spl-automate/content/firmware.fake-signature
RUN cp /opt/spl-automate/content/flash1 /opt/spl-automate/content/flash1.fake-signature
RUN dd if=/opt/spl-automate/content/firmware.fake-signature of=/opt/spl-automate/content/flash1.fake-signature bs=1k conv=notrunc

# Use pexpect to automate testing of QEMU.
RUN pip install pexpect

# Need several additional debugging utils.
RUN apt-get update && apt-get install -y -f bsdmainutils

# Copy in a debug helper and the test harness.
COPY tests.py /opt/spl-automate/tests.py
RUN /opt/spl-automate/tests.py
