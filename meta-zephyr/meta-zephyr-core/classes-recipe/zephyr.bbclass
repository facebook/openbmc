inherit terminal
inherit python3native

PYTHONPATH = "${STAGING_DIR_HOST}${libdir}/${PYTHON_DIR}/site-packages"
DEPENDS += "python3-pyelftools-native python3-pyyaml-native python3-pykwalify-native ninja-native"

OE_TERMINAL_EXPORTS += "HOST_EXTRACFLAGS HOSTLDFLAGS TERMINFO CROSS_CURSES_LIB CROSS_CURSES_INC"
HOST_EXTRACFLAGS = "${BUILD_CFLAGS} ${BUILD_LDFLAGS}"
HOSTLDFLAGS = "${BUILD_LDFLAGS}"
CROSS_CURSES_LIB = "-lncurses -ltinfo"
CROSS_CURSES_INC = '-DCURSES_LOC="<curses.h>"'
TERMINFO = "${STAGING_DATADIR_NATIVE}/terminfo"

KCONFIG_CONFIG_COMMAND ??= "menuconfig"
KCONFIG_CONFIG_ROOTDIR ??= "${B}"
ZEPHYR_BOARD ?= "${MACHINE}"

# qemuboot writes into IMGDEPLOYDIR, force to write to DEPLOY_DIR_IMAGE
IMGDEPLOYDIR = "${DEPLOY_DIR_IMAGE}"

python () {
    # Translate MACHINE into Zephyr BOARD
    # Zephyr BOARD is basically our MACHINE, except we must use "-" instead of "_"
    board = d.getVar('ZEPHYR_BOARD', True)
    board = board.replace('-', '_')
    d.setVar('BOARD',board)
}

EXTRA_OECMAKE:append = " -DZEPHYR_MODULES=${ZEPHYR_MODULES}"

python do_menuconfig() {
    import shutil

    config = d.getVar('KCONFIG_CONFIG_ROOTDIR', True) + '/zephyr/' + '.config'
    configorig = d.getVar('KCONFIG_CONFIG_ROOTDIR', True) + '/zephyr/' + '.config.orig'

    try:
        mtime = os.path.getmtime(config)
        shutil.copy(config, configorig)
    except OSError:
        mtime = 0

    oe_terminal("sh -c \"ninja %s; if [ \\$? -ne 0 ]; then echo 'Command failed.'; \
                printf 'Press any key to continue... '; \
                read r; fi\"" % d.getVar('KCONFIG_CONFIG_COMMAND'), \
                d.getVar('PN') + ' Configuration', d)

    try:
        newmtime = os.path.getmtime(config)
    except OSError:
        newmtime = 0

    if newmtime > mtime:
        bb.warn("Configuration changed, recompile will be forced")
        bb.build.write_taint('do_compile', d)
}
do_menuconfig[depends] += "ncurses-native:do_populate_sysroot"
do_menuconfig[nostamp] = "1"
do_menuconfig[dirs] = "${KCONFIG_CONFIG_ROOTDIR}"
addtask menuconfig after do_configure

python do_devshell:prepend () {
    # Most likely we need to manually edit prj.conf...
    os.chdir(d.getVar('ZEPHYR_SRC_DIR', True))
}

