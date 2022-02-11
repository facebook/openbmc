# How many tasks bitbake should run in parallel:
BB_NUMBER_THREADS ?= "${@oe.utils.cpu_count()}"
# How many processes make should run in parallel:
PARALLEL_MAKE ?= "-j ${@oe.utils.cpu_count()}"
