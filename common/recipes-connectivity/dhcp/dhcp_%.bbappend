FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://dhclient.conf \
            file://fix-the-error-on-chmod-chown-when-running-dhclient-script.patch \
           "
