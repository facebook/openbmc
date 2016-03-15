BASEFILESISSUEINSTALL = "do_install_bmc_issue"

DISTRO_HOSTNAME = "bmc"

do_install_bmc_issue () {
    if [ "${DISTRO_HOSTNAME}" != "" ]; then
        echo ${DISTRO_HOSTNAME} > ${D}${sysconfdir}/hostname
    else
        echo ${MACHINE} > ${D}${sysconfdir}/hostname
    fi

    # found out the source dir
    dir=$(pwd)
    while [ -n "$dir" -a "$dir" != "/" -a ! -d "$dir/meta-openbmc/.git" ]; do
        dir=$(dirname $dir)
    done

    if [ -d "$dir/meta-openbmc/.git" ]; then
        srcdir="$dir/meta-openbmc"
        srcdir_git="${srcdir}/.git"
        version=$(git --git-dir=${srcdir_git} --work-tree=${srcdir} describe --tags --dirty --always 2> /dev/null)
    else
        version=""
    fi

    echo "OpenBMC Release ${version}" > ${D}${sysconfdir}/issue
    echo >> ${D}${sysconfdir}/issue
    echo "OpenBMC Release ${version} %h" > ${D}${sysconfdir}/issue.net
    echo >> ${D}${sysconfdir}/issue.net
}
