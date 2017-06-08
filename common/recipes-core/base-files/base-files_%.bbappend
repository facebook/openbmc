FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://console_idle_logout.sh"

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

    print_version="${MACHINE}"
    if [[ $version == ${MACHINE}-v* ]]; then
      print_version=${version}
    else
      sha=$(git --git-dir=${srcdir_git} --work-tree=${srcdir} rev-parse --short HEAD)
      print_version="${MACHINE}-${sha}"
    fi
    echo "OpenBMC Release ${print_version}" > ${D}${sysconfdir}/issue
    echo >> ${D}${sysconfdir}/issue
    echo "OpenBMC Release ${print_version} %h" > ${D}${sysconfdir}/issue.net
    echo >> ${D}${sysconfdir}/issue.net
}

# Enable the idle timeout for auto logout on console port.
# Change the value to 0 will disable this feature.
CONSOLE_IDLE_TIMEOUT ?= "300"

do_install_append() {
    if [[ ${CONSOLE_IDLE_TIMEOUT} -ne 0 ]]; then
        install -d ${D}/${sysconfdir}/profile.d
        install -m 644 console_idle_logout.sh ${D}/${sysconfdir}/profile.d/console_idle_logout.sh
        sed -i 's/__TMOUT__/${CONSOLE_IDLE_TIMEOUT}/g' ${D}/${sysconfdir}/profile.d/console_idle_logout.sh
    fi
}
