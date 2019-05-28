#!/usr/bin/env python3

import os
import re
import sys
import time


err_list = []
succ_list = []

# ['name', 'cmd0', 'opcode0', 'expect0', 'cmd1', 'opcode1', 'expect1']
TestItem = [
    # ['sol.sh', 'sol.sh', '', '0x', '', '', '', '', '', '', ''],
    [
        "slot_id",
        "cat /sys/class/i2c-adapter/i2c-12/12-0031/slotid",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
    ],
    ["SCM_slot_id", "i2cget -f -y 0 0x3e 0x03", ">=", "0", "", "", "", "", "", "", ""],
    ["board id", "i2cget -f -y 6 0x27 0x0", ">=", "0", "", "", "", "", "", "", ""],
    #####BUS1######
    [
        "BUS(1) rertimer(0x18)",
        "i2cget -f -y 0 0x18 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 0 0x18",
    ],
    [
        "BUS(1) ADM1278(0x10)",
        "i2cget -f -y 11 0x10 0x1",
        ">=",
        "0",
        "i2cget -f -y 12 0x31 0x05",
        "=",
        "0",
        "",
        "",
        "",
        "i2cdump -f -y 0 0x10",
    ],
    [
        "BUS(1) PCA9534(0x21)",
        "i2cget -f -y 0 0x21 0x0",
        ">=",
        "0",
        "i2cget -f -y 12 0x31 0x05",
        "=",
        "0",
        "",
        "",
        "",
        "i2cdump -f -y 0 0x21",
    ],
    [
        "BUS(1) EEPROM(0x52)",
        "i2cget -f -y 0 0x52 0x0",
        ">=",
        "0",
        "i2cget -f -y 12 0x31 0x05",
        "=",
        "0",
        "",
        "",
        "",
        "i2cdump -f -y 0 0x52",
    ],
    [
        "BUS(1) EC(0x33)",
        "i2cget -f -y 0 0x33 0x0",
        ">=",
        "0",
        "i2cset -f -y 0 0x3e 0x18 0x01",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 0 0x33",
    ],
    [
        "BUS(1) SCM_CPLD(0x3e)",
        "i2cget -f -y 0 0x3e 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 0 0x3e",
    ],
    [
        "BUS(1) EEPROM(0x54)",
        "i2cget -f -y 0 0x54 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 0 0x54",
    ],
    ######BUS2#######
    [
        "BUS(2) IR3584(0x10)",
        "i2cget -f -y 1 0x10 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 1 0x10",
    ],
    [
        "BUS(2) IR3584(0x11)",
        "i2cget -f -y 1 0x11 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 1 0x11",
    ],
    [
        "BUS(2) retimer(0x18)",
        "i2cget -f -y 1 0x18 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 1 0x18",
    ],
    ######BUS3#######
    [
        "BUS(3) pwr1014a(0x40)",
        "i2cget -f -y 2 0x40 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 2 0x40",
    ],
    ######BUS5#######
    ######BUS6#######
    [
        "BUS(6) TPM(0x20)",
        "i2cget -f -y 5 0x20 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 5 0x20",
    ],
    ######BUS7#######
    [
        "BUS(7) PCF8574(0x27)",
        "i2cget -f -y 6 0x27 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 6 0x27",
    ],
    [
        "BUS(7) EEPROM(0x51)",
        "i2cget -f -y 6 0x51 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 6 0x51",
    ],
    ######BUS10#######
    [
        "BUS(10) EEPROM(0x50)",
        "i2cget -f -y 9 0x50 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 9 0x50",
    ],
    ######BUS12#######
    [
        "BUS(12) ADM1278(0x10)",
        "i2cget -f -y 11 0x10 0x1",
        ">=",
        "0",
        "i2cget -f -y 12 0x31 0x05",
        "=",
        "0",
        "",
        "",
        "",
        "i2cdump -f -y 11 0x10",
    ],
    ######BUS13#######
    [
        "BUS(13) SYS_CPLD(0x31)",
        "i2cget -f -y 12 0x31 0x0",
        ">=",
        "0",
        "",
        "",
        "",
        "",
        "",
        "",
        "i2cdump -f -y 12 0x31",
    ],
    [
        "BUS(13) PCA9541(0x74)",
        "i2cget -f -y 12 0x74 0x0",
        ">=",
        "0",
        "i2cget -f -y 12 0x31 0x3",
        "<",
        "0x8",
        "",
        "",
        "",
        "i2cdump -f -y 12 0x74",
    ],
    #####sys led######
    ["SYS_LED", "i2cset -f -y 12 0x31 0x06 0xff", "", "", "", "", "", "", "", "", ""],
    [
        "BIOS-Flash",
        "bios_flash.sh",
        "",
        "W25Q64.V",
        "i2cset -f -y 0 0x3e 0x18 0x0",
        "",
        "",
        "",
        "",
        "",
        "",
    ],
    # ['ifconfig eth0', 'ifconfig eth0 down;ifconfig eth0 192.168.0.15 up;ping 192.168.0.11', '', 'ttl', '', '', '', '', '', '', ''],
    # for test
    # ['test None', 'echo 123', '','1', 'echo 1', '>=', '1', 'echo 2', '=', '2', ''],
]


class TestObj:
    def __init__(self, item):
        self.name = item[0]
        self.cmd0 = item[1]
        self.opcode0 = item[2]
        self.expect0 = item[3]
        self.cmd1 = item[4]
        self.opcode1 = item[5]
        self.expect1 = item[6]
        self.cmd2 = item[7]
        self.opcode2 = item[8]
        self.expect2 = item[9]
        self.extern_cmd = item[10]


class Galaxy100Test(object):
    def __init__(self, cmd_item):
        self.cmd0 = cmd_item.cmd0
        self.recv = {}
        self.name = cmd_item.name
        self.opcode0 = cmd_item.opcode0
        self.expect0 = cmd_item.expect0
        self.cmd1 = cmd_item.cmd1
        self.opcode1 = cmd_item.opcode1
        self.expect1 = cmd_item.expect1
        self.cmd2 = cmd_item.cmd2
        self.opcode2 = cmd_item.opcode2
        self.expect2 = cmd_item.expect2
        self.extern_cmd = cmd_item.extern_cmd

    def sendcmd(self):
        print("\033[1;34;44m" + "TestItem:  " + '\033[0m"' + self.name + '"')
        print("\033[1;34;44m" + "CMD:  " + '\033[0m"' + self.cmd0 + '"', end=" ")
        # sys.stdout.flush()
        # self.recv = os.popen(self.cmd0).read()

    def receive(self):
        # print 'self.recv:' + self.recv
        if self.extern_cmd != "" and self.recv != "":
            print("result: " + self.recv, end=" ")
            print(
                "\033[1;34;44m" + "Extern Cmd:  " + '\033[0m"' + self.extern_cmd + '"'
            )

            print(os.popen(self.extern_cmd).read())
        else:
            print("result: " + self.recv)

    def charge(self, opcode, expect, recv):
        # print 'recv: ' + recv
        if opcode == "":
            ret = re.search(expect, recv)
            # print ret
            if str(ret) == "None":
                return -1
            else:
                return 0
        elif opcode == ">":
            # print 'self.recv:' + self.recv + 'expext:' + self.expect0
            if recv == "":
                return -1
            ret = re.search("0x", recv)
            # print ret
            if str(ret) == "None":
                if int(recv) > int(expect):
                    return 0
                else:
                    return -1
            else:
                if int(recv, 16) > int(expect, 16):
                    return 0
                else:
                    return -1

        elif opcode == ">=":
            # print 'self.recv:' + self.recv + 'expext:' + self.expect0
            if recv == "":
                return -1
            ret = re.search("0x", recv)
            if str(ret) == "None":
                if int(recv) >= int(expect):
                    return 0
                else:
                    return -1
            else:
                if int(recv, 16) >= int(expect, 16):
                    return 0
                else:
                    return -1
        elif opcode == "<":
            if recv == "":
                return -1
            ret = re.search("0x", recv)
            if str(ret) == "None":
                if int(recv) < int(expect):
                    return 0
                else:
                    return -1
            else:
                if int(recv, 16) < int(expect, 16):
                    return 0
                else:
                    return -1
        elif opcode == "<=":
            if recv == "":
                return -1
            ret = re.search("0x", recv)
            if str(ret) == "None":
                if int(recv) <= int(expect):
                    return 0
                else:
                    return -1
            else:
                if int(recv, 16) <= int(expect, 16):
                    return 0
                else:
                    return -1
        elif str(opcode) == "=":
            if recv == "":
                return -1
            ret = re.search("0x", recv)
            if str(ret) == "None":
                if int(recv) == int(expect):
                    return 0
                else:
                    return -1
            else:
                if int(recv, 16) == int(expect, 16):
                    return 0
                else:
                    return -1

    def judge(self):
        if self.cmd1 != "":
            # print '\n\033[1;34;44m' +'Need Charge0:  ' + '\033[0m\"' +  self.cmd1 + '\"',
            sys.stdout.flush()
            self.recv = os.popen(self.cmd1).read()
            ret = self.charge(self.opcode1, self.expect1, self.recv)
            if ret != 0:  # if !=0 , ignore cmd0 and return OK
                # print '     \033[1;33;43m' + '[Not need to test]' + '\033[0m';
                return -1
        if self.cmd2 != "":
            # print '\n\033[1;34;44m' +'Need Charge1:  ' + '\033[0m\"' +  self.cmd2 + '\"',
            sys.stdout.flush()
            self.recv = os.popen(self.cmd2).read()
            ret = self.charge(self.opcode2, self.expect2, self.recv)
            if ret != 0:  # if !=0 , ignore cmd1 and return OK
                # print '     \033[1;33;43m' + '[Not need to test]' + '\033[0m';
                return -1
        return 0

    def verify(self):
        # self.sendcmd()
        sys.stdout.flush()
        self.recv = os.popen(self.cmd0).read()
        ret = self.charge(self.opcode0, self.expect0, self.recv)
        if ret == 0:
            print("     \033[1;32;42m" + "[PASS]" + "\033[0m")
            succ_list.append(self.name)
            return 0
        else:
            print("     \033[1;31;41m" + "[Fail]" + "\033[0m")
            err_list.append(self.name)
            return -1


def main():
    i = 0
    error = 0
    success = 0
    print("TestItem len:" + str(len))
    for i in range(len):
        testObj = TestObj(TestItem[i])
        test = Galaxy100Test(testObj)
        if test.judge() == 0:
            test.sendcmd()
            if test.verify() == 0:
                success += 1
            else:
                error += 1
            test.receive()
            time.sleep(1)
        # continue
    if succ_list:
        print("\033[1;32;42m" + "Success items: [" + str(success) + "]")
        for item in succ_list:
            print(item)
    if err_list:
        print("\033[1;31;41m" + "\nError items: [" + str(error) + "]")
        for item in err_list:
            print(item)
        print("\033[0m")


len = len(TestItem)
rc = main()
