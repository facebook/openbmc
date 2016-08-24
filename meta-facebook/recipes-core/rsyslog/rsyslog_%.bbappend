# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# This bbappend amends the meta-oe rsyslog recipe

# Options set here may get overridden by PACKAGECONFIG.
#EXTRA_OECONF += ""

# Override PACKAGECONFIG to remove gnutls, imdiag, uuid, snmp, ptest,
# and testbench. regexp+zlib+libgcrypt are required for compile
PACKAGECONFIG ??= " \
    zlib rsyslogd rsyslogrt klog inet regexp libgcrypt imfile \
    ${@base_contains('DISTRO_FEATURES', 'systemd', 'systemd', '', d)} \
"
