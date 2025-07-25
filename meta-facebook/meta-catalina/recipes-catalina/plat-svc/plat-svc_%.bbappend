FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# We don't want this for clemente
SRC_URI:append:catalina = "file://0001-82029-meta-facebook-catalina-Remove-exit-on-second-F.patch"

