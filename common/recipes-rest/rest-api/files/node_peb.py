#!/usr/bin/env python

from common_utils import async_exec
from kv import kv_get
from node import node
from rest_pal_legacy import pal_get_platform_name


class pebNode(node):
    def __init__(self, name=None, info=None, actions=None):
        self.name = pal_get_platform_name()
        if info == None:
            self.info = {}
        else:
            self.info = info
        if actions == None:
            self.actions = []
        else:
            self.actions = actions

    async def getInformation(self, param={}):
        name = pal_get_platform_name()
        location = kv_get("tray_location")
        cmd = "cat /sys/class/gpio/gpio108/value"
        _, stdout, _ = await async_exec(cmd, shell=True)
        data = stdout.strip("\n")
        if data == "0":
            status = "In"
        elif data == "1":
            status = "Out"
        else:
            status = "Unknown"

        data = kv_get("system_identify", 1)
        identify_status = data.strip("\n")

        info = {
            "Description": name + " PCIe Expansion Board",
            "Tray Location": location,
            "Tray Status": status,
            "Status of identify LED": identify_status,
        }

        return info

    async def doAction(self, data, param={}):
        if data["action"] == "identify-on":
            cmd = "/usr/bin/fpc-util --identify on"
            _, data, _ = await async_exec(cmd, shell=True)
            if data.startswith("Usage"):
                res = "failure"
            else:
                res = "success"
        elif data["action"] == "identify-off":
            cmd = "/usr/bin/fpc-util --identify off"
            _, data, _ = await async_exec(cmd, shell=True)
            if data.startswith("Usage"):
                res = "failure"
            else:
                res = "success"
        else:
            res = "not support this action"

        result = {"result": res}

        return result


def get_node_peb():
    actions = ["identify-on", "identify-off"]

    return pebNode(actions=actions)
