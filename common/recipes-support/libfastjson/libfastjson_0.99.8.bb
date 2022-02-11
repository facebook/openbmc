SUMMARY = "A fork of json-c modified to be smaller, faster, and slightly less compliant"
HOMEPAGE = "https://github.com/rsyslog/libfastjson"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING;md5=a958bb07122368f3e1d9b2efe07d231f"

inherit autotools

SRC_URI = "http://download.rsyslog.com/libfastjson/${BP}.tar.gz"

SRC_URI[md5sum] = "fe7b4eae1bf40499f6f92b51d7e5899e"
SRC_URI[sha256sum] = "3544c757668b4a257825b3cbc26f800f59ef3c1ff2a260f40f96b48ab1d59e07"

