# This file can be removed once rocko is eliminated.
#
# Starting in warrior, ntpq is split out from ntp-utils into its own package.
# By picking up ntpq instead of the full ntp-utils package we can eliminate
# the dependency on PERL.
#
# Backport split of ntpq from warrior to rocko.

PACKAGES_prepend = "ntpq "
FILES_ntpq = "${sbindir}/ntpq"
