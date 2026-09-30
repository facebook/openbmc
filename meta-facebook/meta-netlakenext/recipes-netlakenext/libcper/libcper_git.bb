SUMMARY = "Header-only installation of libcper for OpenBMC"

DESCRIPTION = "This recipe installs only the header files from the libcper repository"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=a832eda17114b48ae16cda6a500941c2"

SRCREV = "559780e3224d90b62a18aedea0fc1bccadc3977e"
SRC_URI = "git://github.com/openbmc/libcper.git;protocol=https;branch=main"

S = "${WORKDIR}/git"

do_install() {
    local dest_dir="${D}${includedir}/libcper"
    local src_dir="${S}/include/libcper"

    install -d "${dest_dir}"
    install -m 0644 "${src_dir}/Cper.h" "${dest_dir}/"
    install -m 0644 "${src_dir}/BaseTypes.h" "${dest_dir}/"
    install -m 0644 "${src_dir}/common-utils.h" "${dest_dir}/"
}
