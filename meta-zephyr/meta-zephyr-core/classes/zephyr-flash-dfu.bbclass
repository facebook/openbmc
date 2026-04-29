
python do_flash_usb() {
    import subprocess

    # Append the original PATH so we can find dfu-util...
    origbbenv = d.getVar("BB_ORIGENV", False)
    path = d.getVar('PATH') + ":" + origbbenv.getVar('PATH')
    os.environ['PATH'] = path

    return_code = subprocess.call("which dfu-util", shell=True)
    if return_code != 0:
        bb.error("ERROR: dfu-util binary not in PATH")
        sys.exit(1)

    board = d.getVar('BOARD')

    if board == 'arduino_101_sss':
        iface = 'sensor_core'
    elif board == 'arduino_101':
        iface = 'x86_app'
    elif board == 'arduino_101_ble':
        iface = 'ble_core'
    else:
        bb.error(" Unsupported board %s" % board)
        sys.exit(2)

    # We need to serialize flashing of separate images (when using multiconfig)
    lock = bb.utils.lockfile(os.path.join(d.getVar('TOPDIR'),"flash-dfu.lock"), False, True)
    image = "%s/%s.elf" % (d.getVar('DEPLOY_DIR_IMAGE'), d.getVar('PN'))
    statement = 'dfu-util -v -a ' + iface + ' -d 8087:0aba -D ' + image.replace('elf','bin')

    bb.note("command: %s" % statement)
    bb.plain("Attempting to flash board: %s" % board)

    # Arduino-101 needs to be reset in order to enter "flash mode".
    # The flash window is about 5 seconds.
    # However, if the device is flashed, it remains in the "flash mode"
    # until it is reset again, so it needs to enter the flash mode via reset
    # only once even if we flash images for multiple cores. So we only prompt
    # for reset once.

    timeout = 5
    promptneeded = True
    while (timeout > 0):
        time.sleep(0.5)
        timeout = timeout - 0.5
        return_code = subprocess.call(statement, shell=True)
        if return_code == 0:
            break
        if promptneeded:
            bb.warn("\n\n   *** Failed to flash %s. *** \n   Press reset, will retry for five seconds...\n" % board)
            promptneeded = False

    if return_code != 0:
        bb.error("Error flashing %s [%d]" % (board,return_code))
    else:
        bb.plain("Success (return code %d)" % return_code)

    bb.utils.unlockfile(lock)
}

addtask do_flash_usb after do_deploy

do_flash_usb[nostamp] = "1"
do_flash_usb[vardepsexclude] = "BB_ORIGENV"
