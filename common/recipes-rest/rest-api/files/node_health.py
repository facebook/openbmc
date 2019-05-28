#!/usr/bin/env python

from subprocess import *

from kv import kv_get
from node import node
from pal import *


class healthNode(node):
    def __init__(self, name=None, info=None, actions=None):
        self.name = name

        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    def getInformation(self, param={}):
        # Get Platform Name
        name = pal_get_platform_name()

        result = "NA"
        # Enclosure health LED status (GOOD/BAD)
        if name == "FBTTN":
            dpb_hlth = (
                Popen(
                    "cat /mnt/data/kv_store/dpb_sensor_health", shell=True, stdout=PIPE
                )
                .stdout.read()
                .decode()
            )
            iom_hlth = (
                Popen(
                    "cat /mnt/data/kv_store/iom_sensor_health", shell=True, stdout=PIPE
                )
                .stdout.read()
                .decode()
            )
            nic_hlth = (
                Popen(
                    "cat /mnt/data/kv_store/nic_sensor_health", shell=True, stdout=PIPE
                )
                .stdout.read()
                .decode()
            )
            scc_hlth = (
                Popen(
                    "cat /mnt/data/kv_store/scc_sensor_health", shell=True, stdout=PIPE
                )
                .stdout.read()
                .decode()
            )
            slot1_hlth = (
                Popen(
                    "cat /mnt/data/kv_store/slot1_sensor_health",
                    shell=True,
                    stdout=PIPE,
                )
                .stdout.read()
                .decode()
            )

            if (
                (dpb_hlth == "1")
                & (iom_hlth == "1")
                & (nic_hlth == "1")
                & (scc_hlth == "1")
                & (slot1_hlth == "1")
            ):
                result = "Good"
            else:
                result = "Bad"
        elif name == "Lightning":
            peb_hlth = kv_get("peb_sensor_health")
            pdpb_hlth = kv_get("pdpb_sensor_health")
            fcb_hlth = kv_get("fcb_sensor_health")
            bmc_hlth = kv_get("bmc_health")

            if (
                (peb_hlth == "1")
                and (pdpb_hlth == "1")
                and (fcb_hlth == "1")
                and (bmc_hlth == "1")
            ):
                result = "Good"
            else:
                result = "Bad"

        info = {"Status of enclosure health LED": result}

        return info


def get_node_health():
    return healthNode()
