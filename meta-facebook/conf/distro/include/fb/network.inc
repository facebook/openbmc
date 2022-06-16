# Distro settings unique to the Facebook network.

# Set up internal source mirrors and block the internet.
INHERIT += "fb-source-mirror fb-only-network"
# If you need to download new packages from the internet you can
# temporarily add 'INHERIT:remove = "fb-only-network"' to your
# `conf/local.conf`.

def force_shallow_tarballs(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "0"
    return "1"

# Save local tarballs for all packages we download.
# This can be used to update our mirror directory above.
BB_GENERATE_MIRROR_TARBALLS = "1"
BB_GENERATE_SHALLOW_TARBALLS ?= "${@force_shallow_tarballs(d)}"
BB_GIT_SHALLOW ?= "${@force_shallow_tarballs(d)}"

# Do not check reachability as it does not work in our environment
CONNECTIVITY_CHECK_URIS = ""
