inherit rootfs-postcommands testimage

python zephyrtest_virtclass_handler () {
    variant = e.data.getVar("BBEXTENDVARIANT", True)

    # ipk doesn't like underscores in pacakges names. So just use dashes
    # for PN and the image name.
    variant_dashes = variant.replace('_', '-')

    pn = variant_dashes
    pn_underscores = e.data.getVar("PN") + "-" + variant

    e.data.setVar("PN", pn)
    e.data.setVar("ZEPHYR_IMAGENAME", pn + ".elf")

    # Most tests for Zephyr 1.6 are in the "legacy" folder
    e.data.setVar("ZEPHYR_SRC_DIR", "${ZEPHYR_BASE}/tests/kernel/" + variant)
    e.data.setVar("ZEPHYR_MAKE_OUTPUT", "zephyr.elf")

    # Allow to build using both foo-some_test form as well as foo-some-test
    e.data.setVar("PROVIDES", e.data.getVar("PROVIDES") + pn_underscores)
}

addhandler zephyrtest_virtclass_handler
zephyrtest_virtclass_handler[eventmask] = "bb.event.RecipePreFinalise"

IMAGE_LINK_NAME = "${PN}-image-${MACHINE}"

# Generate test data json file
python do_testdata_write() {
    bb.build.exec_func("write_image_test_data", d)

    # While at it, create dummy manifest files so testimage does not
    # complain...

    fname = os.path.join(d.getVar('DEPLOY_DIR_IMAGE'), (d.getVar('IMAGE_LINK_NAME') + ".manifest"))
    open(fname, 'w').close()
}

CLEANFUNCS += "testdata_clean"

python testdata_clean() {
    import glob

    files = glob.glob(d.getVar('DEPLOY_DIR_IMAGE')+'/'+ d.getVar('PN') + '*.testdata.json')
    for f in files:
        os.remove(f)
    fname = os.path.join(d.getVar('DEPLOY_DIR_IMAGE'), (d.getVar('IMAGE_LINK_NAME') + ".manifest"))
    if os.path.exists(fname):
        os.remove(fname)
}

addtask do_testdata_write before do_testimage after do_deploy
