require recipes-kernel/zephyr-kernel/zephyr-kernel-src.inc
require recipes-kernel/zephyr-kernel/zephyr-kernel-common.inc
inherit deploy image-artifact-names

OECMAKE_SOURCEPATH = "${ZEPHYR_SRC_DIR}"

ZEPHYR_IMAGE_LINK_NAME ?= "${PN}-${MACHINE}"
ZEPHYR_IMAGE_BASE_NAME ?= "${PN}-${MACHINE}${IMAGE_VERSION_SUFFIX}"

do_install() {
    install -d ${D}/firmware

    for output in ${ZEPHYR_MAKE_OUTPUT}; do
        if [ -f ${B}/zephyr/${output} ];
        then
          output_img=$(echo ${output} | sed -e "s/zephyr/${ZEPHYR_IMAGE_BASE_NAME}/g")
          install -D ${B}/zephyr/${output} ${D}/firmware/${output_img}
        fi
    done
}
FILES:${PN} = "/firmware"
INSANE_SKIP += "ldflags buildpaths"
SYSROOT_DIRS += "/firmware"

do_deploy() {
    for output in ${ZEPHYR_MAKE_OUTPUT}; do
        if [ -f ${B}/zephyr/${output} ];
        then
          output_img=$(echo ${output} | sed -e "s/zephyr/${ZEPHYR_IMAGE_BASE_NAME}/g")
          output_img_link=$(echo ${output} | sed -e "s/zephyr/${ZEPHYR_IMAGE_LINK_NAME}/g")
          cp ${B}/zephyr/${output} ${DEPLOYDIR}/${output_img}
          ln -sf ${output_img} ${DEPLOYDIR}/${output_img_link}
        fi
    done
}
addtask deploy after do_install
