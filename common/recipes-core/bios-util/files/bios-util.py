#!/usr/bin/python
import sys
import re
import os.path
from subprocess import Popen, PIPE
from ctypes import *

lpal_hndl = CDLL("libpal.so")

POST_CODE_FILE = "/tmp/post_code_buffer.bin"
IPMIUTIL = "/usr/local/fbpackages/ipmi-util/ipmi-util"

boot_order_device = { 0: "USB Device", 1: "IPv4 Network", 9: "IPv6 Network", 2: "SATA HDD", 3: "SATA-CDROM", 4: "Other Removalbe Device" }
supported_commands = ["boot_order", "postcode"]

def pal_print_postcode(fru):
    postcodes = (c_ubyte * 256)()
    plen = c_ushort(0)
    ret = lpal_hndl.pal_get_80port_record(fru, 0, 0, byref(postcodes), byref(plen))
    if ret != 0:
        print("Error %d returned by get_80port" % (ret))
        return
    for i in range(0, plen.value):
        sys.stdout.write("%02X " % (postcodes[i]))
        sys.stdout.flush()
        if (i%16 == 15):
            sys.stdout.write("\n")
            sys.stdout.flush()
    if plen.value % 16 != 0:
        sys.stdout.write("\n")
    sys.stdout.flush()

def pal_get_fru_id(fruname):
    fruid = c_ubyte(0)
    ret = lpal_hndl.pal_get_fru_id(c_char_p(fruname), byref(fruid))
    if (ret != 0):
        return ret
    return fruid.value

def pal_is_slot_server(fru):
    ret = lpal_hndl.pal_is_slot_server(c_ubyte(fru))
    if ret != 1:
        return False
    return True

def usage():
    print("Usage: bios-util FRU [ boot_order, postcode ] <options>")
    exit(-1)
    
def usage_boot_order():
    print("Usage: bios-util FRU boot_order --clear_CMOS <--enable, --disable, --get>")
    print("                                --force_boot_BIOS_setup <--enable, --disable, --get>")
    print("                                --boot_order [ --set < #1st #2nd #3rd #4th #5th >, --get, --disable ]")
    print("       *" + repr(boot_order_device))
    exit(-1)

def usage_postcode():
    print("Usage: bios-util postcode --get")
    exit(-1)

def execute_IPMI_command(fru, netfn, cmd, req):
    netfn = (netfn << 2)

    sendreqdata = ""
    if ( req != "" ):
      for i in range(0, len(req), 1):
          sendreqdata += str(req[i]) + " "

    input = IPMIUTIL + " " + str(fru) + " " + str(netfn) + " " + str(cmd) + " " + sendreqdata
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
def boot_order(fru, argv):
    req_data = [""]
    function = argv[2]
    option = argv[3]
    boot_flags_valid = 1  

    result = execute_IPMI_command(fru, 0x30, 0x53, "")
    
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
    
    execute_IPMI_command(fru, 0x30, 0x52, req_data)

def postcode(fru):
    if os.path.isfile(POST_CODE_FILE):
        postcode_file = open(POST_CODE_FILE, 'r')
        print(postcode_file.read())
    else:
        pal_print_postcode(fru)

def bios_main_fru(fru, command):
    if ( command == "boot_order" ):
        if ( len(sys.argv) < 5 ):
            usage_boot_order()
        else:
            boot_order(fru, sys.argv[1:])
    elif ( command == "postcode" ):
        if ( len(sys.argv) != 4 ) or ( sys.argv[3] != "--get" ):
            usage_postcode()
        else:
            postcode(fru)

def bios_main():
    if ( len(sys.argv) < 3 ):
        usage()
    fruname = sys.argv[1]
    command = sys.argv[2]

    if command not in supported_commands:
        usage()
        return

    if fruname == "all":
        frulist_c = create_string_buffer(128)
        ret = lpal_hndl.pal_get_fru_list(frulist_c)
        if ret:
            print("Getting fru list failed!")
            return
        frulist_s = frulist_c.value.decode()
        frulist = re.split(r',\s', frulist_s)
        for fruname in frulist:
            fru = pal_get_fru_id(fruname)
            if fru >= 0:
                if pal_is_slot_server(fru) == True:
                    print ("%s:" % (fruname))
                    bios_main_fru(fru, command)
    else:
        fru = pal_get_fru_id(fruname)
        if fru < 0:
            print("%s is not a known FRU on this platform" % (fruname))
            return
        if pal_is_slot_server(fru) == False:
            print("%s is not a server" % (fruname))
            return
        bios_main_fru(fru, command)
        
if ( __name__ == '__main__' ):
    bios_main()
