OVERRIDES:append = ":pn-obmc-phosphor-image"
require recipes-phosphor/images/obmc-phosphor-image.bb

# TODO (T100404841): These come from obmc-phosphor-image.bbappend upstream but
# they don't don't get applied when doing the `require`.  Add a mechanism
# upstream to create a `.inc` file instead.

IMAGE_FEATURES:remove:fb-nohost = "obmc-console"
OBMC_IMAGE_EXTRA_INSTALL:append = "\
    ${@bb.utils.contains('DISTRO_FEATURES', 'tpm', \
        bb.utils.contains('MACHINE_FEATURES', 'tpm2', 'tpm2-tools', '', d), \
        '', d)} \
    "
