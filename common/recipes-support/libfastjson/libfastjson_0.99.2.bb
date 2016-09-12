SUMMARY = "A fork of json-c modified to be smaller, faster, and slightly less compliant"
HOMEPAGE = "https://github.com/rsyslog/libfastjson"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=a958bb07122368f3e1d9b2efe07d231f"

inherit autotools

SRC_URI = "http://download.rsyslog.com/libfastjson/${BP}.tar.gz"

SRC_URI[md5sum] = "5e95590422d26b22eb8e9efb073b9b83"
SRC_URI[sha256sum] = "6ff053d455243a81014f37b4d81c746d9b8d40256a56326c3a7921c8bf458dfd"

