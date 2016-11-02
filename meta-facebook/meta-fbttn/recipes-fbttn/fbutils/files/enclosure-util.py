#!/usr/bin/python
import socket
import os, os.path
import time
import sys
import re
from datetime import datetime

APPNAME = 'enclosure-util'
ERR_CODE_FILE = "/tmp/error_code.bin"
# The addresses and commands are all 7-bit and hexadecimal format.
PATH_IPMBSocket = ''
EXPANDER_SLAVE_ADDR = ''
BMC_SLAVE_ADDR = 0x10
RqSeq_RqLUN = 0
EventResLUN = 0

IPMBResPkt = []
ResPkt = []

def print_usage():
    print 'Usage: %s --hdd'              % APPNAME
    print 'Usage: %s --hdd <number>'     % APPNAME
    print '       %s --error'            % APPNAME

def createSocket():
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.setblocking(1)
    client.settimeout(10.0)
    client.connect(PATH_IPMBSocket)
    return client

def sendIPMBCommand(NetFn,Cmd,Req):
    SendReqData = ''
    if Req != '':      
      for i in range(0, len(Req), 1):
          ReqData = int(Req[i], 16)
          ReqData = "%02x" % (ReqData)
          SendReqData += ReqData

    ExpanderSlaveAddr = EXPANDER_SLAVE_ADDR << 1
    BMCSlaveAddr = BMC_SLAVE_ADDR << 1
    NetFn = NetFn << 2   # NetFn/rsLUN

    #print "*IPMB socket connecting..."
    if os.path.exists(PATH_IPMBSocket):
        client = createSocket()
        #print "SendReqData:%s" % SendReqData
        # Calculate the header checksum of IPMB request
        Check1 = 256 - ((ExpanderSlaveAddr + NetFn) % 256)
        SendCmd = "%02x%02x%02x%02x%02x%02x%s%02x" % (ExpanderSlaveAddr, NetFn, Check1, BMCSlaveAddr, RqSeq_RqLUN, Cmd, SendReqData, EventResLUN)
        IPMBReqPkt = ''
        for i in range(0, len(SendCmd), 2):
            IPMBReqPkt = IPMBReqPkt + SendCmd[i:i+2] + " "
        #print '-> IPMB request data:  %s' % IPMBReqPkt

        # send command
        DATA1 = SendCmd.decode('hex')
        client.sendall(DATA1)
        #time.sleep(0.01)

        # get command
        global IPMBResPkt
        rData = client.recv(256)
        if not rData:
            print "No Data Received..."
            sys.exit(0)
        ResData = rData.replace('0x',' ').replace('x',' ')
        ResData = ResData.encode('hex')
        #print
        for i in range(0, len(ResData)/2, 1):
            IPMBResPkt.append(ResData[i*2:i*2+2])
        #print '<- IPMB response data: %s' % IPMBResPkt
        client.close()
        
        #Parse IPMBResPkt to ResPkt, only response data no header
        for i in range(7, len(IPMBResPkt), 1):
            ResPkt.append(IPMBResPkt[i])
        '''        
        # decode response package
        print '---'
        print 'Completion Code: %s' % IPMBResPkt[6]
        print 'Response Date:  %s' % IPMBResPkt[7:len(IPMBResPkt)]
        print '---'
        '''
    else:
        print "ERROR: Couldn`t connect IPMB socket!"
        sys.exit(0)

def get_hdd_status():
    Req = ''    
    HDD = [[0 for x in range(4)] for y in range(36)]
    GOOD = []
    ERROR = []
    MISSING = []
    sendIPMBCommand(0x30,0xC0,Req);
    if len(ResPkt) < 144:
        print "There something wrong with the Response Length"
        return
    for i in range(36):
        for j in range(4):
            HDD[i][j] = int(ResPkt[j+(i*4)], 16) #Parse each HDD statu to HDD array
            
    if len(sys.argv) == 2:
        for i in range(36):
            if (HDD[i][0] & 0xF) == 1:            
                GOOD.append(i)    
            elif (HDD[i][0] & 0xF) == 5:
                MISSING.append(i)
            else:
                ERROR.append(i)
        print 'GOOD HDD: %s' % GOOD
        print 'ERROR HDD: %s' % ERROR
        print 'MISSING HDD: %s' % MISSING
    else:
        if int(sys.argv[2]) > 35 or int(sys.argv[2]) < 0:
            print "Invalid HDD number, HDD number is from 0~35"
            return
        print " ".join([ hex(i) for i in HDD[int(sys.argv[2])] ])

def get_error_code():
    Req = ''
    exp_error = [13] # error code from Expander 13 bytes, Expander Using 0~99 bits
    error = [32] #error code from BMC whole 32 bytes, BMC using 100~255 bits

    sendIPMBCommand(0x30,0x11,Req);
    exp_error = ResPkt[0:13]
    try:
        file = open(ERR_CODE_FILE, "r")
    except:
        print "There is something wrong with the BMC error code file..., only showing Expander Error Code"
        exp_error = [str(item).zfill(2) for item in exp_error]
        print(" ".join([item for item in exp_error] ))
        return
        
    print 'BMC Error Code:'
    error = file.read().split()
    file.close()
    
    #Take the frist four bit from Expander side, last four bit from BMC side for the 13th byte
    error[12] = (hex((int(error[12],16) & 0xF0) + (int(exp_error[12], 16) & 0xF)))
    #Replace the frist 12 bytes to Epxander's error code
    error[0:12] = exp_error[0:12]
    #Remove "0x" and convert to Hex
    results = [hex(int(item,16)).replace('0x', '') for item in error]
    #Zero fill up to two Zeros
    results = [item.zfill(2) for item in results]
    print(" ".join([item for item in results] ))
    
if len(sys.argv) < 2:
    print "ERROR: Please set the expander location, command, and optional request!"
    print
    print_usage()
    exit(0)

# Get the location parameters.
PATH_IPMBSocket = '/tmp/ipmb_socket_9'
EXPANDER_SLAVE_ADDR = 0x13

# Get the command parameters.
if sys.argv[1] == '--hdd':
    get_hdd_status()
elif sys.argv[1] == '--error':
    get_error_code()
else:
    print_usage()
