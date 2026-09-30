# Force POSIX pthread mutexes for Berkeley DB.
#
# On gcc >= 14 build hosts (e.g. CentOS Stream 10 Sandcastle), db's autoconf
# mutex probe fails to compile (implicit-function-declaration is now a hard
# error), so configure falls back to UNIX/fcntl and aborts with:
#   "Support for FCNTL mutexes was removed in BDB 4.8."
# Passing --with-mutex bypasses the broken probe.
#
# NOTE: rocko's db recipe exposes a ${MUTEX} variable folded into EXTRA_OECONF,
# but the dunfell db_5.3.28 recipe dropped it, so setting MUTEX here is a no-op.
# Append the option to EXTRA_OECONF directly instead.
EXTRA_OECONF:append = " --with-mutex=POSIX/pthreads/library"
