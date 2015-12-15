require eglibc_${PV}.bb
require eglibc-initial.inc

DEPENDS += "kconfig-frontends-native"

# main eglibc recipes muck with TARGET_CPPFLAGS to point into
# final target sysroot but we
# are not there when building eglibc-initial
# so reset it here

TARGET_CPPFLAGS = ""
