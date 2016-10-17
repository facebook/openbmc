# Prevent bitbake from downloading anything over the network.
# This is done via custom fb-only setting in fido and by using
# the builtin BB_ALLOWED_NETWORKS setting in krogoth and above.
python () {
    if d.getVar('DISTRO_CODENAME', True) == 'fido':
        d.setVar('BB_NO_NETWORK', 'fb-only')
    else:
        d.setVar('BB_ALLOWED_NETWORKS', '*.facebook.com')
}
