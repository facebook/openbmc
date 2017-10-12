#!/usr/bin/python
import sys
import os.path
from subprocess import Popen, PIPE

IPMIUTIL = "/usr/local/fbpackages/ipmi-util/ipmi-util"
PLATFORM_FILE = "/tmp/system.bin"
POST_CODE_FILE = "/tmp/post_code_buffer.bin"

NODE_NUM = "1"  # FRU:1 Server
IOM_M2 = 1      # IOM type: M.2
IOM_IOC = 2     # IOM type: IOC

boot_order_device = { 0: "USB Device", 1: "IPv4 Network", 9: "IPv6 Network", 2: "SATA HDD", 3: "SATA-CDROM", 4: "Other Removalbe Device" }
PCIe_port_name_T5 = {0: "SCC IOC", 1: "flash1", 2: "flash2", 3: "NIC"}
PCIe_port_name_T7 = {0: "SCC IOC", 1: "IOM IOC", 3: "NIC"}

def usage():
    print("Usage: bios-util [ boot_order, plat_info, pcie_config, pcie_port_config, postcode ] <options>")
    exit(-1)
    
def usage_boot_order():
    print("Usage: bios-util boot_order --clear_CMOS <--enable, --disable, --get>")
    print("                            --force_boot_BIOS_setup <--enable, --disable, --get>")
    print("                            --boot_order [ --set < #1st #2nd #3rd #4th #5th >, --get, --disable ]")
    print("       *" + repr(boot_order_device))
    exit(-1)
        
def usage_plat_info():
    print("Usage: bios-util plat_info --get")
    exit(-1)
    
def usage_PCIE_config():
    print("Usage: bios-util pcie_config --get")
    exit(-1)

def usage_PCIE_port_config():
    iom_type = get_iom_type()

    if ( iom_type == IOM_M2 ):
        print("Usage: bios-util pcie_port_config [ --set <--enable, --disable> <scc_ioc, flash1, flash2, nic>, --get ]")
    elif ( iom_type == IOM_IOC ):
        print("Usage: bios-util pcie_port_config [ --set <--enable, --disable> <scc_ioc, iom_ioc, nic>, --get ]")
    exit(-1)

def usage_postcode():
    print("Usage: bios-util postcode --get")
    exit(-1)

def get_iom_type():
    pal_sku_file = open(PLATFORM_FILE, 'r')
    pal_sku = pal_sku_file.read()
    iom_type = int(pal_sku) & 0x3  # The IOM type is the last 2 bits

    if ( iom_type != IOM_M2) and ( iom_type != IOM_IOC):
        print("System type is unknown! Please confirm the system type.")
        exit(-1)
    else:
        return iom_type

def execute_IPMI_command(netfn, cmd, req):
    netfn = (netfn << 2)

    sendreqdata = ""
    if ( req != "" ):
      for i in range(0, len(req), 1):
          sendreqdata += str(req[i]) + " "

    input = IPMIUTIL + " " + NODE_NUM + " " + str(netfn) + " " + str(cmd) + " " + sendreqdata
    data=''

    output = Popen(input, shell=True, stdout=PIPE)
    (data, err) =  output.communicate()
    result = data.strip().split(" ")

    if ( output.returncode != 0 ):
        print("ipmi-util execute fail..., please check ipmid.")
        exit(-1)    

    if ( int(result[2], 16) != 0 ):
        print("IPMI Failed, CC Code:%s" % result[2])
        exit(0)
    
    return result[3:]
    
def status_decode(input):
    if ( input == 1 ):
        return "Enabled"
    else:
        return "Disabled"
        
def status_N_decode(input):
    if ( input == 1 ):
        return "Disabled"
    else:
        return "Enabled"

def trans2opcode(input):
    if ( input == "--enable" ):
        return 1
    else:
        return 0

'''
OEM Set BIOS Boot Order (NetFn:0x30, CMD: 0x52h)
Request:
   Byte 1 - Boot mode
     Bit 0 - 0 : Legacy, 1 : UEFI
     Bit 1 - CMOS clear (Optional, BIOS implementation dependent)
     Bit 2 - Force Boot into BIOS Setup (Optional, BIOS implementation dependent)
     Bit 6:3 - reserved
     Bit 7 - boot flags valid
  Byte 2-6 - Boot sequence
     Bit 2:0 - boot device id
         000b: USB device
         001b: Network
         010b: SATA HDD
         011b: SATA-CDROM
         100b: Other removable Device
     Bit 7:3 - reserve for boot device special request
           If Bit 2:0 is 001b (Network), Bit3 is IPv4/IPv6 order
              Bit3=0b: IPv4 first
              Bit3=1b: IPv6 first
Response:
Byte1 - Completion Code
'''
def boot_order(argv):
    req_data = [""]
    function = argv[2]
    option = argv[3]
    boot_flags_valid = 1  

    result = execute_IPMI_command(0x30, 0x53, "")
    
    data = [int(n, 16) for n in result]
        
    clear_CMOS = ((data[0] & 0x2) >> 1)
    force_boot_BIOS_setup = ((data[0] & 0x4) >> 2)
    boot_order = data[1:]
    
    if ( option == "--get" ):
        if ( function == "--boot_order" ):
            try:
                print("Boot Order: " + ", ".join(boot_order_device[dev] for dev in boot_order))
            except KeyError:
                print("Invalid Boot Device ID!")
                print(boot_order_device)
        elif ( function == "--clear_CMOS" ):
            print("Clear CMOS Function: " + status_decode(clear_CMOS))
            exit(0)
        elif ( function == "--force_boot_BIOS_setup" ):
            print("Force Boot to BIOS Setup Function: " + status_decode(force_boot_BIOS_setup))
            exit(0)
        else:
            usage_boot_order() 
    elif ( option == "--enable" ):
        if ( function == "--clear_CMOS" ):
            clear_CMOS = trans2opcode(option)
        elif ( function == "--force_boot_BIOS_setup" ):
            force_boot_BIOS_setup = trans2opcode(option)
        else:
            usage_boot_order()

    elif ( option == "--disable" ):
        if ( function == "--clear_CMOS" ):
            clear_CMOS = trans2opcode(option)
        elif ( function == "--force_boot_BIOS_setup" ):
            force_boot_BIOS_setup = trans2opcode(option)
        elif ( function != "--boot_order" ):
            usage_boot_order()
            
        #Clear the 7th valid bit for disable clean CMOS, force boot to BIOS setup, and set boot order action
        boot_flags_valid = 0

    elif ( option == "--set" ):
        if ( function == "--boot_order" ):
            if ( len(argv) < 9 ):
                usage_boot_order() 
            
            set_boot_order = argv[4:]
            for num in set_boot_order:
                if ( not int(num) in boot_order_device ):
                    print("Invalid Boot Device ID!")
                    usage_boot_order()
            boot_order = set_boot_order
        else:
            usage_boot_order()

    else:
        usage_boot_order() 
    
    req_data[0] = ((((data[0] & ~0x86) | (clear_CMOS << 1)) | (force_boot_BIOS_setup << 2)) |  (boot_flags_valid << 7) )
    req_data[1:] = boot_order
    
    execute_IPMI_command(0x30, 0x52, req_data)  
    
'''
OEM Get Platform Info (NetFn:0x30, CMD: 0x7Eh)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - Node Slot Index
     Bit 7 - 1b: present, 0b: not present
     Bit 6 - 1b: Test Board, 0b: Non Test Board
     Bit 5:3  - SKU ID
         000b: Yosemite
         010b: Triton-Type 5A (Left sub-system)
         011b: Triton-Type 5B (Right sub-system)
         100b: Triton-Type 7 SS (IOC based IOM)
     Bit 2:0 - Slot Index, 1 based
'''
def plat_info():
    req_data = [""]
    presense = "Not Present"
    test_board = "Non Test Board"
    SKU = "Unknown"
    slot_index = ""
    result = execute_IPMI_command(0x30, 0x7E, "")
    
    data = int(result[0], 16)
    
    if ( data & 0x80 ):
        presense = "Present"
        
    if ( data & 0x40 ):
        test_board = "Test Board"
    
    SKU_ID = ((data & 0x38) >> 3 )
    if ( SKU_ID == 0 ):
        SKU = "Yosemite"
    elif ( SKU_ID == 2 ):
        SKU = "Triton-Type 5A (Left sub-system)"
    elif ( SKU_ID == 3 ):
        SKU = "Triton-Type 5B (Right sub-system)"
    elif ( SKU_ID == 4 ):
        SKU = "Triton-Type 7 SS (IOC based IOM)"
        
    slot_index = str((data & 0x7))
    
    print("Presense: " + presense)
    print(test_board)
    print("SKU: " + SKU)
    print("Slot Index: " + slot_index)

'''
OEM Get PCIe Configuration (NetFn:0x30, CMD: 0xF4h)
Request:
   NA
Response:
   Byte 1 - Completion Code
   Byte 2 - config number
     0x6: Triton-Type 5A/5B
     0x8: Triton-Type 7
     0xA: Yosemite
'''
def pcie_config():
    req_data = [""]
    result = execute_IPMI_command(0x30, 0xF4, "")
    
    if ( result[0] == "06" ):
        config = "Triton-Type 5"
    elif ( result[0] == "08" ):
        config = "Triton-Type 7"
    elif ( result[0] == "0A" ):
        config = "Yosemite"
    else:
        config = "Unknown"
    print("PCIe Configuration:" + config)

'''
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
'''
'''
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
'''
def pcie_port_config(argv):
    req_data = ["", ""]
    res_data = ["", ""]
    pcie_setup_data = [0, 0]
    function = argv[2]

    # Get IOM type
    iom_type = get_iom_type()
    if ( iom_type == IOM_M2 ):
        PCIe_port_name = PCIe_port_name_T5
    elif ( iom_type == IOM_IOC ):
        PCIe_port_name = PCIe_port_name_T7
    else:
        usage_PCIE_port_config()

    # Get the original config
    result = execute_IPMI_command(0x30, 0x80, "")
    res_data = [int(n, 16) for n in result]

    if ( function == "--set" ):
        option = argv[3]
        device = argv[4]

        if ( device == "nic" ):
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

            if ( device == "scc_ioc" ):
                # Port 3A and 3B
                pcie_setup_data[1] = (pcie_setup_data[1] | (1 << 1)) | (1 << 0)
            elif ( iom_type == IOM_M2 ):
                if ( device == "flash1" ):
                    # Port 3C
                    pcie_setup_data[1] = (pcie_setup_data[1] | (1 << 2))
                elif ( device == "flash2" ):
                    # Port 3D
                    pcie_setup_data[1] = (pcie_setup_data[1] | (1 << 3))
                else:
                    usage_PCIE_port_config()
            elif ( iom_type == IOM_IOC ):
                if ( device == "iom_ioc" ):
                    # Port 3C and 3D
                    pcie_setup_data[1] = (pcie_setup_data[1] | (1 << 3)) | (1 << 2)
                else:
                    usage_PCIE_port_config()
            else:
                usage_PCIE_port_config()

        if ( option == "--enable" ):
            req_data[0] = req_data[0] & ~(pcie_setup_data[0])
            req_data[1] = req_data[1] & ~(pcie_setup_data[1])
        elif ( option == "--disable" ):
            req_data[0] = req_data[0] | pcie_setup_data[0]
            req_data[1] = req_data[1] | pcie_setup_data[1]
        else:
            usage_PCIE_port_config()

        execute_IPMI_command(0x30, 0x81, req_data)

    elif ( function == "--get" ):
        # If the valid bit is invalid, it will be ignored
        IOU1_valid_bit = (res_data[1] >> 7) & 0x1
        if ( IOU1_valid_bit == 1 ):
            Port3D_bit = (res_data[1] >> 3) & 0x1
            Port3C_bit = (res_data[1] >> 2) & 0x1
            Port3B_bit = (res_data[1] >> 1) & 0x1
            Port3A_bit = (res_data[1] >> 0) & 0x1

            # SCC IOC
            if ( Port3A_bit == 1 ) or ( Port3B_bit == 1 ):
                PCIe_Port3AB_status = 1  # Disable
            else:
                PCIe_Port3AB_status = 0  # Enable
            print("  " + PCIe_port_name[0] + ": " + status_N_decode(PCIe_Port3AB_status))

            if ( iom_type == IOM_M2 ):
                # IOM M.2
                print("  " + PCIe_port_name[1] + ": " + status_N_decode(Port3C_bit))
                print("  " + PCIe_port_name[2] + ": " + status_N_decode(Port3D_bit))
            elif ( iom_type == IOM_IOC ):
                # IOM IOC
                if ( Port3C_bit == 1 ) or ( Port3D_bit == 1 ):
                    PCIe_Port3CD_status = 1  # Disable
                else:
                    PCIe_Port3CD_status = 0  # Enable
                print("  " + PCIe_port_name[1] + ": " + status_N_decode(PCIe_Port3CD_status))
            else:
                usage_PCIE_port_config()
        else:
            print("* PCIe Port #3 is invalid and will be ignored.")
            print("  " + PCIe_port_name[0] + ": Unknown")
            if ( iom_type == IOM_M2 ):
                print("  " + PCIe_port_name[1] + ": Unknown")
                print("  " + PCIe_port_name[2] + ": Unknown")
            elif ( iom_type == IOM_IOC ):
                print("  " + PCIe_port_name[1] + ": Unknown")
            else:
                usage_PCIE_port_config()

        IOU2_valid_bit = (res_data[0] >> 7) & 0x1
        if ( IOU2_valid_bit == 1 ):
            Port1B_bit = (res_data[0] >> 1) & 0x1
            Port1A_bit = (res_data[0] >> 0) & 0x1

            # NIC
            if ( Port1A_bit == 1 ) or ( Port1B_bit == 1 ):
                PCIe_Port1AB_status = 1  # Disable
            else:
                PCIe_Port1AB_status = 0  # Enable
            print("  " + PCIe_port_name[3] + ": " + status_N_decode(PCIe_Port1AB_status))
        else:
            print("* PCIe Port #1 is invalid and will be ignored.")
            print("  " + PCIe_port_name[3] + ": Unknown")

    else:
        usage_PCIE_port_config()

def postcode():
    if os.path.isfile(POST_CODE_FILE):
        postcode_file = open(POST_CODE_FILE, 'r')
        print(postcode_file.read())
    else:
        print("POST code file is not exist!")

def bios_main():
    if ( len(sys.argv) < 2 ):
        usage()

    if ( sys.argv[1] == "boot_order" ):
        if ( len(sys.argv) < 4 ):
            usage_boot_order()
        else:
            boot_order(sys.argv)
        
    elif ( sys.argv[1] == "plat_info" ):
        if ( len(sys.argv) != 3 ) or ( sys.argv[2] != "--get" ):
            usage_plat_info()
        else:
            plat_info()
        
    elif ( sys.argv[1] == "pcie_config" ):
        if ( len(sys.argv) != 3 ) or ( sys.argv[2] != "--get" ):
            usage_PCIE_config()
        else:
            pcie_config()

    elif ( sys.argv[1] == "pcie_port_config" ):   
        if ( len(sys.argv) < 3 ):
            usage_PCIE_port_config()
        elif ( sys.argv[2] == "--get" ) and ( len(sys.argv) != 3 ):
            usage_PCIE_port_config()
        elif ( sys.argv[2] == "--set" ) and ( len(sys.argv) != 5 ):
            usage_PCIE_port_config()
        else:
            pcie_port_config(sys.argv)

    elif ( sys.argv[1] == "postcode" ):
        if ( len(sys.argv) != 3 ) or ( sys.argv[2] != "--get" ):
            usage_postcode()
        else:
            postcode()

    else:
        usage()
        
if ( __name__ == '__main__' ):
    bios_main()
