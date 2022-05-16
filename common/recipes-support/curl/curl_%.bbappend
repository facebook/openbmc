PACKAGECONFIG:remove = "gnutls"

def curl_ssl_pkgconfig(d):
    distro = d.getVar('DISTRO_CODENAMRE', True)
    if distro in [ 'rocko', 'dunfell', 'honister' ]:
        return "ssl"
    return "openssl"

PACKAGECONFIG:append = " ${@curl_ssl_pkgconfig(d)}"
