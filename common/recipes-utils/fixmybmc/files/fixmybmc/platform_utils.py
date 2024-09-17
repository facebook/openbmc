import os


def running_systemd():
    return "systemd" in os.readlink("/proc/1/exe")
