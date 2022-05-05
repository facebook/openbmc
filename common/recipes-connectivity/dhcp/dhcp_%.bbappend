FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
        file://dhclient.conf \
        "

# Systems with ncsi need to wait until the MAC is assigned by ncsid to avoid
# getting an IPv6-LL with a random address.
SRC_URI:append:mf-ncsi = " file://2000-fix-dhclient-get-wrong-mac.patch"
