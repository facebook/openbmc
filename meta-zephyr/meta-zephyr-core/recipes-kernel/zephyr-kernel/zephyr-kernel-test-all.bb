LICENSE = "Apache-2.0"
INHIBIT_DEFAULT_DEPS = "1"

require recipes-kernel/zephyr-kernel/zephyr-kernel-test.inc

addtask testimage
deltask compile
deltask install

do_testimage () {
	:
}

do_testimage[depends] = '${@" ".join(["zephyr-kernel-test-" + x + ":do_testimage" for x in d.getVar("ZEPHYRTESTS", True).split()])}'

do_build[depends] = '${@" ".join(["zephyr-kernel-test-" + x + ":do_build" for x in d.getVar("ZEPHYRTESTS", True).split()])}'

do_clean[depends] = '${@" ".join(["zephyr-kernel-test-" + x + ":do_clean" for x in d.getVar("ZEPHYRTESTS", True).split()])}'
