#!/usr/bin/env python
from __future__ import print_function
import os
import json
import time
import socket
import logging
try:
    import subprocess32 as subprocess
except Exception:
    import subprocess
from logging import info, warning
from collections import namedtuple

logging.basicConfig(
    level=logging.DEBUG
)

Condition = namedtuple('Condition', ['condition', 'port'])
State = namedtuple('State', ['conditions', 'bmctime'])

DEST_IP = "fe80::2%usb0"
DEST_PORT = 30321

gpios = {
    'RMON1_PF': Condition("power_loss", 1),
    'RMON2_PF': Condition("power_loss", 2),
    'RMON3_PF': Condition("power_loss", 3),
    'RMON1_RF': Condition("redundancy_failure", 1),
    'RMON2_RF': Condition("redundancy_failure", 2),
    'RMON3_RF': Condition("redundancy_failure", 3),
}

INTERVAL = 1
alertsock = None

# Wedge40 doesn't have gpionames
hardmap = {
    'RMON1_PF': 120,
    'RMON2_PF': 122,
    'RMON3_PF': 124,
    'RMON1_RF': 121,
    'RMON2_RF': 123,
    'RMON3_RF': 125,
}


def gpio_path(name):
    sympath = '/var/volatile/tmp/gpionames/{}'.format(name)
    if os.path.exists(sympath):
        return sympath
    else:
        num = hardmap.get(name)
        if num:
            return '/sys/class/gpio/gpio{}'.format(num)
    return None


class Monitor(object):
    def __init__(self):
        self.active_conditions = {}
        self.gpio_fds = {}
        self.open_gpio_fds()
        self.open_socket()

    def open_socket(self):
        self.alertsock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)

    def send_update(self, pkt):
        info('Sending alert: {}'.format(pkt))
        self.alertsock.sendto(pkt, (DEST_IP, DEST_PORT))

    def open_gpio_fds(self):
        info('Opening GPIOs')
        for name, _ in gpios.items():
            valpath = os.path.join(gpio_path(name), 'value')
            fd = os.open(valpath, os.O_RDONLY)
            self.gpio_fds[name] = fd

    def conditions(self):
        now = time.time()
        for condition, begin in self.active_conditions.items():
            c = {'duration': int(now - begin)}
            c.update(condition._asdict())
            yield c

    def check_gpios(self):
        for name, condition in gpios.items():
            fd = self.gpio_fds[name]
            os.lseek(fd, 0, os.SEEK_SET)
            value = int(os.read(fd, 2))
            if value == 0:
                if condition not in self.active_conditions:
                    self.active_conditions[condition] = time.time()
            if value == 1:
                if condition in self.active_conditions:
                    del self.active_conditions[condition]
        state = State(list(self.conditions()), int(time.time()))
        state_packet = json.dumps(state._asdict())
        if len(state.conditions) > 0:
            pingret = subprocess.call(['ping6', '-c', '1', DEST_IP])
            if pingret != 0:
                warning('Could not ping RSW over USB')
            self.send_update(state_packet)

    def run(self):
        info('Monitoring GPIOs')
        while True:
            self.check_gpios()
            time.sleep(INTERVAL)


def main():
    m = Monitor()
    m.run()


if __name__ == "__main__":
    main()
