FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://rsyslog.conf \
            file://rotate_logfile \
            file://rotate_cri_sel \
            file://rsyslog.logrotate \
"

do_install_append() {
  dst="${D}/usr/local/fbpackages/rotate"
  install -d $dst
  install -m 755 ${WORKDIR}/rotate_logfile ${dst}/logfile
  install -m 755 ${WORKDIR}/rotate_cri_sel ${dst}/cri_sel
  install -m 644 ${WORKDIR}/rsyslog.conf ${D}${sysconfdir}/rsyslog.conf
  install -m 644 ${WORKDIR}/rsyslog.logrotate ${D}${sysconfdir}/logrotate.rsyslog

  dir=$(pwd)
  while [ -n "$dir" -a "$dir" != "/" -a ! -d "$dir/meta-openbmc/.git" ]; do
      dir=$(dirname $dir)
  done

  if [ -d "$dir/meta-openbmc/.git" ]; then
      srcdir="$dir/meta-openbmc"
      srcdir_git="${srcdir}/.git"
      git_version=$(git --git-dir=${srcdir_git} --work-tree=${srcdir} describe --tags --dirty --always 2> /dev/null)
  else
      git_version=""
  fi

  plat_name="${MACHINE}"
  version="${MACHINE}"
  if [[ $git_version == ${MACHINE}-v* ]]; then
    version=${git_version}
  else
    sha=$(git --git-dir=${srcdir_git} --work-tree=${srcdir} rev-parse --short HEAD)
    version="${MACHINE}-${sha}"
  fi

  sed -i "s/__PLATFORM_NAME__/${plat_name}/g" ${D}${sysconfdir}/rsyslog.conf
  sed -i "s/__BMC_VERSION_TAG__/${version}/g" ${D}${sysconfdir}/rsyslog.conf
}

FILES_${PN} += "/usr/local/fbpackages/rotate"
