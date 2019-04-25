FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

# The patch is for bug discovered in dhclient v4.3.3 while
# rocko packages dhcp v4.3.6. We need to retest if issue is
# still reproducable on this version before creating a new
# patch based on this version.
def dhcp_patches(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro == 'fido' or distro == 'krogoth':
        return "file://dhclinet_ipv6_addr_missing.patch \
               "
    elif distro == 'rocko':
        return "file://fix-the-error-on-chmod-chown-when-running-dhclient-script.patch \
               "
    else:
        return ""

SRC_URI += "file://dhclient.conf \
           "
SRC_URI += '${@dhcp_patches(d)}'
