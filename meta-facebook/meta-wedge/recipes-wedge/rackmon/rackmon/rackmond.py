import struct
import socket
import os, os.path
import subprocess
import time

def configure_rackmond(reglist, verify_configure=False):
    COMMAND_TYPE_SET_CONFIG = 2
    config_command = struct.pack("@HxxH",
            COMMAND_TYPE_SET_CONFIG,
            len(reglist))
    for r in reglist:
        keep = 1
        if "keep" in r:
            keep = r["keep"]
        flags = 0
        if "flags" in r:
            flags = r["flags"]
        monitor_interval = struct.pack("@HHHH", r["begin"], r["length"], keep, flags)
        config_command += monitor_interval

    config_packet = struct.pack("H", len(config_command)) + config_command
    srvpath = "/var/run/rackmond.sock"

    # Internal function for attempting to configure rackmond once
    def configure_once(srvpath, config_packet):
        try:
            if os.path.exists(srvpath):
                client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                client.connect(srvpath)
                client.send(config_packet)
        except Exception as e:
            print("Exception while configuring rackmond: " + str(e))

    # Internal function to check if rackmond is configured
    def check_if_rackmond_configured():
        configured = False;
        cmd = [ "/usr/local/bin/rackmonstatus" ]
        try:
            # If it's configured, it will display a message
            # containing "Monitored PSUs'.
            # Otherwise, the command will either fail to
            # execute, or some message with "Unconfigured"
            # will be printed out.
            o =subprocess.check_output(cmd)
            if 'Monitored PSUs' in o.decode():
                configured = True
        except Exceptions as e:
            configured = False
        return configured

    configure_once(srvpath, config_packet)
    # If verify_configure options is given, we will make sure rackmond
    # is configured. Until it's confirmed, this function will not terminate
    SLEEP_BETWEEN_RETRIES = 5
    MAX_RETRY = 60
    if verify_configure:
        # Do this check up to 60 times. (60 x 5 sec = Max 5 minutes wait)
        retry = 0
        configured = False
        while (not configured) and (retry < MAX_RETRY):
            retry = retry + 1
            time.sleep(SLEEP_BETWEEN_RETRIES)
            configured = check_if_rackmond_configured()
            if not configured:
                # Try configuring rackmond once
                # check_if_rackmond_configured() will check if the attempt
                # was successful in the next loop
                configure_once(srvpath, config_packet)
