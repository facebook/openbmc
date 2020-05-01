inherit ptest
inherit meson

DEPENDS += "\
    ptest-meson-crosstarget-native \
    chrpath-native \
    "

RDEPENDS_${PN}-ptest += "meson"

do_compile_append() {
    cat <<EOF > ${WORKDIR}/run-ptest
#!/bin/sh
meson test --no-rebuild
EOF

}

do_install_ptest() {
    # Fix up meson private data to run on target.
    ptest-meson-crosstarget --install-path ${PTEST_PATH} --package-path ${B}

    # Create destination directories.
    install -d ${D}${PTEST_PATH}
    install -d ${D}${PTEST_PATH}/meson-private
    install -d ${D}${PTEST_PATH}/meson-logs

    # Install executables with the name test-*
    for file in ${B}/test-*
    do
        if [ -f ${file} ]; then
            install -m 0755 ${file} ${D}${PTEST_PATH}/
            # Meson 0.40 (Rocko) adds incorrect RPATHs, so delete them.
            chrpath -d ${D}${PTEST_PATH}/$(basename ${file})
        fi
    done

    # Install meson data.
    for file in ${B}/meson-private/*.dat
    do
        install -m 0644 ${file} ${D}${PTEST_PATH}/meson-private
    done
}
