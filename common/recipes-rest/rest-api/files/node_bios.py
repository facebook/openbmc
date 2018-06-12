#!/usr/bin/env python

from node import node
from pal import *
import os.path

IOM_M2 = 1      # IOM type: M.2
IOM_IOC = 2     # IOM type: IOC
PLATFORM_FILE = "/tmp/system.bin"

def get_iom_type():
    pal_sku_file = open(PLATFORM_FILE, 'r')
    pal_sku = pal_sku_file.read()
    iom_type = int(pal_sku) & 0x3  # The IOM type is the last 2 bits

    if ( iom_type != IOM_M2) and ( iom_type != IOM_IOC):
        print("Rest-API: System type is unknown! Please confirm the system type.")
        return -1
    else:
        return iom_type

'''''''''''''''''''''''''''''''''''
          Main Node
'''''''''''''''''''''''''''''''''''
class biosNode(node):
    def __init__(self, name, info = None, actions = None):
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
        info = {
        "Description": "BIOS Information",
        }

        return info

def get_node_bios(name):
    return biosNode(name)


'''''''''''''''''''''''''''''''''''
      Boot Order Information
'''''''''''''''''''''''''''''''''''
class bios_boot_order_trunk_node(node):
    def __init__(self, name, info = None, actions = None):
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
        info = {
        "Description": "BIOS Boot Order Information",
        }

        return info

'''''''''''''''''''''''''''''
         Boot Mode
'''''''''''''''''''''''''''''
class bios_boot_mode_node(node):
    def __init__(self, name, info = None, actions = None):
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
        cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order get --boot_mode'
        boot_order = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        boot_order = boot_order.split('\n')[0].split(': ')

        info = {
            boot_order[0]: boot_order[1],
            "Note #1: Actions Format:": "{ 'action': 'set', 'mode': {0,1} }",
            "Note #2: Boot Mode No.": "{ 0: 'Legacy', 1: 'UEFI' }",
            }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] == "set" and len(data) == 2:
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order set --boot_mode ' + data["mode"]
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            else:
                res = 'failure'

            result = { "result": res }

        return result

'''''''''''''''''''''''''''''
         Clear CMOS
'''''''''''''''''''''''''''''
class bios_clear_cmos_node(node):
    def __init__(self, name, info = None, actions = None):
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
        cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order get --clear_CMOS'
        clear_cmos = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        clear_cmos = clear_cmos.split('\n')[0].split(': ')

        info = { clear_cmos[0]: clear_cmos[1] }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] == "enable":
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order enable --clear_CMOS'
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            elif data["action"] == "disable":
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order disable --clear_CMOS'
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            else:
                res = 'failure'

            result = { "result": res }

        return result

'''''''''''''''''''''''''''''
    Force Boot BIOS Setup
'''''''''''''''''''''''''''''
class bios_force_boot_setup_node(node):
    def __init__(self, name, info = None, actions = None):
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
        cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order get --force_boot_BIOS_setup'
        force_boot_bios_setup = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        force_boot_bios_setup = force_boot_bios_setup.split('\n')[0].split(': ')

        info = { force_boot_bios_setup[0]: force_boot_bios_setup[1] }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] == "enable":
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order enable --force_boot_BIOS_setup'
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            elif data["action"] == "disable":
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order disable --force_boot_BIOS_setup'
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            else:
                res = 'failure'

            result = { "result": res }

        return result

'''''''''''''''''''''''''''''
         Boot Order
'''''''''''''''''''''''''''''
class bios_boot_order_node(node):
    def __init__(self, name, info = None, actions = None):
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
        cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order get --boot_order'
        boot_order = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        boot_order = boot_order.split('\n')[0].split(': ')

        info = {
            boot_order[0]: boot_order[1],
            "Note #1: Actions Format:": "{'action': 'set', '1st': <1st_no>, '2nd': <2nd_no>, '3rd': <3rd_no>, '4th': <4th_no>, '5th': <5th_no>}",
            "Note #2: Boot Order No.": "{ 0: 'USB Device', 1: 'IPv4 Network', 9: 'IPv6 Network', 2: 'SATA HDD', 3: 'SATA-CDROM', 4: 'Other Removalbe Device', 255: 'Reserved' }",
            }

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] == "set" and len(data) == 6:
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order set --boot_order ' \
                + data["1st"] + " " + data["2nd"] + " " + data["3rd"] + " " + data["4th"] + " " + data["5th"]
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err != "" or data != "":
                    res = 'failure'
                else:
                    res = 'success'
            elif data["action"] == "disable":
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --boot_order disable --boot_order'
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            else:
                res = 'failure'

            result = { "result": res }

        return result

def get_node_bios_boot_order_trunk(name):
    return bios_boot_order_trunk_node(name)

def get_node_bios_boot_mode(name, is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions =  ["set",
                    ]
    return bios_boot_mode_node(name = name, actions = actions)

def get_node_bios_clear_cmos(name, is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions =  ["enable",
                    "disable",
                    ]
    return bios_clear_cmos_node(name = name, actions = actions)

def get_node_bios_force_boot_setup(name, is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions =  ["enable",
                    "disable",
                    ]
    return bios_force_boot_setup_node(name = name, actions = actions)

def get_node_bios_boot_order(name, is_read_only=True):
    if is_read_only:
        actions =  []
    else:
        actions =  ["set",
                    "disable",
                    ]
    return bios_boot_order_node(name = name, actions = actions)


'''''''''''''''''''''''''''''''''''
     BIOS POST Code Information
'''''''''''''''''''''''''''''''''''
class bios_postcode_node(node):
    def __init__(self, name, info = None, actions = None):
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
        cmd = '/usr/local/bin/bios-util ' + self.name + ' --postcode get'
        postcode = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        postcode = postcode.replace("\n", "").strip()

        info = { "POST Code": postcode }

        return info

def get_node_bios_postcode_trunk(name):
    return bios_postcode_node(name)


'''''''''''''''''''''''''''''''''''
       Platform Information
'''''''''''''''''''''''''''''''''''
class bios_plat_info_node(node):
    def __init__(self, name, info = None, actions = None):
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
        cmd = '/usr/local/bin/bios-util ' + self.name + ' --plat_info get'
        data = plat_info = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
        plat_info = data.stdout.read().decode()
        err = data.stderr.read().decode()
        if err.startswith( 'usage' ):
            plat_info = 'Currently the platform does not support plat-info\n'

        plat_info = plat_info.split('\n')
        plat_info_len = len(plat_info)

        info = { "Platform Information": plat_info[0 : plat_info_len - 1] }

        return info

def get_node_bios_plat_info_trunk(name):
    return bios_plat_info_node(name)


'''''''''''''''''''''''''''''''''''
     PCIe Port Configuration
'''''''''''''''''''''''''''''''''''
class bios_pcie_port_config_node(node):
    def __init__(self, name, info = None, actions = None):
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
        cmd = '/usr/local/bin/bios-util ' + self.name + ' --pcie_port_config get'
        data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
        pcie_port_config = data.stdout.read().decode()
        err = data.stderr.read().decode()
        if err.startswith( 'usage' ):
            pcie_port_config = 'Currently the platform does not support pcie-port-config\n'

        pcie_port_config = pcie_port_config.split('\n')
        pcie_port_config_len = len(pcie_port_config)

        iom_type = get_iom_type()
        if iom_type == IOM_M2:
            info = {
                "PCIe Port Configuration": pcie_port_config[0 : pcie_port_config_len - 1],
                "Note: Actions Format:": "{'action': <enable, disable>, 'pcie_dev': <scc_ioc, flash1, flash2, nic>}",
                }
        elif iom_type == IOM_IOC:
            info = {
                "PCIe Port Configuration": pcie_port_config[0 : pcie_port_config_len - 1],
                "Note: Actions Format:": "{'action': <enable, disable>, 'pcie_dev': <scc_ioc, iom_ioc, nic>}",
                }
        else:
            info = []

        return info

    def doAction(self, data, is_read_only=True):
        if is_read_only:
            result = { "result": 'failure' }
        else:
            if data["action"] == "enable" and len(data) == 2:
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --pcie_port_config enable --' + data["pcie_dev"]
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            elif data["action"] == "disable":
                cmd = '/usr/local/bin/bios-util ' + self.name + ' --pcie_port_config disable --' + data["pcie_dev"]
                data = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
                err = data.stderr.read().decode()
                data = data.stdout.read().decode()
                if err.startswith( 'usage' ):
                    res = 'failure'
                else:
                    res = 'success'
            else:
                res = 'failure'

            result = { "result": res }

        return result

def get_node_bios_pcie_port_config_trunk(name, is_read_only=True):
    if is_read_only:
        actions = []
    else:
        actions = ["enable",
                   "disable",
                  ]
    return bios_pcie_port_config_node(name = name, actions = actions)
