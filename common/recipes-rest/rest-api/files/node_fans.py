#!/usr/bin/env python

from node import node
from pal import *


class fansNode(node):
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
        result = []
        skip_flag = False
        cmd = "/usr/local/bin/fan-util --get"
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        sdata = data.split("\n")
        for line in sdata:
            # skip lines with " or startin with FRU
            if line.find("Error") != -1:
                continue

            kv = line.split(":")
            if len(kv) < 2:
                continue

            if (skip_flag) and (kv[0].strip() != "Fan Fail"):
                continue

            # 0: normal, 1: boost, 2: transitional
            if kv[0].strip() == "Fan Mode":
                if kv[1].strip() == "Normal":
                    kv[1] = " 0"
                elif kv[1].strip() == "Boost":
                    kv[1] = " 1"
                elif kv[1].strip() == "Transitional":
                    kv[1] = " 2"

            # 0: normal, 1: 1 or more sensor failed
            elif kv[0].strip() == "Sensor Fail":
                skip_flag = True
                if kv[1].strip() == "None":
                    kv[1] = " 0"
                elif kv[1].strip() == "":
                    kv[1] = " 1"

            # 0: normal, 1: 1 or more fan failed
            elif kv[0].strip() == "Fan Fail":
                skip_flag = False
                if kv[1].strip() == "None":
                    kv[1] = " 0"
                elif kv[1].strip() == "":
                    kv[1] = " 1"

            # 0: normal (no boost), 1: platform-specific boost cause triggered
            elif kv[0].strip() == "Sled Fan Latch Open":
                kv[0] = "Platform Specific"
                if kv[1].strip() == "False":
                    kv[1] = " 0"
                elif kv[1].strip() == "True":
                    kv[1] = " 1"

            result.append(kv[0] + ":" + kv[1])
        return result


def get_node_fans():
    return fansNode()
