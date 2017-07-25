DESCRIPTION = "aiohttp framework"
SECTION = "base"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=60dd5a575c9bd4339411bdef4a702d46"

SRC_URI = "https://pypi.python.org/packages/cf/4a/bec3705f07294d9a4057fe5abd93ca89f52149931301674e1e6a9dd66366/aiohttp-2.1.0.tar.gz"
SRC_URI[md5sum] = "52c94bf1735485d9e02fd097ff7d7db9"
SRC_URI[sha256sum] = "3e80d944e9295b1360e422d89746b99e23a99118420f826f990a632d284e21df"

S = "${WORKDIR}/${PN}-${PV}"

dst="/usr/lib/python3.5/site-packages/aiohttp.egg-info"
dst1="/usr/lib/python3.5/site-packages/aiohttp"

do_install() {
  mkdir -p ${D}/${dst}
  mkdir -p ${D}/${dst1}
  install aiohttp.egg-info/dependency_links.txt ${D}/${dst}
  install aiohttp.egg-info/PKG-INFO ${D}/${dst}
  install aiohttp.egg-info/SOURCES.txt ${D}/${dst}
  install aiohttp.egg-info/requires.txt ${D}/${dst}
  install aiohttp.egg-info/top_level.txt ${D}/${dst}
  install aiohttp/web_protocol.py ${D}/${dst1}
  install aiohttp/web.py ${D}/${dst1}
  install aiohttp/abc.py ${D}/${dst1}
  install aiohttp/backport_cookies.py ${D}/${dst1}
  install aiohttp/client_exceptions.py ${D}/${dst1}
  install aiohttp/client_proto.py ${D}/${dst1}
  install aiohttp/client.py ${D}/${dst1}
  install aiohttp/client_reqrep.py ${D}/${dst1}
  install aiohttp/client_ws.py ${D}/${dst1}
  install aiohttp/connector.py ${D}/${dst1}
  install aiohttp/cookiejar.py ${D}/${dst1}
  install aiohttp/_cparser.pxd ${D}/${dst1}
  install aiohttp/formdata.py ${D}/${dst1}
  install aiohttp/hdrs.py ${D}/${dst1}
  install aiohttp/helpers.py ${D}/${dst1}
  install aiohttp/http_exceptions.py ${D}/${dst1}
  install aiohttp/http_parser.py ${D}/${dst1}
  install aiohttp/_http_parser.c ${D}/${dst1}
  install aiohttp/_http_parser.pyx ${D}/${dst1}
  install aiohttp/http.py ${D}/${dst1}
  install aiohttp/http_websocket.py ${D}/${dst1}
  install aiohttp/http_writer.py ${D}/${dst1}
  install aiohttp/__init__.py ${D}/${dst1}
  install aiohttp/log.py ${D}/${dst1}
  install aiohttp/multipart.py ${D}/${dst1}
  install aiohttp/payload.py ${D}/${dst1}
  install aiohttp/payload_streamer.py ${D}/${dst1}
  install aiohttp/pytest_plugin.py ${D}/${dst1}
  install aiohttp/resolver.py ${D}/${dst1}
  install aiohttp/signals.py ${D}/${dst1}
  install aiohttp/streams.py ${D}/${dst1}
  install aiohttp/test_utils.py ${D}/${dst1}
  install aiohttp/web_exceptions.py ${D}/${dst1}
  install aiohttp/web_fileresponse.py ${D}/${dst1}
  install aiohttp/web_middlewares.py ${D}/${dst1}
  install aiohttp/web_request.py ${D}/${dst1}
  install aiohttp/web_response.py ${D}/${dst1}
  install aiohttp/web_server.py ${D}/${dst1}
  install aiohttp/_websocket.c ${D}/${dst1}
  install aiohttp/_websocket.pyx ${D}/${dst1}
  install aiohttp/web_urldispatcher.py ${D}/${dst1}
  install aiohttp/web_ws.py ${D}/${dst1}
  install aiohttp/worker.py ${D}/${dst1}
}

do_configure() {
}

do_compile() {
}

FILES_${PN} = "${dst} ${dst1}"
