SRCREV = "73d6c59af8d1bcedf5de4aa1f5d5b7f765f545f5"
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/cjb/mmc-utils.git;branch=${SRCBRANCH} \
           file://0001-enable-CMD25-as-optional-for-FFU.patch \
          "
