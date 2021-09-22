# Set common sstate directory.
SSTATE_DIR ?= "${COREBASE}/../../../.sstate"

# Use pre-packaged sstate mirrors.
SSTATE_MIRRORS ?= "\
  file://.* \
  file:///data/users/openbmc/openbmc.sstatecache/LAST/.sstate/PATH \
  file://.* \
  file:///data/sandcastle/temp/openbmc_sstate/.sstate/PATH "
