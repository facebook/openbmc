# Set common sstate directory.
def fb_sstate_location(d):
    is_openbmc_root = lambda cur: \
        os.path.isdir(os.path.join(cur, '.git')) and \
        os.path.isfile(os.path.join(cur, 'openbmc-init-build-env'))

    # Find the openbmc root directory.
    openbmc_root = d.getVar('COREBASE', True)
    while openbmc_root and openbmc_root != '/' and \
            not is_openbmc_root(openbmc_root):
        openbmc_root = os.path.dirname(openbmc_root)

    return os.path.join(openbmc_root, '.sstate')

SSTATE_DIR ?= "${@fb_sstate_location(d)}"

# Use pre-packaged sstate mirrors.
SSTATE_MIRRORS ?= "\
  file://.* \
  file:///data/users/openbmc/openbmc.sstatecache/LAST/.sstate/PATH \
  file://.* \
  file:///data/sandcastle/temp/openbmc_sstate/.sstate/PATH \
  file://.* \
  file:///data/sandcastle/temp/skycastle/openbmc_sstate/.sstate/PATH "
