FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
  file://0001-Cherry-pick-fix-for-https-bugs.ntp.org-show_bug.cgi-.patch \
  file://0002-ntpd-use-ASN.1-string-accessors-for-OpenSSL-4.patch \
"
