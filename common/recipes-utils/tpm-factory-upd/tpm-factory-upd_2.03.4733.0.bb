DESCRIPTION = "Infineon TPM firmware updater"
SECTION = "base"
PR = "r1"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://Source/TPMFactoryUpd/makefile;beginline=2;endline=11;md5=df29a900095e4f745b1fa6f7aa063c42"

S = "${UNPACKDIR}"
LOCAL_URI = "file://Source/TPMFactoryUpd \
             file://Source/Common \
             file://LICENSE \
             file://00-modify_makefile.patch \
            "

DEPENDS += "openssl"

# The makefile defines -D_FORTIFY_SOURCE=1, but Yocto adds
# -D_FORTIFY_SOURCE=2 via SECURITY_CFLAGS. Override to avoid
# redefinition warning. The makefile appends its own CFLAGS after ours,
# so the effective level stays at the vendor's 1. That is deliberate:
# the tool is built the way Infineon validated it.
CFLAGS:append = " -U_FORTIFY_SOURCE"

INSANE_SKIP:${PN}:append = "already-stripped"

binfiles = "TPMFactoryUpd"

do_compile() {
  oe_runmake -C ${UNPACKDIR}/Source/TPMFactoryUpd
}

do_install() {
  bin="${D}/usr/local/bin"
  install -d $bin
  install -m 755 ${UNPACKDIR}/Source/TPMFactoryUpd/${binfiles} ${bin}/${binfiles}
}

FILES:${PN} = "${prefix}/local/bin"
