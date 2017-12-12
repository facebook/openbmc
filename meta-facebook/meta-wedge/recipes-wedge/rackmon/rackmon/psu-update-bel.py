#!/usr/bin/python3

import sys
import os
import socket
from collections import namedtuple
import struct
import traceback
import argparse
import time
import json
from binascii import hexlify, unhexlify


def bh(bs):
    return hexlify(bs).decode('utf-8')


status = {
    'pid': os.getpid(),
    'state': 'started'
}

def auto_int(x):
    return int(x, 0)

delay = 0
pct_done = 0
bel_logline = ''
log_clear = False
def bel_log(s):
    global pct_done
    global bel_logline
    global log_clear
    if log_clear and s != '':
        bel_logline = ''
        if sys.stdout.isatty():
            sys.stdout.write('\n')
        log_clear = False
    if s.find('\n') != -1:
        bel_logline += s.strip()
        status_state(bel_logline)
        log_clear = True
    else:
        bel_logline += s
        status_state(bel_logline)
    if sys.stdout.isatty():
        sys.stdout.write('\r[%d%%] %s' % (pct_done, bel_logline))
        sys.stdout.flush()
    else:
        print('[%d%%] %s' % (pct_done, bel_logline.strip()))

def bprint(s):
    bel_log(s + '\n')

parser = argparse.ArgumentParser()
parser.add_argument('--addr', type=auto_int, required=True,
                    help="PSU Modbus Address")
parser.add_argument('--statusfile', default=None,
                    help="Write status to JSON file during process")
parser.add_argument('--rmfwfile', action='store_true',
                    help="Delete FW file after update completes")
parser.add_argument('file', help="firmware file")


statuspath = None

def write_status():
    global status
    if statuspath is None:
        return
    tmppath = statuspath + '~'
    with open(tmppath, 'w') as tfh:
        tfh.write(json.dumps(status))
    os.rename(tmppath, statuspath)

def status_state(state):
    global status
    status['state'] = state
    write_status()

BelCommand = namedtuple('BelCommand', ['type', 'data'])

class ModbusTimeout(Exception):
    pass


class ModbusCRCFail(Exception):
    pass


class ModbusUnknownError(Exception):
    pass


class BadMEIResponse(Exception):
    pass


def rackmon_command(cmd):
    srvpath = "/var/run/rackmond.sock"
    replydata = []
    if os.path.exists(srvpath):
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        client.connect(srvpath)
        cmdlen = struct.pack("@H", len(cmd))
        client.send(cmdlen)
        client.send(cmd)
        while True:
            data = client.recv(1024)
            if not data:
                break
            replydata.append(data)
        client.close()
    return b''.join(replydata)

def pause_monitoring():
    COMMAND_TYPE_PAUSE_MONITORING = 0x04
    command = struct.pack("@Hxx", COMMAND_TYPE_PAUSE_MONITORING)
    result = rackmon_command(command)
    (res_n, ) = struct.unpack("@B", result)
    if res_n == 1:
        bprint("Monitoring was already paused when tried to pause")
    elif res_n == 0:
        bprint("Monitoring paused")
    else:
        bprint("Unknown response pausing monitoring: %d" % res_n)


def resume_monitoring():
    COMMAND_TYPE_START_MONITORING = 0x05
    command = struct.pack("@Hxx", COMMAND_TYPE_START_MONITORING)
    result = rackmon_command(command)
    (res_n, ) = struct.unpack("@B", result)
    if res_n == 1:
        bprint("Monitoring was already running when tried to resume")
    elif res_n == 0:
        bprint("Monitoring resumed")
    else:
        bprint("Unknown response resuming monitoring: %d" % res_n)


def modbuscmd(raw_cmd, expected=0, timeout=0):
    COMMAND_TYPE_RAW_MODBUS = 1
    send_command = struct.pack("@HxxHHL",
                               COMMAND_TYPE_RAW_MODBUS,
                               len(raw_cmd),
                               expected,
                               timeout) + raw_cmd
    result = rackmon_command(send_command)
    if len(result) == 0:
        raise ModbusUnknownError()
    (resp_len,) = struct.unpack("@H", result[:2])
    if resp_len == 0:
        (error, ) = struct.unpack("@H", result[2:4])
        if error == 4:
            raise ModbusTimeout()
        if error == 5:
            raise ModbusCRCFail()
        bprint("Unknown modbus error: " + str(error))
        raise ModbusUnknownError()
    return result[2:resp_len]


last_rx = ''
address = ''
rx_failed = False

def do_dump(cmd):
    bprint(str(cmd))

def do_log(cmd):
    bel_log(cmd.data.decode('utf-8'))

def do_progress(cmd):
    global pct_done, status
    (progress, ) = struct.unpack('B', cmd.data)
    pct_done = progress
    status['flash_progress_percent'] = progress
    bel_log('')

def read_timeout(cmd):
    (ms, ) = struct.unpack('<H', cmd.data)
    return ms

def do_timeout(cmd):
    global delay
    (ms, ) = struct.unpack('<H', cmd.data)
    delay = ms

WriteCommand = namedtuple('WriteCommand', ['tx', 'rx', 'timeout'])
def read_wcmd(cmd):
    (txsz, rxsz) = struct.unpack('<BB',cmd.data[:2])
    txsz = txsz
    rxsz = rxsz
    txdata = cmd.data[3:][:txsz]
    rxdata = cmd.data[txsz+3:][:rxsz]
    # these include sent/received CRCs, which we want to remove
    # as they will be calculated and checked by rackmond
    if txdata:
        txdata = txdata[:-2]
    if rxdata:
        rxdata = rxdata[:-2]
    return WriteCommand(txdata, rxdata, 0)

def do_write(cmd):
    global last_rx
    global delay
    txsz = len(cmd.data.tx)
    rxsz = len(cmd.data.rx)
    txdata = cmd.data.tx
    rxdata = cmd.data.rx
    timeout = cmd.data.timeout
    # the 'timeout' commands in the script are not timeouts, but forced
    # inter-command delays, which we elide from TWW/TWTW command sequences,
    # then finish out after completion of the request/reponse transaction
    # if we get a response early we're still good since we have early exit from
    # expected len and finish out any requested delays
    if timeout > 0:
        delay = timeout
    # P1/Bel's original script used pyserial with a 15s timeout
    # (allowing a read to take that long to start)
    # Just in case the timeout preceding a tx-only W command was actually
    # greater, we preserve it here
    if timeout < 15000:
        timeout = 15000
    expected = 0
    if rxsz > 0:
        # address byte, CRC
        expected = rxsz + 1 + 2
    spent = 0
    addrbyte = struct.pack("B", address)
    mcmd = addrbyte + txdata
    if len(txdata) > 0:
        try:
            t1 = time.clock()
            # remove incoming address byte
            last_rx = modbuscmd(mcmd, expected=expected, timeout=timeout)[1:]
            t2 = time.clock()
            spent = (t2 - t1)
        except ModbusTimeout:
            last_rx = b''
            bprint('No response from cmd: ' + bh(mcmd))
    left = spent - (delay / 1000.0)
    if len(rxdata) > 0:
        if rxdata != last_rx:
            global rx_failed
            bprint('Expected response: %s, len: %d' % (bh(rxdata), expected))
            bprint('  Actual response: ' + bh(last_rx))
            bprint('timeout,spent,left: %d, %d, %d' % (timeout, spent * 1000, left * 1000))
            rx_failed = True
    if left > 0:
        time.sleep(left)

class BelCommandError(Exception):
    pass

def do_error(cmd):
    global rx_failed
    if rx_failed:
        bel_log(cmd.data.decode('utf-8'))
        raise BelCommandError()

cmdfuns = {
        'H': do_dump,
        'W': do_write,
        'T': do_timeout,
        'X': do_error,
        'L': do_log,
        'P': do_progress,
        'M': do_dump
}

def belcmd(cmd):
    cmdfuns[cmd.type](cmd)

def main():
    args = parser.parse_args()
    global address
    address = args.addr
    global statuspath
    statuspath = args.statusfile
    cmds = []
    script = []
    # Script excerpt, sets progress to 0 percent and logs 'Entering bootloader... '
    #
    # P004D
    # L456E746572696E6720626F6F746C6F616465722E2E2E200C
    #
    # ^ first char is command, rest is parameter data for command except for
    # final byte (two hex chars) which is a CRC8 check digit

    # Commands are one of:
    #
    # (H)eader,
    # (W)rite,
    # (T)imeout (sleep),
    # (X) Error message (to print if the last # command failed),
    # (L)og message,
    # (P)rogress -- set progress percent to given value,
    # (M) MD5 code (checksum of the entire script, excluding this line)
    try:
        bprint('Reading FW script...')
        with open(args.file, 'r') as f:
            for line in f:
                line = line.strip()
                # convert hex to bytes leave off check digit
                cmd = BelCommand(line[0], unhexlify(line[1:][:-2]))
                if cmd.type == 'W':
                    cmd = BelCommand(cmd.type, read_wcmd(cmd))
                cmds.append(cmd)
        bprint('Coalescing TX/RX commands...')
        olen = len(cmds)
        # Combine TWW / TWTW sequences (write,wait,read) into single write and
        # read with timeout commands so that the response/timeout handling can
        # be done in rackmond directly.
        # This unfortunately adds over a minute of time to an update, since
        # python is quite slow on the BMC.
        while len(cmds) > 0:
            cmd = cmds.pop(0)
            if cmds and \
               cmd.type == 'T' \
               and read_timeout(cmd) > 0 \
               and cmds[0].type == 'W' \
               and cmds[0].data.rx == b'':
                wcmd = cmds.pop(0)
                tx = wcmd.data.tx
                timeout = read_timeout(cmd)
                rx = b''
                if cmds and cmds[0].type == 'T' and read_timeout(cmds[0]) == 0:
                    cmds.pop(0)
                if cmds and cmds[0].type == 'W' and cmds[0].data.tx == b'':
                    rx = cmds.pop(0).data.rx
                cmd = BelCommand('W', WriteCommand(tx, rx, timeout))
            script.append(cmd)
        bprint('Reduced by %d cmds.' % (olen - len(script)))
        pause_monitoring()
        for cmd in script:
            belcmd(cmd)
    except Exception as e:
        bprint("Update failed")
        resume_monitoring()
        status['exception'] = traceback.format_exc()
        status_state('failed')
        traceback.print_exc()
        sys.exit(1)
    resume_monitoring()
    status_state('done')
    if args.rmfwfile:
        os.remove(args.file)
    print('\nDone')
if __name__ == "__main__":
    main()
