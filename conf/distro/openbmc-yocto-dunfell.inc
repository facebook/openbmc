# Prior to Honister, the licenses like GPL-2.0-only didn't exist and only
# the non-timed versions did, but these non-timed versions are now deprecated
# in Kirkstone.  We need our recipes to have the timed versions so that they
# are compatible with Kirkstone, but we need to provide mappings for old:
# distros in order to avoid missing license warnings.
SPDXLICENSEMAP[GPL-2.0-only] = "GPL-2.0"
SPDXLICENSEMAP[GPL-2.0-or-later] = "GPL-2.0"
SPDXLICENSEMAP[LGPL-2.1-only] = "LGPL-2.1"
SPDXLICENSEMAP[LGPL-2.1-or-later] = "LGPL-2.1"

require conf/distro/openbmc-pin-dunfell.inc
