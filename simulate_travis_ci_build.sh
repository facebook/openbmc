#!/bin/bash

# Script to naively emmulate what Travis CI does
# for testing warrior release changes on OpenBMC

# Reset necessary variables
unset TEMPLATECONF
unset override
unset override_plat
unset override_distro
unset platform
unset DISTRO_OVERRIDES

# Remove previous builds just to make sure we can build from scratch
# Comment the following line if building from scratch is not desired
rm -rf build*

# Get a top directory to share downloads amongst builds
TPDIR=$PWD
if [ ! -d $TPDIR/downloads ]; then
    mkdir $TPDIR/downloads
fi

# DISTRO_DEFAULT gets reset when sourcing openbmc-init-build-env
# so we need to keep the initial chosen one
DEF_RELEASE=$DISTRO_DEFAULT

# Boards that get tested on Travis CI by default, more could be easily added
if [ -z "$BOARDS" ]; then
    BOARDS="wedge wedge100 yosemite lightning cmm galaxy100 fbtp fbttn fby2"
fi

echo "Starting OpenBMC build for the following BOARDS: $BOARDS"
echo ""

for i in $BOARDS;
  do
    export BOARD="$i"
    echo "Building OpenBMC for $BOARD on $DISTRO_DEFAULT release"
    echo ""

    DISTRO_DEFAULT=$DEF_RELEASE source ./openbmc-init-build-env meta-facebook/meta-${BOARD} build-${BOARD}

    # Share downloads if this is a new build directory
    if [ ! -h $PWD/downloads ]; then
        ln -s $TPDIR/downloads .
    fi
    if [ "$BOARD" == "fbal" ]; then
        bitbake angelslanding-image -k
    elif [ "$BOARD" == "fbep" ]; then
        bitbake emeraldpools-image -k
    elif [ "$BOARD" == "fbsp" ]; then
        bitbake sonorapass-image -k
    else
        bitbake ${BOARD}-image -k
    fi
    cd ..
    unset TEMPLATECONF
    unset override override_plat override_distro
    unset platform
    unset DISTRO_OVERRIDES
done
