import struct
import socket
import os, os.path

def configure_rackmond(reglist):
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
    if os.path.exists(srvpath):
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect(srvpath)
        client.send(config_packet)

