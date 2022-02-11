#!/usr/bin/env python
import os.path
import re
import sys
from ctypes import *
from subprocess import PIPE, Popen


IPMIUTIL = "/usr/local/fbpackages/ipmi-util/ipmi-util"


def execute_IPMI_command(fru, netfn, cmd, req):
    netfn = netfn << 2

    sendreqdata = ""
    if req != "":
        for i in range(0, len(req), 1):
            sendreqdata += str(req[i]) + " "

    input = (
        IPMIUTIL
        + " "
        + str(fru)
        + " "
        + str(netfn)
        + " "
        + str(cmd)
        + " "
        + sendreqdata
    )
    data = ""

    output = Popen(input, shell=True, stdout=PIPE)
    (data, err) = output.communicate()
    data = data.decode()
    result = data.strip().split(" ")

    if output.returncode != 0:
        print("ipmi-util execute fail..., please check ipmid.")
        exit(-1)

    if int(result[2], 16) != 0:
        print("IPMI Failed, CC Code:%s" % result[2])
        exit(0)

    return result[3:]


def status_decode(input):
    if input == 1:
        return "Enabled"
    else:
        return "Disabled"


def status_N_decode(input):
    if input == 1:
        return "Disabled"
    else:
        return "Enabled"


def trans2opcode(input):
    if input == "enable":
        return 1
    else:
        return 0
