PYOCD_CONNECT_TIMEOUT_SECONDS ?= "30"
PYOCD_FLASH_IDS ?= "all"

python do_flash_usb() {
    try:
        from pyocd.core.helpers import ConnectHelper
        from pyocd.flash.file_programmer import FileProgrammer
    except ImportError:
        bb.fatal("Flashing with pyocd needs the relevant python package. Make sure your host provides it or consult your distribution packages for how to install this prerequisite.")

    try:
        timeout = int(d.getVar('PYOCD_CONNECT_TIMEOUT_SECONDS'))
    except ValueError:
        bb.fatal(f"PYOCD_CONNECT_TIMEOUT_SECONDS was set to an invalid value: {d.getVar('PYOCD_CONNECT_TIMEOUT_SECONDS')}.")
    image = f"{d.getVar('DEPLOY_DIR_IMAGE')}/{d.getVar('PN')}.elf"
    ids = d.getVar('PYOCD_FLASH_IDS')

    # Compute the list of IDs to program
    if ids == 'all':
        ids = []
        for probe in ConnectHelper.get_all_connected_probes(blocking=False):
            ids.append(probe.unique_id)
        if not ids:
            bb.fatal("No probe detected. Make sure your target is connected.")
    else:
        ids = ids.split()
        if not ids:
            bb.fatal("No probe requested for programming. Make sure PYOCD_FLASH_IDS is set.")

    # Fetch target type to pass to the ConnectHelper
    target = d.getVar('PYOCD_TARGET')

    # Program each ID
    for id in ids:
        bb.plain(f"Attempting to flash {os.path.basename(image)} to board {d.getVar('BOARD')} [{id}]")

        # Try to connect to a probe with a timeout
        now = 0
        step = 3
        while True:
            if target is not None:
                session = ConnectHelper.session_with_chosen_probe(blocking=False, return_first=True, unique_id=id, target_override=target)
            else:
                bb.warn(f"Target type not provided. Flashing may fail or result in an undefined behavior.")
                session = ConnectHelper.session_with_chosen_probe(blocking=False, return_first=True, unique_id=id)

            if session:
                break
            if now >= timeout:
                bb.fatal(f"Timeout while trying to connect to probe ID: {id}. Make sure the target device is connected and the udev is configured accordingly. See <https://github.com/mbedmicro/pyOCD/tree/master/udev> for help.")
            bb.warn(f"Can't connect to the probe ID: {id}. Retrying in {step} seconds...")
            time.sleep(step)
            now += step

        # Program the selected probe
        with session:
            FileProgrammer(session).program(image)
            session.board.target.reset()
}

addtask do_flash_usb after do_deploy

do_flash_usb[nostamp] = "1"
do_flash_usb[vardepsexclude] = "BB_ORIGENV"
