# Copyright 2018-present Facebook. All Rights Reserved.

### ZSTD support taken from latest image_types.bbclass ###
# Once we move to a Yocto version that includes this in image_types.bbclass,
# we should remove it.
IMAGE_TYPES += "cpio.zst"
COMPRESSIONTYPES += "zst"

ZSTD_COMPRESSION_LEVEL="-19"

CONVERSION_CMD:zst = "zstd -f -k -T0 -c ${ZSTD_COMPRESSION_LEVEL} ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type} > ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type}.zst"
CONVERSION_DEPENDS_zst = "zstd-native"
### end ZSTD support ###

# Base this image on core-image-minimal
require recipes-core/images/core-image-minimal.bb

require common/images/fb-openbmc-common.inc

IMAGE_FEATURES:append = " \
    ssh-server-openssh \
    tools-debug \
    "

PACKAGE_EXCLUDE += "\
    gnutls \
    perl \
    u-boot \
    "

# ca-certificates:
# Nearly every package that uses TLS/SSL RRECOMMENDS the CA certificate
# package.  Since we should not be initiating SSL traffic from a BMC
# to outside of our own network, there is no value in all of these
# certificates.  Add it to the BAD_RECOMMENDATIONS so it is not installed
# unless there is a strict need for it.
#
# udev-hwdb:
# There is no need for the udev hwdb and it installs 8MB uncompressed of data.
#
# shared-mime-info:
# We're not sending email, so unneeded.
#
# util-linux-renice:
# TODO: FBOSS platforms are suddenly pulling a bunch of util-linux packages,
# but we expect busybox-renice.  Un-RECOMMEND util-linux-renice until we
# can figure out why all these are coming in.
BAD_RECOMMENDATIONS += " \
    ca-certificates \
    udev-hwdb \
    shared-mime-info \
    util-linux-renice \
    "

# Many of the sysv init files we already have install into runlevel 5, so
# the automatic service files created for them by systemd add them to the
# graphical.target.  Set that as our default until we get them all migrated
# to native systemd services.
SYSTEMD_DEFAULT_TARGET = "graphical.target"

# systemd uses systemd-networkd, so make minor tweaks to use it instead of
# 'init-ifupdown'.
IMAGE_INSTALL += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'systemd-networkd', '', d)}"
IMAGE_INSTALL:remove = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'init-ifupdown', '', d)}"
SYSVINIT_SCRIPTS:remove = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'init-ifupdown', '', d)}"

#
# "openssl-bin" (which provides "/usr/bin/openssl") is needed by backport
# and it needs to be explicitly included after yocto rocko.
#
def openssl_bin_recipe(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro != 'rocko':
        return 'openssl-bin'
    return ''

IMAGE_INSTALL += "${@openssl_bin_recipe(d)}"
