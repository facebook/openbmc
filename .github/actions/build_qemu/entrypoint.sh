#!/bin/bash

set -e

git config --global --add safe.directory "$PWD"
git config --global user.name "openbmc"
git config --global user.email "openbmc@meta.com"
./sync_yocto.sh
. ./openbmc-init-build-env greatlakes build
touch conf/sanity.conf
devtool modify qemu-system-native
cd workspace/sources/qemu-system-native
./configure \
    --target-list=aarch64-softmmu \
    --with-git-submodules=ignore \
    --without-default-features \
    --disable-debug-info \
    --disable-install-blobs \
    --enable-strip \
    --enable-tpm \
    --enable-slirp=internal
ninja -C build
./build/qemu-system-aarch64 -h
