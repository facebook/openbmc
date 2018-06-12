#!/usr/bin/env python

from node import node
from subprocess import *

def get_node_enclosure():
    info = {
        "Description" : "Enclosure-util Information",
    }
    return node(info)

class enclosure_error_Node(node):
    def __init__(self, name = None, info = None, actions = None):
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
        result = {}

        cmd = '/usr/bin/enclosure-util --error'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        data = data.strip()
        sdata = data.split('\n')
        for line in sdata:
            kv = line.split(':')
            result[kv[0].strip()] = kv[1].strip()

        return result

def get_node_enclosure_error():
    return enclosure_error_Node()


class enclosure_flash_health_Node(node):
    def __init__(self, name = None, info = None, actions = None):
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
        result = {}

        cmd = '/usr/bin/enclosure-util --flash-health'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        data = data.strip()
        sdata = data.split('\n')
        for line in sdata:
            kv = line.split(':')
            result[kv[0].strip()] = kv[1].strip()

        return result

def get_node_enclosure_flash_health():
    return enclosure_flash_health_Node()


class enclosure_flash_status_Node(node):
    def __init__(self, name = None, info = None, actions = None):
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
        info = {}

        cmd = '/usr/bin/enclosure-util --flash-status'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        data = data.split('flash-2')
        data_flash_1 = data[0].strip().split('\n')
        data_flash_2 = data[1].strip().split('\n')
        info = {
            "flash-1": data_flash_1[1:],
            "flash-2": data_flash_2[1:],
        }      

        return info

def get_node_enclosure_flash_status():
    return enclosure_flash_status_Node()


class enclosure_hdd_status_Node(node):
    def __init__(self, name = None, info = None, actions = None):
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
        result = {}

        cmd = '/usr/bin/enclosure-util --hdd-status'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        data = data.strip()
        sdata = data.split('\n')
        for line in sdata:
            kv = line.split(':')
            result[kv[0].strip()] = kv[1].strip()

        return result

    def doAction(self, data):
        result = {}

        if (data["action"].isdigit()) and (int(data["action"]) >= 0) and (int(data["action"]) < 36):
            cmd = '/usr/bin/enclosure-util --hdd-status ' + data["action"]
            data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
            sdata = data.strip().split(':')

            result[sdata[0].strip()] = sdata[1].strip()
        else:
            result = { "result": "failure" }

        return result

def get_node_enclosure_hdd_status():
    actions =  ["0~35"]
    return enclosure_hdd_status_Node(actions = actions)
