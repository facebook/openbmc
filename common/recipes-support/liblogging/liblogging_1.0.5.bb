SUMMARY = "An easy to use logging library."
HOMEPAGE = "http://www.liblogging.org/"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=63fe03535d83726f5655072502bef1bc"

inherit pkgconfig autotools

SRC_URI = "http://download.rsyslog.com/liblogging/${BP}.tar.gz"

SRC_URI[md5sum] = "44b8ce2daa1bfb84c9feaf42f9925fd7"
SRC_URI[sha256sum] = "310dc1691279b7a669d383581fe4b0babdc7bf75c9b54a24e51e60428624890b"

EXTRA_OECONF = "--disable-journal --disable-man-pages"
