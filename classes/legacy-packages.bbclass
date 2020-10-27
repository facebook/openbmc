# Many 'legacy' packages use to install into `/usr/local/fbpackages/<pkgdir>`
# and then symlink their binaries into `/usr/local/bin` but the default meson
# behavior is to install into /usr/bin.  This bbclass will find all of the
# executables installed by a package and symlink them back into the 'legacy'
# locations.  This allows scripts to continue working until they are updated
# to reflect the new locatiosn.
#
#
# To use this:
#   - set 'pkgdir' to the legacy /usr/local/fbpackages/<pkgdir> location.
#   - inherit legacy-packages
#
# If 'pkgdir' is not set then only the /usr/local/bin symlinks will be made.

do_install_append() {
    if [ "x${pkgdir}" != "x" ]; then
        pkgpath=${D}/usr/local/fbpackages/${pkgdir}
    fi

    install -d ${D}/usr/local/bin

    for exe in ${D}/${bindir}/* ; do
        exename=$(basename ${exe})

        if [ "x${pkgdir}" != "x" ]; then
            install -d ${pkgpath}
            ln -snf ${bindir}/${exename} ${pkgpath}/${exename}
        fi
        ln -snf ${bindir}/${exename} ${D}/usr/local/bin/${exename}
    done
}

FILES_${PN}_append = " /usr/local/bin /usr/local/fbpackages"
