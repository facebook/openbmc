#!/usr/bin/env python
import os.path
import sys
from subprocess import PIPE, Popen

from bios_ipmi_util import *


PLATFORM_FILE = "/tmp/system.bin"

NODE_NUM = "1"  # FRU:1 Server
IOM_M2 = 1  # IOM type: M.2
IOM_IOC = 2  # IOM type: IOC

PCIe_port_name_T5 = {0: "SCC IOC", 1: "flash1", 2: "flash2", 3: "NIC"}
PCIe_port_name_T7 = {0: "SCC IOC", 1: "IOM IOC", 3: "NIC"}


def get_iom_type(plat_sku_file):
    pal_sku_file = open(plat_sku_file, "r")
    pal_sku = pal_sku_file.read()
    iom_type = int(pal_sku) & 0x3  # The IOM type is the last 2 bits

    if (iom_type != IOM_M2) and (iom_type != IOM_IOC):
        print("System type is unknown! Please confirm the system type.")
        exit(-1)
    else:
        return iom_type


"""
OEM Get PCIe Port Configuration (NetFn:0x30, CMD: 80h)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - IOU2 -8 lanes
     Bit 7   - Vaild bit (0: the command byte is invalid and will be ignored; 1: the command byte is available)
     Bit 6   - Restore bit (0: refer to Bit[1:0] ; 1: all ports from IOU2 will be restored to default Auto, Bit[1:0]are ignored.)
     Bit 5-2 - Reserved
     Bit 1   - Port 1B  0:Enable 1:Disable
     Bit 0   - Port 1A  0:Enable 1:Disable
   Byte 3 - IOU1 -16 lanes
     Bit 7   - Vaild bit (0: the command byte is invalid and will be ignored; 1: the command byte is available)
     Bit 6   - Restore bit (0: refer to Bit[3:0]; 1: all ports from IOU1 will be restored to default Auto, Bit[3:0]are ignored.)
     Bit 5-4 - Reserved
     Bit 3   - Port 3D  0:Enable 1:Disable
     Bit 2   - Port 3C  0:Enable 1:Disable
     Bit 1   - Port 3B  0:Enable 1:Disable
     Bit 0   - Port 3A  0:Enable 1:Disable
"""
"""
OEM Set PCIe Port Configuration (NetFn:0x30, CMD: 0x81h)
Request:
   Byte 1 - IOU2 -8 lanes
     Bit 7   - Vaild bit (0: the command byte is invalid and will be ignored; 1: the command byte is available)
     Bit 6   - Restore bit (0: refer to Bit[1:0] ; 1: all ports from IOU2 will be restored to default Auto, Bit[1:0]are ignored.)
     Bit 5-2 - Reserved
     Bit 1   - Port 1B  0:Enable 1:Disable (If the port is not exist due to the platform bifurcation, please leave its corresponding bit as 0)
     Bit 0   - Port 1A  0:Enable 1:Disable
   Byte 2 - IOU1 -16 lanes
     Bit 7   - Vaild bit (0: the command byte is invalid and will be ignored; 1: the command byte is available)
     Bit 6   - Restore bit (0: refer to Bit[3:0]; 1: all ports from IOU1 will be restored to default Auto, Bit[3:0]are ignored.)
     Bit 5-4 - Reserved
     Bit 3   - Port 3D  0:Enable 1:Disable (If the port is not exist due to the platform bifurcation, please leave its corresponding bit as 0)
     Bit 2   - Port 3C  0:Enable 1:Disable (If the port is not exist due to the platform bifurcation, please leave its corresponding bit as 0)
     Bit 1   - Port 3B  0:Enable 1:Disable (If the port is not exist due to the platform bifurcation, please leave its corresponding bit as 0)
     Bit 0   - Port 3A  0:Enable 1:Disable
Response:
   Byte 1 - Completion Code
"""


def pcie_port_config(fru, argv):
    req_data = ["", ""]
    res_data = ["", ""]
    function = argv[2]

    if function != "get":
        device = argv[3]
    else:
        device = ""

    # Get the original config
    result = execute_IPMI_command(fru, 0x30, 0x80, "")
    res_data = [int(n, 16) for n in result]
    req_data = do_pcie_port_config_action(PLATFORM_FILE, function, device, res_data)

    if function != "get":
        execute_IPMI_command(fru, 0x30, 0x81, req_data)


def do_pcie_port_config_action(plat_sku_file, function, device, res_data):
    req_data = ["", ""]
    pcie_setup_data = [0, 0]

    # Get IOM type
    iom_type = get_iom_type(plat_sku_file)
    if iom_type == IOM_M2:
        PCIe_port_name = PCIe_port_name_T5
    elif iom_type == IOM_IOC:
        PCIe_port_name = PCIe_port_name_T7

    if function == "get":
        # If the valid bit is invalid, it will be ignored
        IOU1_valid_bit = (res_data[1] >> 7) & 0x1
        if IOU1_valid_bit == 1:
            Port3D_bit = (res_data[1] >> 3) & 0x1
            Port3C_bit = (res_data[1] >> 2) & 0x1
            Port3B_bit = (res_data[1] >> 1) & 0x1
            Port3A_bit = (res_data[1] >> 0) & 0x1

            # SCC IOC
            if (Port3A_bit == 1) or (Port3B_bit == 1):
                PCIe_Port3AB_status = 1  # Disable
            else:
                PCIe_Port3AB_status = 0  # Enable
            print(
                "  " + PCIe_port_name[0] + ": " + status_N_decode(PCIe_Port3AB_status)
            )

            if iom_type == IOM_M2:
                # IOM M.2
                print("  " + PCIe_port_name[1] + ": " + status_N_decode(Port3C_bit))
                print("  " + PCIe_port_name[2] + ": " + status_N_decode(Port3D_bit))
            elif iom_type == IOM_IOC:
                # IOM IOC
                if (Port3C_bit == 1) or (Port3D_bit == 1):
                    PCIe_Port3CD_status = 1  # Disable
                else:
                    PCIe_Port3CD_status = 0  # Enable
                print(
                    "  "
                    + PCIe_port_name[1]
                    + ": "
                    + status_N_decode(PCIe_Port3CD_status)
                )

        else:
            print("* PCIe Port #3 is invalid and will be ignored.")
            print("  " + PCIe_port_name[0] + ": Unknown")
            if iom_type == IOM_M2:
                print("  " + PCIe_port_name[1] + ": Unknown")
                print("  " + PCIe_port_name[2] + ": Unknown")
            elif iom_type == IOM_IOC:
                print("  " + PCIe_port_name[1] + ": Unknown")

        IOU2_valid_bit = (res_data[0] >> 7) & 0x1
        if IOU2_valid_bit == 1:
            Port1B_bit = (res_data[0] >> 1) & 0x1
            Port1A_bit = (res_data[0] >> 0) & 0x1

            # NIC
            if (Port1A_bit == 1) or (Port1B_bit == 1):
                PCIe_Port1AB_status = 1  # Disable
            else:
                PCIe_Port1AB_status = 0  # Enable
            print(
                "  " + PCIe_port_name[3] + ": " + status_N_decode(PCIe_Port1AB_status)
            )
        else:
            print("* PCIe Port #1 is invalid and will be ignored.")
            print("  " + PCIe_port_name[3] + ": Unknown")

        return None
    else:  # enable or disable
        if device == "--nic":
            # IOU2 setting: set the valid bit [7] to 'available' and restore bit [6] to 'enable'
            req_data[0] = (res_data[0] | (1 << 7)) & ~(1 << 6)
            # Port 1A and 1B
            pcie_setup_data[0] = (pcie_setup_data[0] | (1 << 1)) | (1 << 0)
            # Keep IOU1 setting
            req_data[1] = res_data[1]
        else:
            # Keep the IOU2 setting
            req_data[0] = res_data[0]
            # IOU1 setting: set the valid bit [7] to 'available' and restore bit [6] to 'enable'
            req_data[1] = (res_data[1] | (1 << 7)) & ~(1 << 6)

            if device == "--scc_ioc":
                # Port 3A and 3B
                pcie_setup_data[1] = (pcie_setup_data[1] | (1 << 1)) | (1 << 0)
            elif iom_type == IOM_M2:
                if device == "--flash1":
                    # Port 3C
                    pcie_setup_data[1] = pcie_setup_data[1] | (1 << 2)
                elif device == "--flash2":
                    # Port 3D
                    pcie_setup_data[1] = pcie_setup_data[1] | (1 << 3)
            elif iom_type == IOM_IOC:
                if device == "--iom_ioc":
                    # Port 3C and 3D
                    pcie_setup_data[1] = (pcie_setup_data[1] | (1 << 3)) | (1 << 2)

        if function == "enable":
            req_data[0] = req_data[0] & ~(pcie_setup_data[0])
            req_data[1] = req_data[1] & ~(pcie_setup_data[1])
        elif function == "disable":
            req_data[0] = req_data[0] | pcie_setup_data[0]
            req_data[1] = req_data[1] | pcie_setup_data[1]

        return req_data
